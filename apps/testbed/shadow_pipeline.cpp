#include "shadow_pipeline.h"
#include "common.h"

#include "gpu/command_list.h"
#include "gpu/resources.h"
#include "gpu/utils.h"
#include "graphics/render_graph.h"
#include "graphics/render_pass.h"
#include "resource/manager.h"

namespace Testbed
{
	using namespace Graphics;

	const RenderGraphBufferDesc viewCBDesc = RenderGraphBufferDesc(sizeof(ViewConstants));
	const RenderGraphBufferDesc objectSBDesc = RenderGraphBufferDesc(sizeof(ObjectConstants) * 1000);

	struct BaseDrawFnData
	{
		DrawFn drawFn_;
	};

	struct CommonBuffers
	{
		RenderGraphResource viewCB_;
		RenderGraphResource objectSB_;
	};

	struct ShadowSettings
	{
		/// Object buffer to use.
		RenderGraphResource objectSB_;

		/// Shadow map settings.
		i32 width_ = 1024;
		i32 height_ = 1024;
		i16 elements_ = 1;
		GPU::Format format_ = GPU::Format::R32_TYPELESS;

		/// View settings.
		ViewConstants view_;

		/// Common buffers.
		CommonBuffers cbs_;

		/// Output shadow map to render to.
		RenderGraphResource outShadowMap_;
		/// Index of element to render into.
		i32 idx_ = 0;
	};

	struct ShadowData
	{
		RenderGraphResource outShadowMap_;
		RenderGraphResource outObjectSB_;

		GPU::FrameBindingSetDesc fbsDesc_;
	};

	static ShadowData AddShadowPass(DrawFn drawFn, RenderGraph& renderGraph, ShadowSettings settings)
	{
		struct ShadowPassData : BaseDrawFnData
		{
			GPU::DrawState drawState_;

			ViewConstants view_;
			RenderGraphResource viewCB_;

			RenderGraphResource outShadowMap_;
			RenderGraphResource outObjectSB_;
		};

		auto& renderPassShadowMap = renderGraph.AddCallbackRenderPass<ShadowPassData>("Shadow Map Pass",
		    [&](RenderGraphBuilder& builder, ShadowPassData& data) {
			    data.drawFn_ = drawFn;
			    data.drawState_.scissorRect_.w_ = settings.width_;
			    data.drawState_.scissorRect_.h_ = settings.height_;
			    data.drawState_.viewport_.w_ = (f32)settings.width_;
			    data.drawState_.viewport_.h_ = (f32)settings.height_;

			    // Create shadow map if none are provided.
			    if(!settings.outShadowMap_)
				{
					RenderGraphTextureDesc desc;
					desc.type_ = GPU::TextureType::TEX2D;
					desc.width_ = settings.width_;
					desc.height_ = settings.height_;
					desc.elements_ = settings.elements_;
					desc.format_ = settings.format_;
				    settings.outShadowMap_ = builder.Create("Shadow Map", desc);
				}


				// Setup frame buffer.
			    data.outShadowMap_ = builder.SetDSV(settings.outShadowMap_);
			},

		    [](RenderGraphResources& res, GPU::CommandList& cmdList, const ShadowPassData& data) {
			    auto fbs = res.GetFrameBindingSet();

			    // Clear depth buffer.
			    cmdList.ClearDSV(fbs, 1.0f, 0);

			    // Draw all render packets that are valid for this pass.
			    data.drawFn_(cmdList, "RenderPassShadow", data.drawState_, fbs, res.GetBuffer(data.viewCB_),
			        res.GetBuffer(data.outObjectSB_), nullptr);
			});

		ShadowData output;
		output.outShadowMap_ = renderPassShadowMap.GetData().outShadowMap_;
		output.outObjectSB_ = renderPassShadowMap.GetData().outObjectSB_;
		output.fbsDesc_ = renderPassShadowMap.GetFrameBindingDesc();
		return output;
	}

	struct FullscreenData
	{
		RenderGraphResource outColor_;

		GPU::FrameBindingSetDesc fbsDesc_;
	};

	using FullscreenSetupFn = Core::Function<void(RenderGraphBuilder&)>;
	using FullscreenBindFn = Core::Function<void(RenderGraphResources&, Shader*, ShaderTechnique&)>;

#if 0
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

			    ShaderTechniqueDesc techDesc;
			    techDesc.SetFrameBindingSet(fbsDesc);
			    techDesc.SetTopology(GPU::TopologyType::TRIANGLE);
			    auto tech = data.shader_->CreateTechnique("TECH_FULLSCREEN", techDesc);

			    data.bindFn_(res, data.shader_, tech);
			    if(auto binding = tech.GetBinding())
			    {
				    cmdList.Draw(binding, GPU::Handle(), fbs, data.drawState_, GPU::PrimitiveTopology::TRIANGLE_LIST, 0,
				        0, 3, 0, 1);
			    }
			});

		FullscreenData output;
		output.outColor_ = pass.GetData().outColor_;
		output.fbsDesc_ = pass.GetFrameBindingDesc();
		return output;
	}
#endif

	static const char* SHADOW_RESOURCE_NAMES[] = {"in_depth", "out_shadow_map", nullptr};

	ShadowPipeline::ShadowPipeline()
	    : Pipeline(SHADOW_RESOURCE_NAMES)
	{
		Resource::Manager::RequestResource(shader_, "shader_tests/shadow_pipeline.esf");
		Resource::Manager::WaitForResource(shader_);

		ShaderTechniqueDesc desc;
	}

	ShadowPipeline::~ShadowPipeline()
	{
		Resource::Manager::ReleaseResource(shader_);
	}

	void ShadowPipeline::CreateTechniques(Shader* shader, ShaderTechniqueDesc desc, ShaderTechniques& outTechniques)
	{
		auto AddTechnique = [&](const char* name) {
			auto techIt = fbsDescs_.find(name);
			if(techIt != fbsDescs_.end())
			{
				desc.SetFrameBindingSet(techIt->second);
			}

			i32 size = outTechniques.passIndices_.size();
			auto idxIt = outTechniques.passIndices_.find(name);
			if(idxIt != outTechniques.passIndices_.end())
			{
				if(!outTechniques.passTechniques_[idxIt->second])
					outTechniques.passTechniques_[idxIt->second] = shader->CreateTechnique(name, desc);
			}
			else
			{
				outTechniques.passTechniques_.emplace_back(shader->CreateTechnique(name, desc));
				outTechniques.passIndices_.insert(name, size);
			}
		};

		AddTechnique("RenderPassShadow");
	}

	void ShadowPipeline::SetDirectionalLight(Math::Vec3 eyePos, Light light)
	{
		eyePos_ = eyePos;
		directionalLight_ = light;
		Math::Mat44 view;
	}

	void ShadowPipeline::SetDrawCallback(DrawFn drawFn) { drawFn_ = drawFn; }

	void ShadowPipeline::Setup(RenderGraph& renderGraph)
	{
		struct ViewConstantData
		{
			ViewConstants view_;
			CommonBuffers cbs_;
		};

		// Setup shadow settings.
		ShadowSettings settings;

		Math::Mat44 view;
		view.LookAt(directionalLight_.position_, eyePos_, Math::Vec3(0.0f, 1.0f, 0.0f));

		Math::Mat44 proj;
		proj.OrthoProjection(-1000.0f, 1000.0f, 1000.0f, -1000.0f, 0.0f, 10000.0f);

		view_.view_ = view;
		view_.proj_ = proj;
		view_.viewProj_ = view * proj;
		view_.invView_ = view;
		view_.invView_.Inverse();
		view_.invProj_ = proj;
		view_.invProj_.Inverse();
		view_.screenDimensions_ = Math::Vec2((f32)settings.width_, (f32)settings.height_);
		settings.view_ = view_;

		auto& renderPassCommonBuffers = renderGraph.AddCallbackRenderPass<ViewConstantData>("Setup Common Buffers",
		    [&](RenderGraphBuilder& builder, ViewConstantData& data) {
			    data.view_ = view_;
			    data.cbs_.viewCB_ =
			        builder.Write(builder.Create("View Constants", objectSBDesc), GPU::BindFlags::CONSTANT_BUFFER);
			    data.cbs_.objectSB_ =
			        builder.Write(builder.Create("Object Constants", objectSBDesc), GPU::BindFlags::SHADER_RESOURCE);
			},
		    [](RenderGraphResources& res, GPU::CommandList& cmdList, const ViewConstantData& data) {
			    cmdList.UpdateBuffer(
			        res.GetBuffer(data.cbs_.viewCB_), 0, sizeof(data.view_), cmdList.Push(&data.view_));
			});

		auto cbs = renderPassCommonBuffers.GetData().cbs_;

		auto renderPassShadow = AddShadowPass(drawFn_, renderGraph, settings);
		fbsDescs_.insert("RenderPassShadow", renderPassShadow.fbsDesc_);

		SetResource("out_shadow_map", renderPassShadow.outShadowMap_);

#if 0
		RenderGraphResource debugTex = debugTex = renderPassShadow.outShadowMap_;
		AddFullscreenPass(drawFn_, renderGraph, cbs, resources_[0], shader_,
			[&](RenderGraphBuilder& builder) {
				debugTex = builder.Read(debugTex, GPU::BindFlags::SHADER_RESOURCE);
			},
			[debugTex](RenderGraphResources& res, Shader* shader, ShaderTechnique& tech) {
				tech.Set("debugTex", res.Texture2D(debugTex, GPU::Format::INVALID, 0, -1));
			});
#endif
	}

} // namespace Testbed
