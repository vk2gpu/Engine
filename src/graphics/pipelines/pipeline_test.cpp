#include "graphics/pipeline.h"
#include "graphics/render_graph.h"
#include "graphics/render_pass.h"
#include "core/concurrency.h"
#include "core/set.h"
#include "core/string.h"
#include "core/vector.h"
#include "gpu/command_list.h"
#include "gpu/manager.h"

#include "core/allocator_overrides.h"

DECLARE_MODULE_ALLOCATOR("General/" MODULE_NAME);

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
		Core::Mutex mutex_;
		Core::Set<Core::String> passes_;

		void AddPass(const char* name)
		{
			Core::ScopedMutex lock(mutex_);
			passes_.insert(name);
		}

		bool HavePass(const char* name) const { return passes_.find(name) != nullptr; }
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
				inColor = builder.Create("Color", GetDefaultTextureDesc());
			color_ = builder.SetRTV(0, inColor);

			if(!inDepth)
				inDepth = builder.Create("Depth", GetDepthTextureDesc());
			depth_ = builder.SetDSV(inDepth);
		}

		virtual ~RenderPassMain() { GPU::Manager::DestroyResource(fbs_); }

		void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
		{
			debugData_.AddPass("RenderPassMain");

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
				inColor = builder.Create("Color", GetDefaultTextureDesc());
			color_ = builder.SetRTV(0, inColor);
		}

		virtual ~RenderPassHUD() {}
		void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
		{
			debugData_.AddPass("RenderPassHUD");
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
				inColor = builder.Create("Color", GetDefaultTextureDesc());
			color_ = builder.SetRTV(0, inColor);
		}

		virtual ~RenderPassFinal() {}
		void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
		{
			debugData_.AddPass("RenderPassFinal");
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
				inDepth = builder.Create("Depth", GetDepthTextureDesc());
			depth_ = builder.SetDSV(inDepth);
		}

		virtual ~RenderPassDepthPrepass() {}
		void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
		{
			debugData_.AddPass("RenderPassDepthPrepass");
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
				inDepth = builder.Create("Depth", GetDepthTextureDesc());
			GPU::BindingDSV bindingDSV;
			bindingDSV.flags_ = GPU::DSVFlags::READ_ONLY_DEPTH | GPU::DSVFlags::READ_ONLY_STENCIL;
			depth_ = builder.SetDSV(inDepth, bindingDSV);
			albedo_ = builder.SetRTV(0, builder.Create("Albedo", GetDefaultTextureDesc()));
			material_ = builder.SetRTV(1, builder.Create("Material", GetDefaultTextureDesc()));
			normal_ = builder.SetRTV(2, builder.Create("Normal", GetDefaultTextureDesc()));
		}

		virtual ~RenderPassSolid() {}
		void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
		{
			debugData_.AddPass("RenderPassSolid");
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
			depth_ = builder.Read(inDepth, GPU::BindFlags::SHADER_RESOURCE);

			ssao_ = builder.SetRTV(0, builder.Create("SSAO", GetSSAOTextureDesc()));
		};

		virtual ~RenderPassSSAO() {}
		void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
		{
			debugData_.AddPass("RenderPassSSAO");
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
			depth_ = builder.Read(inDepth, GPU::BindFlags::SHADER_RESOURCE);
			albedo_ = builder.Read(inAlbedo, GPU::BindFlags::SHADER_RESOURCE);
			material_ = builder.Read(inMaterial, GPU::BindFlags::SHADER_RESOURCE);
			normal_ = builder.Read(inNormal, GPU::BindFlags::SHADER_RESOURCE);
			ssao_ = builder.Read(inSSAO, GPU::BindFlags::SHADER_RESOURCE);

			hdr_ = builder.SetRTV(0, builder.Create("HDR", GetHDRTextureDesc()));
		}

		virtual ~RenderPassLighting() {}
		void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
		{
			debugData_.AddPass("RenderPassLighting");
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
			hdr_ = builder.Read(inHDR, GPU::BindFlags::SHADER_RESOURCE);

			color_ = builder.SetRTV(0, inoutColor);
		}

		virtual ~RenderPassToneMap() { GPU::Manager::DestroyResource(fbs_); }

		void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
		{
			debugData_.AddPass("RenderPassToneMap");

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

	static const char* RESOURCE_NAMES[] = {"in_color", "out_color", "out_depth", nullptr};

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
				haveErrors |= !debugData_.HavePass("RenderPassMain");
				haveErrors |= !debugData_.HavePass("RenderPassHUD");
				haveErrors |= !debugData_.HavePass("RenderPassFinal");
			}
			else if(renderer_ == 1)
			{
				haveErrors |= debugData_.passes_.size() != 5;
				if(haveErrors)
					return haveErrors;
				haveErrors |= !debugData_.HavePass("RenderPassDepthPrepass");
				haveErrors |= !debugData_.HavePass("RenderPassSSAO");
				haveErrors |= !debugData_.HavePass("RenderPassSolid");
				haveErrors |= !debugData_.HavePass("RenderPassLighting");
				haveErrors |= !debugData_.HavePass("RenderPassToneMap");
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
