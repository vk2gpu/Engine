#include "imgui_pipeline.h"
#include "imgui/manager.h"

#include "graphics/render_graph.h"
#include "graphics/render_pass.h"

using namespace Graphics;

static const char* IMGUI_RESOURCE_NAMES[] = {"in_color", "out_color", nullptr};

ImGuiPipeline::ImGuiPipeline()
    : Pipeline(IMGUI_RESOURCE_NAMES)
{
}

void ImGuiPipeline::Setup(RenderGraph& renderGraph)
{
	struct ImGuiPassData
	{
		RenderGraphResource outColor_;
	};

	auto& pass = renderGraph.AddCallbackRenderPass<ImGuiPassData>("ImGui Pass",
	    [&](RenderGraphBuilder& builder, ImGuiPassData& data) { data.outColor_ = builder.SetRTV(0, resources_[0]); },

	    [](RenderGraphResources& res, GPU::CommandList& cmdList, const ImGuiPassData& data) {
		    auto fbs = res.GetFrameBindingSet();
		    ImGui::Manager::Render(fbs, cmdList);
		});

	resources_[1] = pass.GetData().outColor_;
}
