#include "graphics/pipeline.h"
#include "graphics/render_graph.h"
#include "graphics/render_pass.h"
#include "core/string.h"
#include "core/vector.h"
#include "gpu/command_list.h"
#include "gpu/manager.h"

#include <array>
#include <cstring>
#include <utility>

namespace
{
	Graphics::RenderGraphTextureDesc GetDefaultTextureDesc()
	{
		Graphics::RenderGraphTextureDesc desc;
		desc.type_ = GPU::TextureType::TEX2D;
		desc.width_ = 1280;
		desc.height_ = 720;
		desc.format_ = GPU::Format::R8G8B8A8_UNORM;
		return desc;
	}

	Graphics::RenderGraphTextureDesc GetDepthTextureDesc()
	{
		Graphics::RenderGraphTextureDesc desc;
		desc.type_ = GPU::TextureType::TEX2D;
		desc.width_ = 1280;
		desc.height_ = 720;
		desc.format_ = GPU::Format::D24_UNORM_S8_UINT;
		return desc;
	}

	Graphics::RenderGraphTextureDesc GetSSAOTextureDesc()
	{
		Graphics::RenderGraphTextureDesc desc;
		desc.type_ = GPU::TextureType::TEX2D;
		desc.width_ = 1280;
		desc.height_ = 720;
		desc.format_ = GPU::Format::R16_FLOAT;
		return desc;
	}

	Graphics::RenderGraphTextureDesc GetHDRTextureDesc()
	{
		Graphics::RenderGraphTextureDesc desc;
		desc.type_ = GPU::TextureType::TEX2D;
		desc.width_ = 1280;
		desc.height_ = 720;
		desc.format_ = GPU::Format::R16G16B16A16_FLOAT;
		return desc;
	}

	struct DebugData
	{
		Core::Vector<Core::String> passes_;
	};

	class RenderPassMain : public Graphics::RenderPass
	{
	public:
		RenderPassMain(Graphics::RenderGraphBuilder& builder, DebugData& debugData,
		    Graphics::RenderGraphResource inColor = Graphics::RenderGraphResource(),
		    Graphics::RenderGraphResource inDepth = Graphics::RenderGraphResource())
		    : Graphics::RenderPass(builder)
		    , debugData_(debugData)
		{
			if(!inColor)
				inColor = builder.CreateTexture("Color", GetDefaultTextureDesc());
			color_ = builder.UseRTV(this, inColor);

			if(!inDepth)
				inDepth = builder.CreateTexture("Depth", GetDepthTextureDesc());
			depth_ = builder.UseDSV(this, inDepth, GPU::DSVFlags::NONE);
		}

		virtual ~RenderPassMain() { GPU::Manager::DestroyResource(fbs_); }

		void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
		{
			debugData_.passes_.push_back("RenderPassMain");

			Graphics::RenderGraphTextureDesc rtTexDesc;
			Graphics::RenderGraphTextureDesc dsTexDesc;
			auto rtTex = color_ ? res.GetTexture(color_, &rtTexDesc) : GPU::Handle();
			auto dsTex = depth_ ? res.GetTexture(depth_, &dsTexDesc) : GPU::Handle();
			GPU::FrameBindingSetDesc fbsDesc;
			fbsDesc.rtvs_[0].resource_ = rtTex;
			fbsDesc.rtvs_[0].format_ = rtTexDesc.format_;
			fbsDesc.rtvs_[0].dimension_ = GPU::ViewDimension::TEX2D;
			fbsDesc.dsv_.resource_ = dsTex;
			fbsDesc.dsv_.format_ = dsTexDesc.format_;
			fbsDesc.dsv_.dimension_ = GPU::ViewDimension::TEX2D;
			fbs_ = GPU::Manager::CreateFrameBindingSet(fbsDesc, "RenderPassMain");

			f32 color[] = {0.1f, 0.1f, 0.2f, 1.0f};
			cmdList.ClearRTV(fbs_, 0, color);
		}

		DebugData& debugData_;

		Graphics::RenderGraphResource color_;
		Graphics::RenderGraphResource depth_;

		GPU::Handle fbs_;
	};


	class RenderPassHUD : public Graphics::RenderPass
	{
	public:
		RenderPassHUD(
		    Graphics::RenderGraphBuilder& builder, DebugData& debugData, Graphics::RenderGraphResource inColor)
		    : Graphics::RenderPass(builder)
		    , debugData_(debugData)
		{
			if(!inColor)
				inColor = builder.CreateTexture("Color", GetDefaultTextureDesc());
			color_ = builder.UseRTV(this, inColor);
		}

		virtual ~RenderPassHUD() {}
		void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
		{
			debugData_.passes_.push_back("RenderPassHUD");
		}

		DebugData& debugData_;

		Graphics::RenderGraphResource color_;
	};


	class RenderPassFinal : public Graphics::RenderPass
	{
	public:
		RenderPassFinal(
		    Graphics::RenderGraphBuilder& builder, DebugData& debugData, Graphics::RenderGraphResource inColor)
		    : Graphics::RenderPass(builder)
		    , debugData_(debugData)
		{
			if(!inColor)
				inColor = builder.CreateTexture("Color", GetDefaultTextureDesc());
			color_ = builder.UseRTV(this, inColor);
		}

		virtual ~RenderPassFinal() {}
		void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
		{
			debugData_.passes_.push_back("RenderPassFinal");
		}

		DebugData& debugData_;

		Graphics::RenderGraphResource color_;
	};

	class RenderPassDepthPrepass : public Graphics::RenderPass
	{
	public:
		RenderPassDepthPrepass(
		    Graphics::RenderGraphBuilder& builder, DebugData& debugData, Graphics::RenderGraphResource inDepth)
		    : Graphics::RenderPass(builder)
		    , debugData_(debugData)
		{
			if(!inDepth)
				inDepth = builder.CreateTexture("Depth", GetDepthTextureDesc());
			depth_ = builder.UseDSV(this, inDepth, GPU::DSVFlags::NONE);
		}

		virtual ~RenderPassDepthPrepass() {}
		void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
		{
			debugData_.passes_.push_back("RenderPassDepthPrepass");
		}

		DebugData& debugData_;

		Graphics::RenderGraphResource depth_;
	};


	class RenderPassSolid : public Graphics::RenderPass
	{
	public:
		RenderPassSolid(
		    Graphics::RenderGraphBuilder& builder, DebugData& debugData, Graphics::RenderGraphResource inDepth)
		    : Graphics::RenderPass(builder)
		    , debugData_(debugData)
		{
			if(!inDepth)
				inDepth = builder.CreateTexture("Depth", GetDepthTextureDesc());
			depth_ = builder.UseDSV(this, inDepth, GPU::DSVFlags::READ_ONLY_DEPTH | GPU::DSVFlags::READ_ONLY_STENCIL);
			albedo_ = builder.UseRTV(this, builder.CreateTexture("Albedo", GetDefaultTextureDesc()));
			material_ = builder.UseRTV(this, builder.CreateTexture("Material", GetDefaultTextureDesc()));
			normal_ = builder.UseRTV(this, builder.CreateTexture("Normal", GetDefaultTextureDesc()));
		}

		virtual ~RenderPassSolid() {}
		void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
		{
			debugData_.passes_.push_back("RenderPassSolid");
		}

		DebugData& debugData_;

		Graphics::RenderGraphResource depth_;

		Graphics::RenderGraphResource albedo_;
		Graphics::RenderGraphResource material_;
		Graphics::RenderGraphResource normal_;
	};

	class RenderPassSSAO : public Graphics::RenderPass
	{
	public:
		RenderPassSSAO(
		    Graphics::RenderGraphBuilder& builder, DebugData& debugData, Graphics::RenderGraphResource inDepth)
		    : Graphics::RenderPass(builder)
		    , debugData_(debugData)
		{
			depth_ = builder.UseSRV(this, inDepth);
			ssao_ = builder.UseRTV(this, builder.CreateTexture("SSAO", GetSSAOTextureDesc()));
		};

		virtual ~RenderPassSSAO() {}
		void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
		{
			debugData_.passes_.push_back("RenderPassSSAO");
		}

		DebugData& debugData_;

		Graphics::RenderGraphResource depth_;

		Graphics::RenderGraphResource ssao_;
	};

	class RenderPassLighting : public Graphics::RenderPass
	{
	public:
		RenderPassLighting(Graphics::RenderGraphBuilder& builder, DebugData& debugData,
		    Graphics::RenderGraphResource inDepth, Graphics::RenderGraphResource inAlbedo,
		    Graphics::RenderGraphResource inMaterial, Graphics::RenderGraphResource inNormal,
		    Graphics::RenderGraphResource inSSAO)
		    : Graphics::RenderPass(builder)
		    , debugData_(debugData)
		{
			depth_ = builder.UseSRV(this, inDepth);
			albedo_ = builder.UseSRV(this, inAlbedo);
			material_ = builder.UseSRV(this, inMaterial);
			normal_ = builder.UseSRV(this, inNormal);
			ssao_ = builder.UseSRV(this, inSSAO);

			hdr_ = builder.UseRTV(this, builder.CreateTexture("HDR", GetHDRTextureDesc()));
		}

		virtual ~RenderPassLighting() {}
		void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
		{
			debugData_.passes_.push_back("RenderPassLighting");
		}

		DebugData& debugData_;

		Graphics::RenderGraphResource depth_;
		Graphics::RenderGraphResource albedo_;
		Graphics::RenderGraphResource material_;
		Graphics::RenderGraphResource normal_;
		Graphics::RenderGraphResource ssao_;

		Graphics::RenderGraphResource hdr_;
	};

	class RenderPassToneMap : public Graphics::RenderPass
	{
	public:
		RenderPassToneMap(Graphics::RenderGraphBuilder& builder, DebugData& debugData,
		    Graphics::RenderGraphResource inHDR, Graphics::RenderGraphResource inoutColor)
		    : Graphics::RenderPass(builder)
		    , debugData_(debugData)
		{
			hdr_ = builder.UseSRV(this, inHDR);
			color_ = builder.UseRTV(this, inoutColor);
		}

		virtual ~RenderPassToneMap() { GPU::Manager::DestroyResource(fbs_); }

		void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
		{
			debugData_.passes_.push_back("RenderPassToneMap");

			Graphics::RenderGraphTextureDesc rtTexDesc;
			auto rtTex = color_ ? res.GetTexture(color_, &rtTexDesc) : GPU::Handle();
			GPU::FrameBindingSetDesc fbsDesc;
			fbsDesc.rtvs_[0].resource_ = rtTex;
			fbsDesc.rtvs_[0].format_ = rtTexDesc.format_;
			fbsDesc.rtvs_[0].dimension_ = GPU::ViewDimension::TEX2D;
			fbs_ = GPU::Manager::CreateFrameBindingSet(fbsDesc, "RenderPassToneMap");

			f32 color[] = {0.1f, 0.1f, 0.2f, 1.0f};
			cmdList.ClearRTV(fbs_, 0, color);
		}

		DebugData& debugData_;

		Graphics::RenderGraphResource hdr_;
		Graphics::RenderGraphResource color_;

		GPU::Handle fbs_;
	};

	void CreateForward(Graphics::RenderGraph& graph, DebugData& debugData, Graphics::RenderGraphResource& inoutColor,
	    Graphics::RenderGraphResource& inoutDepth)
	{
		auto& renderPassMain = graph.AddRenderPass<RenderPassMain>("Main", debugData, inoutColor, inoutDepth);
		auto& renderPassHUD = graph.AddRenderPass<RenderPassHUD>("HUD", debugData, renderPassMain.color_);
		auto& renderPassFinal = graph.AddRenderPass<RenderPassFinal>("Final", debugData, renderPassHUD.color_);

		inoutDepth = renderPassMain.depth_;
		inoutColor = renderPassFinal.color_;
	}

	void CreateDeferred(Graphics::RenderGraph& graph, DebugData& debugData, Graphics::RenderGraphResource& inoutColor,
	    Graphics::RenderGraphResource& inoutDepth)
	{
		auto& renderPassDepthPrepass =
		    graph.AddRenderPass<RenderPassDepthPrepass>("Depth Prepass", debugData, inoutDepth);
		auto& renderPassSolid = graph.AddRenderPass<RenderPassSolid>("Solid", debugData, renderPassDepthPrepass.depth_);
		auto& renderPassSSAO = graph.AddRenderPass<RenderPassSSAO>("SSAO", debugData, renderPassDepthPrepass.depth_);
		auto& renderPassLighting =
		    graph.AddRenderPass<RenderPassLighting>("Lighting", debugData, renderPassSolid.depth_,
		        renderPassSolid.albedo_, renderPassSolid.material_, renderPassSolid.normal_, renderPassSSAO.ssao_);
		auto& renderPassToneMap =
		    graph.AddRenderPass<RenderPassToneMap>("Tone Map", debugData, renderPassLighting.hdr_, inoutColor);

		inoutDepth = renderPassDepthPrepass.depth_;
		inoutColor = renderPassToneMap.color_;
	}

	static const char* RESOURCE_NAMES[] = {"in_color", "out_color", "out_depth"};

	class PipelineTest : public Graphics::Pipeline
	{
	public:
		PipelineTest()
		    : Graphics::Pipeline(RESOURCE_NAMES)
		{
		}

		virtual ~PipelineTest() {}

		void Setup(Graphics::RenderGraph& renderGraph) override
		{

			debugData_ = DebugData();

			// Reset depth.
			resources_[2] = Graphics::RenderGraphResource();
			// Assign input color to output.
			resources_[1] = resources_[0];

			if(renderer_ == 0)
				CreateForward(renderGraph, debugData_, resources_[1], resources_[2]);
			else if(renderer_ == 1)
				CreateDeferred(renderGraph, debugData_, resources_[1], resources_[2]);
		}

		bool HaveExecuteErrors() const override
		{
			bool haveErrors = false;
			if(renderer_ == 0)
			{
				haveErrors |= debugData_.passes_.size() != 3;
				if(haveErrors)
					return haveErrors;
				haveErrors |= debugData_.passes_[0] != "RenderPassMain";
				haveErrors |= debugData_.passes_[1] != "RenderPassHUD";
				haveErrors |= debugData_.passes_[2] != "RenderPassFinal";
			}
			else if(renderer_ == 1)
			{
				haveErrors |= debugData_.passes_.size() != 5;
				if(haveErrors)
					return haveErrors;
				haveErrors |= debugData_.passes_[0] != "RenderPassDepthPrepass";
				haveErrors |= debugData_.passes_[1] != "RenderPassSSAO";
				haveErrors |= debugData_.passes_[2] != "RenderPassSolid";
				haveErrors |= debugData_.passes_[3] != "RenderPassLighting";
				haveErrors |= debugData_.passes_[4] != "RenderPassToneMap";
			}

			return haveErrors;
		}

		DebugData debugData_;
		int renderer_ = 0;
	};
}


extern "C" {
EXPORT bool GetPlugin(struct Plugin::Plugin* outPlugin, Core::UUID uuid)
{
	bool retVal = false;

	// Fill in base info.
	if(uuid == Plugin::Plugin::GetUUID() || uuid == Graphics::PipelinePlugin::GetUUID())
	{
		if(outPlugin)
		{
			outPlugin->systemVersion_ = Plugin::PLUGIN_SYSTEM_VERSION;
			outPlugin->pluginVersion_ = Graphics::PipelinePlugin::PLUGIN_VERSION;
			outPlugin->uuid_ = Graphics::PipelinePlugin::GetUUID();
			outPlugin->name_ = "Graphics.PipelineTest";
			outPlugin->desc_ = "Test graphics pipeline.";
		}
		retVal = true;
	}

	// Fill in plugin specific.
	if(uuid == Graphics::PipelinePlugin::GetUUID())
	{
		if(outPlugin)
		{
			auto* plugin = static_cast<Graphics::PipelinePlugin*>(outPlugin);
			plugin->CreatePipeline = []() -> Graphics::IPipeline* { return new PipelineTest(); };
			plugin->DestroyPipeline = [](Graphics::IPipeline*& pipeline) {
				delete pipeline;
				pipeline = nullptr;
			};
		}
		retVal = true;
	}

	return retVal;
}
}
