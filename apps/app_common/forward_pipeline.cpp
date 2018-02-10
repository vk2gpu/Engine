#include "forward_pipeline.h"
#include "common.h"

#include "core/misc.h"
#include "gpu/command_list.h"
#include "gpu/resources.h"
#include "gpu/utils.h"
#include "graphics/render_graph.h"
#include "graphics/render_pass.h"
#include "resource/manager.h"

using namespace Graphics;

static RenderGraphTextureDesc GetDefaultTextureDesc(i32 w, i32 h)
{
	RenderGraphTextureDesc desc;
	desc.type_ = GPU::TextureType::TEX2D;
	desc.width_ = w;
	desc.height_ = h;
	desc.format_ = GPU::Format::R8G8B8A8_UNORM;
	return desc;
}

static RenderGraphTextureDesc GetDepthTextureDesc(i32 w, i32 h)
{
	RenderGraphTextureDesc desc;
	desc.type_ = GPU::TextureType::TEX2D;
	desc.width_ = w;
	desc.height_ = h;
	desc.format_ = GPU::Format::R24G8_TYPELESS;
	return desc;
}

const RenderGraphBufferDesc viewCBDesc = RenderGraphBufferDesc(sizeof(ViewConstants));
const RenderGraphBufferDesc objectSBDesc = RenderGraphBufferDesc(sizeof(ObjectConstants) * 100000);

struct LightConstants
{
	u32 tileSizeX_;
	u32 tileSizeY_;
	u32 numTilesX_;
	u32 numTilesY_;
	u32 numLights_;
};

struct TileInfo
{
	Math::Vec3 planes_[4];
};

struct CommonBuffers
{
	RenderGraphResource viewCB_;
	RenderGraphResource objectSB_;
};

struct LightCullingData
{
	LightConstants lightConstants_;
	RenderGraphResource outLightCB_;
	RenderGraphResource outLightSB_;
	RenderGraphResource outLightTex_;
	RenderGraphResource outLightIndicesSB_;
	RenderGraphResource outDebug_;
};

static const i32 LIGHT_BUFFER_SIZE = 1024 * 1024;

struct BaseDrawFnData
{
	DrawFn drawFn_;
};

static LightCullingData AddLightCullingPasses(DrawFn drawFn, RenderGraph& renderGraph, const CommonBuffers& cbs,
    RenderGraphResource depth, Shader* shader, const Core::ArrayView<Light>& lights)
{
	GPU::Format lightTexFormat = GPU::Format::R32_UINT;

	// Grab depth stencil descriptor.
	RenderGraphTextureDesc dsDesc;
	renderGraph.GetTexture(depth, &dsDesc);

	// Setup light constants.
	LightConstants light;
	light.numLights_ = lights.size();
	light.tileSizeX_ = 16;
	light.tileSizeY_ = 16;
	light.numTilesX_ = dsDesc.width_ / light.tileSizeX_;
	light.numTilesY_ = dsDesc.height_ / light.tileSizeY_;

	// Data for passing between render passes.
	struct UpdateLightsPassData : BaseDrawFnData
	{
		LightConstants light_;
		Light* lights_ = nullptr;

		RenderGraphResource outLightCB_, outLightSB_, outLightIndex_;
	};

	struct ComputeTileInfoPassData : BaseDrawFnData
	{
		LightConstants light_;

		RenderGraphResource inViewCB_, inLightCB_;
		RenderGraphResource outTileInfoSB_;

		ShaderTechnique tech_;

		mutable ShaderBindingSet viewBindings_;
		mutable ShaderBindingSet tileInfoBindings_;
	};

	struct ComputeLightListsPassData : BaseDrawFnData
	{
		LightConstants light_;
		GPU::Format depthFormat_;

		RenderGraphResource inViewCB_, inLightCB_, inLightSB_, inTileInfoSB_, inDepth_;
		RenderGraphResource outLightTex_, outLightIndicesSB_, outLightIndex_;

		ShaderTechnique tech_;

		mutable ShaderBindingSet viewBindings_;
		mutable ShaderBindingSet lightBindings_;
		mutable ShaderBindingSet lightListBindings_;
	};

	struct DebugOutputPassData : BaseDrawFnData
	{
		LightConstants light_;

		RenderGraphResource inViewCB_, inLightCB_, inLightSB_, inTileInfoSB_, inLightTex_, inLightIndicesSB_;
		RenderGraphResource outDebug_;

		ShaderTechnique tech_;

		mutable ShaderBindingSet viewBindings_;
		mutable ShaderBindingSet lightBindings_;
		mutable ShaderBindingSet lightListBindings_;
		mutable ShaderBindingSet debugBindings_;
	};

	auto& updateLightsPassData =
	    renderGraph
	        .AddCallbackRenderPass<UpdateLightsPassData>("Update Light Buffers",
	            [&](RenderGraphBuilder& builder, UpdateLightsPassData& data) {
		            data.drawFn_ = drawFn;
		            data.light_ = light;
		            data.lights_ = builder.Push(lights.data(), lights.size());

		            data.outLightCB_ =
		                builder.Write(builder.Create("LC LightCB", RenderGraphBufferDesc(sizeof(LightConstants))));
		            data.outLightSB_ = builder.Write(builder.Create(
		                "LC LightSB", RenderGraphBufferDesc(sizeof(Light) * Core::Max(1, light.numLights_))));
	            },
	            [](RenderGraphResources& res, GPU::CommandList& cmdList, const UpdateLightsPassData& data) {
		            cmdList.UpdateBuffer(res.GetBuffer(data.outLightCB_), 0, sizeof(data.light_), &data.light_);
		            if(data.light_.numLights_ > 0)
			            cmdList.UpdateBuffer(
			                res.GetBuffer(data.outLightSB_), 0, sizeof(Light) * data.light_.numLights_, data.lights_);

	            })
	        .GetData();

	auto& computeTileInfoPassData =
	    renderGraph
	        .AddCallbackRenderPass<ComputeTileInfoPassData>("Compute Tile Info",
	            [&](RenderGraphBuilder& builder, ComputeTileInfoPassData& data) {
		            data.drawFn_ = drawFn;
		            data.light_ = light;

		            data.inViewCB_ = builder.Read(cbs.viewCB_, GPU::BindFlags::CONSTANT_BUFFER);
		            data.inLightCB_ = builder.Read(updateLightsPassData.outLightCB_, GPU::BindFlags::CONSTANT_BUFFER);

		            data.outTileInfoSB_ = builder.Write(
		                builder.Create("LC Tile Info SB",
		                    RenderGraphBufferDesc(sizeof(TileInfo) * light.numTilesX_ * light.numTilesY_)),
		                GPU::BindFlags::UNORDERED_ACCESS);

		            data.tech_ = shader->CreateTechnique("TECH_COMPUTE_TILE_INFO", ShaderTechniqueDesc());

		            data.viewBindings_ = shader->CreateBindingSet("ViewBindings");
		            data.tileInfoBindings_ = shader->CreateBindingSet("TileInfoBindings");
	            },
	            [](RenderGraphResources& res, GPU::CommandList& cmdList, const ComputeTileInfoPassData& data) {
		            ShaderContext shaderCtx(cmdList);

		            data.viewBindings_.Set("viewParams", res.CBuffer(data.inViewCB_, 0, sizeof(ViewConstants)));
		            data.viewBindings_.Set("lightParams", res.CBuffer(data.inLightCB_, 0, sizeof(LightConstants)));

		            data.tileInfoBindings_.Set(
		                "outTileInfo", res.RWBuffer(data.outTileInfoSB_, GPU::Format::INVALID, 0,
		                                   data.light_.numTilesX_ * data.light_.numTilesY_, sizeof(TileInfo)));

		            /**
						* TODO: Support taking multiple bindings as parameters?
						* 
						* if(auto binds = shaderCtx.BeginBindingScope(data.viewBindings_, data.tileInfoBind))
						* {
						*	  .....
						* }
						*/
		            if(auto viewBind = shaderCtx.BeginBindingScope(data.viewBindings_))
		            {
			            if(auto tileInfoBind = shaderCtx.BeginBindingScope(data.tileInfoBindings_))
			            {
				            GPU::Handle ps;
				            Core::ArrayView<GPU::PipelineBinding> pb;
				            if(shaderCtx.CommitBindings(data.tech_, ps, pb))
					            cmdList.Dispatch(ps, pb, data.light_.numTilesX_, data.light_.numTilesY_, 1);
			            }
		            }
	            })
	        .GetData();

	auto& computeLightListsPassData =
	    renderGraph
	        .AddCallbackRenderPass<ComputeLightListsPassData>("Compute Light Lists",
	            [&](RenderGraphBuilder& builder, ComputeLightListsPassData& data) {
		            data.drawFn_ = drawFn;
		            data.light_ = light;
		            data.depthFormat_ = GPU::GetSRVFormatDepth(dsDesc.format_);

		            data.inViewCB_ = builder.Read(cbs.viewCB_, GPU::BindFlags::CONSTANT_BUFFER);
		            data.inLightCB_ = builder.Read(updateLightsPassData.outLightCB_, GPU::BindFlags::CONSTANT_BUFFER);
		            data.inLightSB_ = builder.Read(updateLightsPassData.outLightSB_, GPU::BindFlags::SHADER_RESOURCE);
		            data.inTileInfoSB_ =
		                builder.Read(computeTileInfoPassData.outTileInfoSB_, GPU::BindFlags::SHADER_RESOURCE);
		            data.inDepth_ = builder.Read(depth, GPU::BindFlags::SHADER_RESOURCE);

		            data.outLightIndex_ =
		                builder.Write(builder.Create("LC Light Link Index SB", RenderGraphBufferDesc(sizeof(u32))),
		                    GPU::BindFlags::UNORDERED_ACCESS);
		            data.outLightTex_ = builder.Write(
		                builder.Create("LC Light Tex", RenderGraphTextureDesc(GPU::TextureType::TEX2D, lightTexFormat,
		                                                   light.numTilesX_, light.numTilesY_)),
		                GPU::BindFlags::UNORDERED_ACCESS);
		            data.outLightIndicesSB_ = builder.Write(
		                builder.Create("LC Light Indices SB", RenderGraphBufferDesc(sizeof(i32) * LIGHT_BUFFER_SIZE)),
		                GPU::BindFlags::UNORDERED_ACCESS);

		            data.tech_ = shader->CreateTechnique("TECH_COMPUTE_LIGHT_LISTS", ShaderTechniqueDesc());

		            data.viewBindings_ = shader->CreateBindingSet("ViewBindings");
		            data.lightBindings_ = shader->CreateBindingSet("LightBindings");
		            data.lightListBindings_ = shader->CreateBindingSet("LightListBindings");
	            },
	            [lightTexFormat](
	                RenderGraphResources& res, GPU::CommandList& cmdList, const ComputeLightListsPassData& data) {
		            ShaderContext shaderCtx(cmdList);

		            i32 baseLightIndex = 0;
		            cmdList.UpdateBuffer(
		                res.GetBuffer(data.outLightIndex_), 0, sizeof(u32), cmdList.Push(&baseLightIndex));

		            data.viewBindings_.Set("viewParams", res.CBuffer(data.inViewCB_, 0, sizeof(ViewConstants)));
		            data.viewBindings_.Set("lightParams", res.CBuffer(data.inLightCB_, 0, sizeof(LightConstants)));

		            data.lightBindings_.Set("inLights",
		                res.Buffer(data.inLightSB_, GPU::Format::INVALID, 0, data.light_.numLights_, sizeof(Light)));

		            data.lightListBindings_.Set(
		                "inTileInfo", res.Buffer(data.inTileInfoSB_, GPU::Format::INVALID, 0,
		                                  data.light_.numTilesX_ * data.light_.numTilesY_, sizeof(TileInfo)));
		            data.lightListBindings_.Set(
		                "lightIndex", res.RWBuffer(data.outLightIndex_, GPU::Format::R32_TYPELESS, 0, sizeof(u32), 0));
		            data.lightListBindings_.Set("outLightTex", res.RWTexture2D(data.outLightTex_, lightTexFormat));
		            data.lightListBindings_.Set("outLightIndices",
		                res.RWBuffer(data.outLightIndicesSB_, GPU::Format::INVALID, 0, LIGHT_BUFFER_SIZE, sizeof(i32)));
		            data.lightListBindings_.Set("depthTex", res.Texture2D(data.inDepth_, data.depthFormat_, 0, 1));

		            if(auto viewBind = shaderCtx.BeginBindingScope(data.viewBindings_))
		            {
			            if(auto lightBind = shaderCtx.BeginBindingScope(data.lightBindings_))
			            {
				            if(auto lightListBind = shaderCtx.BeginBindingScope(data.lightListBindings_))
				            {
					            GPU::Handle ps;
					            Core::ArrayView<GPU::PipelineBinding> pb;
					            if(shaderCtx.CommitBindings(data.tech_, ps, pb))
						            cmdList.Dispatch(ps, pb, data.light_.numTilesX_, data.light_.numTilesY_, 1);
				            }
			            }
		            }
	            })
	        .GetData();

	auto& debugOutputPassData =
	    renderGraph
	        .AddCallbackRenderPass<DebugOutputPassData>("Debug Light Output",
	            [&](RenderGraphBuilder& builder, DebugOutputPassData& data) {
		            data.drawFn_ = drawFn;
		            data.light_ = light;

		            data.inViewCB_ = builder.Read(cbs.viewCB_, GPU::BindFlags::CONSTANT_BUFFER);
		            data.inLightCB_ = builder.Read(updateLightsPassData.outLightCB_, GPU::BindFlags::CONSTANT_BUFFER);
		            data.inLightSB_ = builder.Read(updateLightsPassData.outLightSB_, GPU::BindFlags::SHADER_RESOURCE);
		            data.inTileInfoSB_ =
		                builder.Read(computeTileInfoPassData.outTileInfoSB_, GPU::BindFlags::SHADER_RESOURCE);
		            data.inLightTex_ =
		                builder.Read(computeLightListsPassData.outLightTex_, GPU::BindFlags::SHADER_RESOURCE);
		            data.inLightIndicesSB_ =
		                builder.Read(computeLightListsPassData.outLightIndicesSB_, GPU::BindFlags::SHADER_RESOURCE);

		            data.outDebug_ =
		                builder.Write(builder.Create("LC Debug Tile Info",
		                                  RenderGraphTextureDesc(GPU::TextureType::TEX2D,
		                                      GPU::Format::R32G32B32A32_FLOAT, light.numTilesX_, light.numTilesY_)),
		                    GPU::BindFlags::UNORDERED_ACCESS);

		            data.tech_ = shader->CreateTechnique("TECH_DEBUG_TILE_INFO", ShaderTechniqueDesc());

		            data.viewBindings_ = shader->CreateBindingSet("ViewBindings");
		            data.lightListBindings_ = shader->CreateBindingSet("LightListBindings");
		            data.debugBindings_ = shader->CreateBindingSet("DebugBindings");
	            },
	            [lightTexFormat](
	                RenderGraphResources& res, GPU::CommandList& cmdList, const DebugOutputPassData& data) {
		            ShaderContext shaderCtx(cmdList);

		            data.viewBindings_.Set("viewParams", res.CBuffer(data.inViewCB_, 0, sizeof(ViewConstants)));
		            data.viewBindings_.Set("lightParams", res.CBuffer(data.inLightCB_, 0, sizeof(LightConstants)));

		            data.lightListBindings_.Set(
		                "inTileInfo", res.Buffer(data.inTileInfoSB_, GPU::Format::INVALID, 0,
		                                  data.light_.numTilesX_ * data.light_.numTilesY_, sizeof(TileInfo)));
		            data.lightListBindings_.Set("inLights",
		                res.Buffer(data.inLightSB_, GPU::Format::INVALID, 0, data.light_.numLights_, sizeof(Light)));
		            data.lightListBindings_.Set("inLightTex", res.Texture2D(data.inLightTex_, lightTexFormat, 0, 1));
		            data.lightListBindings_.Set("inLightIndices",
		                res.Buffer(data.inLightIndicesSB_, GPU::Format::INVALID, 0, LIGHT_BUFFER_SIZE, sizeof(i32)));

		            data.debugBindings_.Set(
		                "outDebug", res.RWTexture2D(data.outDebug_, GPU::Format::R32G32B32A32_FLOAT));

		            if(auto viewBind = shaderCtx.BeginBindingScope(data.viewBindings_))
		            {
			            if(auto lightListBind = shaderCtx.BeginBindingScope(data.lightListBindings_))
			            {
				            if(auto debugBind = shaderCtx.BeginBindingScope(data.debugBindings_))
				            {
					            GPU::Handle ps;
					            Core::ArrayView<GPU::PipelineBinding> pb;
					            if(shaderCtx.CommitBindings(data.tech_, ps, pb))
						            cmdList.Dispatch(ps, pb, data.light_.numTilesX_, data.light_.numTilesY_, 1);
				            }
			            }
		            }
	            })
	        .GetData();

	// Setup output.
	LightCullingData output;
	output.lightConstants_ = light;
	output.outLightCB_ = updateLightsPassData.outLightCB_;
	output.outLightSB_ = updateLightsPassData.outLightSB_;
	output.outLightTex_ = computeLightListsPassData.outLightTex_;
	output.outLightIndicesSB_ = computeLightListsPassData.outLightIndicesSB_;
	output.outDebug_ = debugOutputPassData.outDebug_;
	return output;
}

struct DepthData
{
	RenderGraphResource outDepth_;
	RenderGraphResource outHiZ_;
	RenderGraphResource outObjectSB_;

	GPU::FrameBindingSetDesc fbsDesc_;
};

static DepthData AddDepthPasses(DrawFn drawFn, RenderGraph& renderGraph, const CommonBuffers& cbs,
    const RenderGraphTextureDesc& depthDesc, Shader* shader, RenderGraphResource depth, RenderGraphResource objectSB)
{
	struct DepthPassData : BaseDrawFnData
	{
		GPU::DrawState drawState_;

		RenderGraphResource inViewCB_;
		RenderGraphResource inLightCB_;

		RenderGraphResource outDepth_;
		RenderGraphResource outObjectSB_;

		mutable ShaderBindingSet viewBindings_;
	};

	struct HiZPassData
	{
		RenderGraphResource inDepth_;
		RenderGraphResource outDepth_;

		GPU::Format depthFormat_;
		Graphics::RenderGraphTextureDesc hizDesc_;

		ShaderTechnique tech_;
		ShaderTechnique techMip_;

		mutable ShaderBindingSet hizBindings_;
	};

	auto& depthPass = renderGraph.AddCallbackRenderPass<DepthPassData>("Depth Pass",
	    [&](RenderGraphBuilder& builder, DepthPassData& data) {
		    data.drawFn_ = drawFn;
		    data.drawState_.scissorRect_.w_ = depthDesc.width_;
		    data.drawState_.scissorRect_.h_ = depthDesc.height_;
		    data.drawState_.viewport_.w_ = (f32)depthDesc.width_;
		    data.drawState_.viewport_.h_ = (f32)depthDesc.height_;

		    // Create depth target if none are provided.
		    if(!depth)
			    depth = builder.Create("Depth", depthDesc);

		    data.inViewCB_ = builder.Read(cbs.viewCB_, GPU::BindFlags::CONSTANT_BUFFER);
		    data.inLightCB_ = builder.Write(builder.Create("LC LightCB", RenderGraphBufferDesc(sizeof(LightConstants))),
		        GPU::BindFlags::CONSTANT_BUFFER);

		    // Object buffer.
		    DBG_ASSERT(objectSB);
		    data.outObjectSB_ = builder.Write(objectSB, GPU::BindFlags::SHADER_RESOURCE);

		    // Setup frame buffer.
		    data.outDepth_ = builder.SetDSV(depth);

		    data.viewBindings_ = Shader::CreateSharedBindingSet("ViewBindings");
	    },

	    [](RenderGraphResources& res, GPU::CommandList& cmdList, const DepthPassData& data) {

		    auto fbs = res.GetFrameBindingSet();
		    ShaderContext shaderCtx(cmdList);

		    LightConstants light;
		    cmdList.UpdateBuffer(res.GetBuffer(data.inLightCB_), 0, sizeof(light), &light);

		    data.viewBindings_.Set("viewParams", res.CBuffer(data.inViewCB_, 0, sizeof(ViewConstants)));
		    data.viewBindings_.Set("lightParams", res.CBuffer(data.inLightCB_, 0, sizeof(LightConstants)));

		    // Clear depth buffer.
		    cmdList.ClearDSV(fbs, 1.0f, 0);

		    // Draw all render packets that are valid for this pass.
		    if(auto viewBind = shaderCtx.BeginBindingScope(data.viewBindings_))
		    {
			    DrawContext drawCtx(cmdList, shaderCtx, "RenderPassDepthPrepass", data.drawState_, fbs,
			        res.GetBuffer(data.inViewCB_), res.GetBuffer(data.outObjectSB_), nullptr);

			    data.drawFn_(drawCtx);
		    }
	    });

	auto& hizPass = renderGraph.AddCallbackRenderPass<HiZPassData>("Hi-Z Pass",
	    [&](RenderGraphBuilder& builder, HiZPassData& data) {

		    data.inDepth_ = builder.Read(depthPass.GetData().outDepth_, GPU::BindFlags::SHADER_RESOURCE);

		    data.depthFormat_ = GPU::GetSRVFormatDepth(depthDesc.format_);

		    data.hizDesc_ = depthDesc;
		    data.hizDesc_.format_ = GPU::Format::R32G32_FLOAT;
		    data.hizDesc_.width_ /= 2;
		    data.hizDesc_.height_ /= 2;
		    i32 w = data.hizDesc_.width_;
		    i32 h = data.hizDesc_.height_;
		    data.hizDesc_.levels_ = 0;
		    while(w > 0 || h > 0)
		    {
			    w /= 2;
			    h /= 2;
			    data.hizDesc_.levels_++;
		    }

		    auto hiz = builder.Create("Hi-Z Texture", data.hizDesc_);

		    data.outDepth_ = builder.Write(hiz, GPU::BindFlags::UNORDERED_ACCESS);
		    data.tech_ = shader->CreateTechnique("TECH_COMPUTE_HIZ", ShaderTechniqueDesc());
		    data.techMip_ = shader->CreateTechnique("TECH_COMPUTE_HIZ_MIP", ShaderTechniqueDesc());

		    data.hizBindings_ = shader->CreateBindingSet("HiZBindings");
	    },

	    [](RenderGraphResources& res, GPU::CommandList& cmdList, const HiZPassData& data) {
		    ShaderContext shaderCtx(cmdList);

		    data.hizBindings_.Set("inHiZ", res.Texture2D(data.inDepth_, data.depthFormat_, 0, 1));
		    data.hizBindings_.Set("outHiZ", res.RWTexture2D(data.outDepth_, GPU::Format::R32G32_FLOAT, 0));

		    const i32 tileSize = 8;
		    auto GetGroups = [tileSize](i32 x) { return Core::PotRoundUp(x, tileSize) / tileSize; };

		    i32 w = data.hizDesc_.width_;
		    i32 h = data.hizDesc_.height_;
		    if(auto hizBind = shaderCtx.BeginBindingScope(data.hizBindings_))
		    {
			    GPU::Handle ps;
			    Core::ArrayView<GPU::PipelineBinding> pb;
			    if(shaderCtx.CommitBindings(data.tech_, ps, pb))
				    cmdList.Dispatch(ps, pb, GetGroups(w), GetGroups(h), 1);
		    }

		    for(i32 idx = 1; idx < data.hizDesc_.levels_; ++idx)
		    {
			    w = Core::Max(1, w / 2);
			    h = Core::Max(1, h / 2);
			    data.hizBindings_.Set("inHiZ", res.Texture2D(data.outDepth_, GPU::Format::R32G32_FLOAT, idx - 1, 1));
			    data.hizBindings_.Set("outHiZ", res.RWTexture2D(data.outDepth_, GPU::Format::R32G32_FLOAT, idx));

			    if(auto hizBind = shaderCtx.BeginBindingScope(data.hizBindings_))
			    {
				    GPU::Handle ps;
				    Core::ArrayView<GPU::PipelineBinding> pb;
				    if(shaderCtx.CommitBindings(data.tech_, ps, pb))
					    cmdList.Dispatch(ps, pb, GetGroups(w), GetGroups(h), 1);
			    }
		    }
	    });

	DepthData output;
	output.outDepth_ = depthPass.GetData().outDepth_;
	output.outObjectSB_ = depthPass.GetData().outObjectSB_;
	output.outHiZ_ = hizPass.GetData().outDepth_;
	output.fbsDesc_ = depthPass.GetFrameBindingDesc();
	return output;
}

struct ForwardData
{
	RenderGraphResource outColor_;
	RenderGraphResource outDepth_;
	RenderGraphResource outObjectSB_;

	GPU::FrameBindingSetDesc fbsDesc_;
};

static ForwardData AddForwardPasses(DrawFn drawFn, RenderGraph& renderGraph, const CommonBuffers& cbs,
    const LightCullingData& lightCulling, const RenderGraphTextureDesc& colorDesc, RenderGraphResource color,
    const RenderGraphTextureDesc& depthDesc, RenderGraphResource depth, RenderGraphResource hiz,
    RenderGraphResource objectSB)
{
	struct ForwardPassData : BaseDrawFnData
	{
		GPU::DrawState drawState_;
		i32 numLights_;

		RenderGraphResource inViewCB_;
		RenderGraphResource inLightCB_;
		RenderGraphResource inLightSB_;
		RenderGraphResource inLightTex_;
		RenderGraphResource inLightIndicesSB_;

		RenderGraphResource outColor_;
		RenderGraphResource outDepth_;
		RenderGraphResource outObjectSB_;

		mutable ShaderBindingSet viewBindings_;
		mutable ShaderBindingSet lightBindings_;
		mutable ShaderBindingSet lightTileBindings_;
	};

	auto& pass = renderGraph.AddCallbackRenderPass<ForwardPassData>("Forward Pass",
	    [&](RenderGraphBuilder& builder, ForwardPassData& data) {
		    data.drawFn_ = drawFn;
		    data.drawState_.scissorRect_.w_ = colorDesc.width_;
		    data.drawState_.scissorRect_.h_ = colorDesc.height_;
		    data.drawState_.viewport_.w_ = (f32)colorDesc.width_;
		    data.drawState_.viewport_.h_ = (f32)colorDesc.height_;

		    data.numLights_ = lightCulling.lightConstants_.numLights_;

		    // Create color & depth targets if none are provided.
		    if(!color)
			    color = builder.Create("Color", colorDesc);
		    if(!depth)
			    depth = builder.Create("Depth", depthDesc);

		    data.inViewCB_ = builder.Read(cbs.viewCB_, GPU::BindFlags::CONSTANT_BUFFER);
		    data.inLightCB_ = builder.Read(lightCulling.outLightCB_, GPU::BindFlags::CONSTANT_BUFFER);
		    data.inLightSB_ = builder.Read(lightCulling.outLightSB_, GPU::BindFlags::SHADER_RESOURCE);
		    data.inLightTex_ = builder.Read(lightCulling.outLightTex_, GPU::BindFlags::SHADER_RESOURCE);
		    data.inLightIndicesSB_ = builder.Read(lightCulling.outLightIndicesSB_, GPU::BindFlags::SHADER_RESOURCE);

		    builder.Read(hiz, GPU::BindFlags::SHADER_RESOURCE);

		    // Object buffer.
		    DBG_ASSERT(objectSB);
		    data.outObjectSB_ = builder.Write(objectSB, GPU::BindFlags::SHADER_RESOURCE);

		    // Create binding sets.
		    data.viewBindings_ = Shader::CreateSharedBindingSet("ViewBindings");
		    data.lightBindings_ = Shader::CreateSharedBindingSet("LightBindings");
		    data.lightTileBindings_ = Shader::CreateSharedBindingSet("LightTileBindings");

		    // Setup frame buffer.
		    data.outColor_ = builder.SetRTV(0, color);
		    data.outDepth_ = builder.SetDSV(depth);
	    },
	    [](RenderGraphResources& res, GPU::CommandList& cmdList, const ForwardPassData& data) {
		    auto fbs = res.GetFrameBindingSet();

		    ShaderContext shaderCtx(cmdList);

		    // Clear color buffer.
		    f32 color[] = {0.1f, 0.1f, 0.2f, 1.0f};
		    cmdList.ClearRTV(fbs, 0, color);

		    // Draw all render packets.
		    RenderGraphTextureDesc lightTexDesc;
		    res.GetTexture(data.inLightTex_, &lightTexDesc);

		    data.viewBindings_.Set("viewParams", res.CBuffer(data.inViewCB_, 0, sizeof(ViewConstants)));
		    data.viewBindings_.Set("lightParams", res.CBuffer(data.inLightCB_, 0, sizeof(LightConstants)));

		    data.lightBindings_.Set(
		        "inLights", res.Buffer(data.inLightSB_, GPU::Format::INVALID, 0, data.numLights_, sizeof(Light)));

		    data.lightTileBindings_.Set("inLightTex", res.Texture2D(data.inLightTex_, lightTexDesc.format_, 0, 1));
		    data.lightTileBindings_.Set("inLightIndices",
		        res.Buffer(data.inLightIndicesSB_, GPU::Format::INVALID, 0, LIGHT_BUFFER_SIZE, sizeof(i32)));

		    if(auto viewBind = shaderCtx.BeginBindingScope(data.viewBindings_))
		    {
			    if(auto lightBind = shaderCtx.BeginBindingScope(data.lightBindings_))
			    {
				    if(auto lightTileBind = shaderCtx.BeginBindingScope(data.lightTileBindings_))
				    {
					    DrawContext drawCtx(cmdList, shaderCtx, "RenderPassForward", data.drawState_, fbs,
					        res.GetBuffer(data.inViewCB_), res.GetBuffer(data.outObjectSB_), nullptr);

					    data.drawFn_(drawCtx);
				    }
			    }
		    }
	    });

	ForwardData output;
	output.outColor_ = pass.GetData().outColor_;
	output.outDepth_ = pass.GetData().outDepth_;
	output.outObjectSB_ = pass.GetData().outObjectSB_;
	output.fbsDesc_ = pass.GetFrameBindingDesc();
	return output;
}

struct FullscreenData
{
	RenderGraphResource outColor_;

	GPU::FrameBindingSetDesc fbsDesc_;
};

using FullscreenSetupFn = Core::Function<void(RenderGraphBuilder&)>;
using FullscreenBindFn = Core::Function<void(RenderGraphResources&, Shader*, ShaderTechnique&)>;

static FullscreenData AddFullscreenPass(DrawFn drawFn, RenderGraph& renderGraph, const CommonBuffers& cbs,
    RenderGraphResource color, Shader* shader, FullscreenSetupFn setupFn, FullscreenBindFn bindFn)
{
	struct FullscreenPassData : BaseDrawFnData
	{
		FullscreenBindFn bindFn_;
		Shader* shader_;
		GPU::DrawState drawState_;

		RenderGraphResource inViewCB_;

		RenderGraphResource outColor_;
	};

	auto& pass = renderGraph.AddCallbackRenderPass<FullscreenPassData>("Fullscreen Pass",
	    [&](RenderGraphBuilder& builder, FullscreenPassData& data) {
		    data.bindFn_ = bindFn;
		    data.shader_ = shader;

		    setupFn(builder);

		    RenderGraphTextureDesc colorDesc;
		    builder.GetTexture(color, &colorDesc);
		    data.drawState_.scissorRect_.w_ = colorDesc.width_;
		    data.drawState_.scissorRect_.h_ = colorDesc.height_;
		    data.drawState_.viewport_.w_ = (f32)colorDesc.width_;
		    data.drawState_.viewport_.h_ = (f32)colorDesc.height_;

		    data.inViewCB_ = builder.Read(cbs.viewCB_, GPU::BindFlags::CONSTANT_BUFFER);

		    // Setup frame buffer.
		    data.outColor_ = builder.SetRTV(0, color);

	    },

	    [](RenderGraphResources& res, GPU::CommandList& cmdList, const FullscreenPassData& data) {
		    GPU::FrameBindingSetDesc fbsDesc;
		    auto fbs = res.GetFrameBindingSet(&fbsDesc);

		    ShaderContext shaderCtx(cmdList);

		    ShaderTechniqueDesc techDesc;
		    techDesc.SetFrameBindingSet(fbsDesc);
		    techDesc.SetTopology(GPU::TopologyType::TRIANGLE);
		    auto tech = data.shader_->CreateTechnique("TECH_FULLSCREEN", techDesc);

		    //data.bindFn_(res, data.shader_, tech);
		    {
			    GPU::Handle ps;
			    Core::ArrayView<GPU::PipelineBinding> pb;
			    if(shaderCtx.CommitBindings(tech, ps, pb))
				    cmdList.Draw(ps, pb, GPU::Handle(), fbs, data.drawState_, GPU::PrimitiveTopology::TRIANGLE_LIST, 0,
				        0, 3, 0, 1);
		    }
	    });

	FullscreenData output;
	output.outColor_ = pass.GetData().outColor_;
	output.fbsDesc_ = pass.GetFrameBindingDesc();
	return output;
}

static const char* FORWARD_RESOURCE_NAMES[] = {
    "in_color", "in_depth", "in_shadow_map", "out_color", "out_depth", nullptr};

ForwardPipeline::ForwardPipeline()
    : Pipeline(FORWARD_RESOURCE_NAMES)
{
	Resource::Manager::RequestResource(shader_, "shaders/forward_pipeline.esf");
	Resource::Manager::WaitForResource(shader_);

	ShaderTechniqueDesc desc;
}

ForwardPipeline::~ForwardPipeline()
{
	Resource::Manager::ReleaseResource(shader_);
}

void ForwardPipeline::CreateTechniques(Material* material, ShaderTechniqueDesc desc, ShaderTechniques& outTechniques)
{
	auto AddTechnique = [&](const char* name) {
		auto techIt = fbsDescs_.find(name);
		if(techIt != nullptr)
		{
			desc.SetFrameBindingSet(*techIt);
		}

		i32 size = outTechniques.passIndices_.size();
		auto idxIt = outTechniques.passIndices_.find(name);
		if(idxIt != nullptr)
		{
			if(!outTechniques.passTechniques_[*idxIt])
				outTechniques.passTechniques_[*idxIt] = material->CreateTechnique(name, desc);
		}
		else
		{
			outTechniques.passTechniques_.emplace_back(material->CreateTechnique(name, desc));
			outTechniques.passIndices_.insert(name, size);
		}
	};

	AddTechnique("RenderPassDepthPrepass");
	AddTechnique("RenderPassForward");
}

void ForwardPipeline::SetCamera(
    const Math::Mat44& view, const Math::Mat44& proj, Math::Vec2 screenDimensions, bool updateFrustum)
{
	view_.view_ = view;
	view_.proj_ = proj;
	view_.viewProj_ = view * proj;
	view_.invView_ = view;
	view_.invView_.Inverse();
	view_.invProj_ = proj;
	view_.invProj_.Inverse();
	view_.screenDimensions_ = screenDimensions;

	if(updateFrustum)
		view_.CalculateFrustum();
}

void ForwardPipeline::SetDrawCallback(DrawFn drawFn)
{
	drawFn_ = drawFn;
}

void ForwardPipeline::Setup(RenderGraph& renderGraph)
{
	i32 w = (i32)view_.screenDimensions_.x;
	i32 h = (i32)view_.screenDimensions_.y;

	struct ViewConstantData
	{
		ViewConstants view_;
		CommonBuffers cbs_;
	};

	auto& renderPassCommonBuffers = renderGraph.AddCallbackRenderPass<ViewConstantData>("Setup Common Buffers",
	    [&](RenderGraphBuilder& builder, ViewConstantData& data) {
		    data.view_ = view_;
		    data.cbs_.viewCB_ =
		        builder.Write(builder.Create("View Constants", viewCBDesc), GPU::BindFlags::CONSTANT_BUFFER);
		    data.cbs_.objectSB_ =
		        builder.Write(builder.Create("Object Constants", objectSBDesc), GPU::BindFlags::SHADER_RESOURCE);
	    },
	    [](RenderGraphResources& res, GPU::CommandList& cmdList, const ViewConstantData& data) {
		    cmdList.UpdateBuffer(res.GetBuffer(data.cbs_.viewCB_), 0, sizeof(data.view_), cmdList.Push(&data.view_));
	    });

	auto cbs = renderPassCommonBuffers.GetData().cbs_;

	auto renderPassDepth =
	    AddDepthPasses(drawFn_, renderGraph, cbs, GetDepthTextureDesc(w, h), shader_, resources_[1], cbs.objectSB_);
	fbsDescs_.insert("RenderPassDepthPrepass", renderPassDepth.fbsDesc_);

	auto lightCulling = AddLightCullingPasses(drawFn_, renderGraph, cbs, renderPassDepth.outDepth_, shader_,
	    Core::ArrayView<Light>(lights_.data(), lights_.size()));

	if(debugMode_ == DebugMode::LIGHT_CULLING)
	{
		RenderGraphResource debugTex = debugTex = lightCulling.outDebug_;
		AddFullscreenPass(drawFn_, renderGraph, cbs, resources_[0], shader_,
		    [&](RenderGraphBuilder& builder) { debugTex = builder.Read(debugTex, GPU::BindFlags::SHADER_RESOURCE); },
		    /*[debugTex](RenderGraphResources& res, Shader* shader, ShaderTechnique& tech) {
				tech.Set("debugTex", res.Texture2D(debugTex, GPU::Format::INVALID, 0, -1));
			}*/ nullptr);
	}
	else
	{
		auto renderPassForward = AddForwardPasses(drawFn_, renderGraph, cbs, lightCulling, GetDefaultTextureDesc(w, h),
		    resources_[0], GetDepthTextureDesc(w, h), renderPassDepth.outDepth_, renderPassDepth.outHiZ_,
		    renderPassDepth.outObjectSB_);

		SetResource("out_color", renderPassForward.outColor_);
		SetResource("out_depth", renderPassForward.outDepth_);
		fbsDescs_.insert("RenderPassForward", renderPassForward.fbsDesc_);
	}
}
