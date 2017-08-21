#include "catch.hpp"
#include "core/string.h"
#include "graphics/render_graph.h"
#include "graphics/render_pass.h"
#include "graphics/render_resources.h"
#include "test_shared.h"

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
		RenderPassMain(Graphics::RenderGraphBuilder& builder, DebugData& debugData)
		    : Graphics::RenderPass(builder)
		    , debugData_(debugData)
		{
			output_ = builder.UseRTV(this, builder.CreateTexture("Main", GetDefaultTextureDesc()));
		}

		virtual ~RenderPassMain() {}
		void Execute(Graphics::RenderGraph& rg, GPU::CommandList& cmdList) override
		{
			// Do scene rendering.
			Core::Log("RenderPassMain::Execute\n");
			debugData_.passes_.push_back("RenderPassMain");
		}

		DebugData& debugData_;

		Graphics::RenderGraphResource output_;
	};


	class RenderPassHUD : public Graphics::RenderPass
	{
	public:
		RenderPassHUD(Graphics::RenderGraphBuilder& builder, DebugData& debugData, Graphics::RenderGraphResource input)
		    : Graphics::RenderPass(builder)
		    , debugData_(debugData)
		{
			input_ = builder.UseSRV(this, input);
			output_ = builder.UseRTV(this, builder.CreateTexture("FrameBuffer", GetDefaultTextureDesc()));
		}

		virtual ~RenderPassHUD() {}
		void Execute(Graphics::RenderGraph& rg, GPU::CommandList& cmdList) override
		{
			// Do HUD rendering.
			Core::Log("RenderPassHUD::Execute\n");
			debugData_.passes_.push_back("RenderPassHUD");
		}

		DebugData& debugData_;

		Graphics::RenderGraphResource input_;

		Graphics::RenderGraphResource output_;
	};


	class RenderPassFinal : public Graphics::RenderPass
	{
	public:
		RenderPassFinal(Graphics::RenderGraphBuilder& builder, DebugData& debugData,
		    Graphics::RenderGraphResource inputA, Graphics::RenderGraphResource inputB)
		    : Graphics::RenderPass(builder)
		    , debugData_(debugData)
		{
			inputA_ = builder.UseSRV(this, inputA);
			inputB_ = builder.UseSRV(this, inputB);
			output_ = builder.UseRTV(this, builder.CreateTexture("FrameBuffer", GetDefaultTextureDesc()));
		}

		virtual ~RenderPassFinal() {}
		void Execute(Graphics::RenderGraph& rg, GPU::CommandList& cmdList) override
		{
			// Do HUD rendering.
			Core::Log("RenderPassFinal::Execute\n");
			debugData_.passes_.push_back("RenderPassFinal");
		}

		DebugData& debugData_;

		Graphics::RenderGraphResource inputA_;
		Graphics::RenderGraphResource inputB_;

		Graphics::RenderGraphResource output_;
	};

	class RenderPassDepthPrepass : public Graphics::RenderPass
	{
	public:
		RenderPassDepthPrepass(Graphics::RenderGraphBuilder& builder, DebugData& debugData)
		    : Graphics::RenderPass(builder)
		    , debugData_(debugData)
		{
			depth_ = builder.UseDSV(this, builder.CreateTexture("Main", GetDepthTextureDesc()), GPU::DSVFlags::NONE);
		}

		virtual ~RenderPassDepthPrepass() {}
		void Execute(Graphics::RenderGraph& rg, GPU::CommandList& cmdList) override
		{
			// Do scene rendering.
			Core::Log("RenderPassDepthPrepass::Execute\n");
			debugData_.passes_.push_back("RenderPassDepthPrepass");
		}

		DebugData& debugData_;

		Graphics::RenderGraphResource depth_;
	};


	class RenderPassSolid : public Graphics::RenderPass
	{
	public:
		RenderPassSolid(
		    Graphics::RenderGraphBuilder& builder, DebugData& debugData, Graphics::RenderGraphResource depth)
		    : Graphics::RenderPass(builder)
		    , debugData_(debugData)
		{
			depth_ = builder.UseDSV(this, depth, GPU::DSVFlags::READ_ONLY_DEPTH | GPU::DSVFlags::READ_ONLY_STENCIL);
			albedo_ = builder.UseRTV(this, builder.CreateTexture("Albedo", GetDefaultTextureDesc()));
			material_ = builder.UseRTV(this, builder.CreateTexture("Material", GetDefaultTextureDesc()));
			normal_ = builder.UseRTV(this, builder.CreateTexture("Normal", GetDefaultTextureDesc()));
		}

		virtual ~RenderPassSolid() {}
		void Execute(Graphics::RenderGraph& rg, GPU::CommandList& cmdList) override
		{
			// Do scene rendering.
			Core::Log("RenderPassSolid::Execute\n");
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
		RenderPassSSAO(Graphics::RenderGraphBuilder& builder, DebugData& debugData, Graphics::RenderGraphResource depth)
		    : Graphics::RenderPass(builder)
		    , debugData_(debugData)
		{
			depth_ = builder.UseSRV(this, depth);

			ssao_ = builder.UseRTV(this, builder.CreateTexture("SSAO", GetSSAOTextureDesc()));
		};

		virtual ~RenderPassSSAO() {}
		void Execute(Graphics::RenderGraph& rg, GPU::CommandList& cmdList) override
		{
			// Do scene rendering.
			Core::Log("RenderPassSSAO::Execute\n");
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
		    Graphics::RenderGraphResource depth, Graphics::RenderGraphResource albedo,
		    Graphics::RenderGraphResource material, Graphics::RenderGraphResource normal,
		    Graphics::RenderGraphResource ssao)
		    : Graphics::RenderPass(builder)
		    , debugData_(debugData)
		{
			depth_ = builder.UseSRV(this, depth);
			albedo_ = builder.UseSRV(this, albedo);
			material_ = builder.UseSRV(this, material);
			normal_ = builder.UseSRV(this, normal);
			ssao_ = builder.UseSRV(this, ssao);

			hdr_ = builder.UseRTV(this, builder.CreateTexture("HDR", GetHDRTextureDesc()));
		}

		virtual ~RenderPassLighting() {}
		void Execute(Graphics::RenderGraph& rg, GPU::CommandList& cmdList) override
		{
			// Do scene rendering.
			Core::Log("RenderPassLighting::Execute\n");
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
}

TEST_CASE("render-graph-tests-construct")
{
	ScopedEngine engine;
	Graphics::RenderGraph graph;
}

TEST_CASE("render-graph-tests-forward-simple")
{
	ScopedEngine engine;
	Graphics::RenderGraph graph;

	DebugData debugData;

	auto& renderPassMain = graph.AddRenderPass<RenderPassMain>("Main", debugData);
	(void)renderPassMain;

	graph.Execute(renderPassMain.output_);

	REQUIRE(debugData.passes_.size() == 1);
	REQUIRE(debugData.passes_[0] == "RenderPassMain");
}


TEST_CASE("render-graph-tests-forward-advanced")
{
	ScopedEngine engine;
	Graphics::RenderGraph graph;

	DebugData debugData;

	auto& renderPassMain = graph.AddRenderPass<RenderPassMain>("Main", debugData);
	(void)renderPassMain;

	auto& renderPassHUD = graph.AddRenderPass<RenderPassHUD>("HUD", debugData, renderPassMain.output_);
	(void)renderPassHUD;


	auto& renderPassFinal =
	    graph.AddRenderPass<RenderPassFinal>("Final", debugData, renderPassMain.output_, renderPassHUD.output_);
	(void)renderPassFinal;

	graph.Execute(renderPassFinal.output_);

	REQUIRE(debugData.passes_.size() == 3);
	REQUIRE(debugData.passes_[0] == "RenderPassMain");
	REQUIRE(debugData.passes_[1] == "RenderPassHUD");
	REQUIRE(debugData.passes_[2] == "RenderPassFinal");
}


TEST_CASE("render-graph-tests-deferred-simple")
{
	ScopedEngine engine;
	Graphics::RenderGraph graph;

	DebugData debugData;

	auto& renderPassDepthPrepass = graph.AddRenderPass<RenderPassDepthPrepass>("Depth Prepass", debugData);
	(void)renderPassDepthPrepass;

	auto& renderPassSolid = graph.AddRenderPass<RenderPassSolid>("Solid", debugData, renderPassDepthPrepass.depth_);
	(void)renderPassSolid;

	auto& renderPassSSAO = graph.AddRenderPass<RenderPassSSAO>("SSAO", debugData, renderPassDepthPrepass.depth_);
	(void)renderPassSSAO;

	auto& renderPassLighting = graph.AddRenderPass<RenderPassLighting>("Lighting", debugData, renderPassSolid.depth_,
	    renderPassSolid.albedo_, renderPassSolid.material_, renderPassSolid.normal_, renderPassSSAO.ssao_);
	(void)renderPassLighting;

	graph.Execute(renderPassLighting.hdr_);

	REQUIRE(debugData.passes_.size() == 4);
	REQUIRE(debugData.passes_[0] == "RenderPassDepthPrepass");
	REQUIRE(debugData.passes_[1] == "RenderPassSSAO");
	REQUIRE(debugData.passes_[2] == "RenderPassSolid");
	REQUIRE(debugData.passes_[3] == "RenderPassLighting");
}
