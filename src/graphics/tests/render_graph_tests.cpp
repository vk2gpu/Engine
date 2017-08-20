#include "catch.hpp"
#include "graphics/render_graph.h"
#include "graphics/render_pass.h"
#include "graphics/render_resources.h"

TEST_CASE("render-graph-tests-simple-forward")
{
	Graphics::RenderGraph graph;

	class RenderPassMain : public Graphics::RenderPass
	{
	public:
		RenderPassMain(Graphics::RenderGraphBuilder& builder)
		    : Graphics::RenderPass(builder)
		{
			Graphics::RenderGraphTextureDesc desc;
			desc.type_ = GPU::TextureType::TEX2D;
			desc.width_ = 1280;
			desc.height_ = 720;
			desc.format_ = GPU::Format::R8G8B8A8_UNORM;

			output_ = builder.UseRTV(builder.CreateTexture("Main", desc));
		}

		virtual ~RenderPassMain() {}
		void Execute(Graphics::RenderGraph& rg, GPU::CommandList& cmdList) override
		{
			// Do scene rendering.
			Core::Log("RenderPassMain::Execute\n");
		}

		Graphics::RenderGraphResource output_;
	};

	auto& renderPassMain = graph.AddRenderPass<RenderPassMain>("Main");
	(void)renderPassMain;

	class RenderPassHUD : public Graphics::RenderPass
	{
	public:
		RenderPassHUD(Graphics::RenderGraphBuilder& builder, Graphics::RenderGraphResource input)
		    : Graphics::RenderPass(builder)
		{
			input_ = builder.UseSRV(input);

			Graphics::RenderGraphTextureDesc desc;
			desc.type_ = GPU::TextureType::TEX2D;
			desc.width_ = 1280;
			desc.height_ = 720;
			desc.format_ = GPU::Format::R8G8B8A8_UNORM;

			output_ = builder.UseRTV(builder.CreateTexture("FrameBuffer", desc));
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

	auto& renderPassHUD = graph.AddRenderPass<RenderPassHUD>("HUD", renderPassMain.output_);
	(void)renderPassHUD;
}
