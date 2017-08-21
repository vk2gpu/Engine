#include "catch.hpp"
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

	class RenderPassMain : public Graphics::RenderPass
	{
	public:
		RenderPassMain(Graphics::RenderGraphBuilder& builder)
		    : Graphics::RenderPass(builder)
		{
			output_ = builder.UseRTV(this, builder.CreateTexture("Main", GetDefaultTextureDesc()));
		}

		virtual ~RenderPassMain() {}
		void Execute(Graphics::RenderGraph& rg, GPU::CommandList& cmdList) override
		{
			// Do scene rendering.
			Core::Log("RenderPassMain::Execute\n");
		}

		Graphics::RenderGraphResource output_;
	};


	class RenderPassHUD : public Graphics::RenderPass
	{
	public:
		RenderPassHUD(Graphics::RenderGraphBuilder& builder, Graphics::RenderGraphResource input)
		    : Graphics::RenderPass(builder)
		{
			input_ = builder.UseSRV(this, input);
			output_ = builder.UseRTV(this, builder.CreateTexture("FrameBuffer", GetDefaultTextureDesc()));
		}

		virtual ~RenderPassHUD() {}
		void Execute(Graphics::RenderGraph& rg, GPU::CommandList& cmdList) override
		{
			// Do HUD rendering.
			Core::Log("RenderPassHUD::Execute\n");
		}

		Graphics::RenderGraphResource input_;
		Graphics::RenderGraphResource output_;
	};


	class RenderPassFinal : public Graphics::RenderPass
	{
	public:
		RenderPassFinal(Graphics::RenderGraphBuilder& builder, Graphics::RenderGraphResource inputA,
		    Graphics::RenderGraphResource inputB)
		    : Graphics::RenderPass(builder)
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
		}

		Graphics::RenderGraphResource inputA_;
		Graphics::RenderGraphResource inputB_;
		Graphics::RenderGraphResource output_;
	};

	class RenderPassDepthPrepass : public Graphics::RenderPass
	{
	public:
		RenderPassDepthPrepass(Graphics::RenderGraphBuilder& builder)
		    : Graphics::RenderPass(builder)
		{
			depth_ = builder.UseDSV(this, builder.CreateTexture("Main", GetDefaultTextureDesc()), GPU::DSVFlags::NONE);
		}

		virtual ~RenderPassDepthPrepass() {}
		void Execute(Graphics::RenderGraph& rg, GPU::CommandList& cmdList) override
		{
			// Do scene rendering.
			Core::Log("RenderPassDepthPrepass::Execute\n");
		}

		Graphics::RenderGraphResource depth_;
	};


	class RenderPassSolid : public Graphics::RenderPass
	{
	public:
		RenderPassSolid(Graphics::RenderGraphBuilder& builder, Graphics::RenderGraphResource depth)
		    : Graphics::RenderPass(builder)
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
		}

		Graphics::RenderGraphResource depth_;
		Graphics::RenderGraphResource albedo_;
		Graphics::RenderGraphResource material_;
		Graphics::RenderGraphResource normal_;
	};


	class RenderPassSSAO : public Graphics::RenderPass
	{
	public:
		RenderPassSSAO(Graphics::RenderGraphBuilder& builder, Graphics::RenderGraphResource depth)
		    : Graphics::RenderPass(builder)
		{
			depth_ = builder.UseSRV(this, depth);

			ssao_ = builder.UseRTV(this, builder.CreateTexture("SSAO", GetSSAOTextureDesc()));
		};

		virtual ~RenderPassSSAO() {}
		void Execute(Graphics::RenderGraph& rg, GPU::CommandList& cmdList) override
		{
			// Do scene rendering.
			Core::Log("RenderPassSSAO::Execute\n");
		}

		Graphics::RenderGraphResource depth_;
		Graphics::RenderGraphResource ssao_;
	};


	class RenderPassLighting : public Graphics::RenderPass
	{
	public:
		RenderPassLighting(Graphics::RenderGraphBuilder& builder, Graphics::RenderGraphResource depth,
		    Graphics::RenderGraphResource albedo, Graphics::RenderGraphResource material,
		    Graphics::RenderGraphResource normal, Graphics::RenderGraphResource ssao)
		    : Graphics::RenderPass(builder)
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
		}

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

	auto& renderPassMain = graph.AddRenderPass<RenderPassMain>("Main");
	(void)renderPassMain;

	graph.Execute(renderPassMain.output_);
}


TEST_CASE("render-graph-tests-forward-advanced")
{
	ScopedEngine engine;
	Graphics::RenderGraph graph;

	auto& renderPassMain = graph.AddRenderPass<RenderPassMain>("Main");
	(void)renderPassMain;

	auto& renderPassHUD = graph.AddRenderPass<RenderPassHUD>("HUD", renderPassMain.output_);
	(void)renderPassHUD;


	auto& renderPassFinal =
	    graph.AddRenderPass<RenderPassFinal>("Final", renderPassMain.output_, renderPassHUD.output_);
	(void)renderPassFinal;

	graph.Execute(renderPassFinal.output_);
}


TEST_CASE("render-graph-tests-deferred-simple")
{
	ScopedEngine engine;
	Graphics::RenderGraph graph;

	auto& renderPassDepthPrepass = graph.AddRenderPass<RenderPassDepthPrepass>("Depth Prepass");
	(void)renderPassDepthPrepass;

	auto& renderPassSolid = graph.AddRenderPass<RenderPassSolid>("Solid", renderPassDepthPrepass.depth_);
	(void)renderPassSolid;

	auto& renderPassSSAO = graph.AddRenderPass<RenderPassSSAO>("SSAO", renderPassDepthPrepass.depth_);
	(void)renderPassSSAO;

	auto& renderPassLighting = graph.AddRenderPass<RenderPassLighting>("Lighting", renderPassSolid.depth_,
	    renderPassSolid.albedo_, renderPassSolid.material_, renderPassSolid.normal_, renderPassSSAO.ssao_);
	(void)renderPassLighting;

	graph.Execute(renderPassLighting.hdr_);
}
