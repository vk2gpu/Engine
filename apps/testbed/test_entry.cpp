#include "client/manager.h"
#include "core/file.h"
#include "graphics/pipeline.h"
#include "graphics/render_graph.h"
#include "graphics/render_pass.h"
#include "graphics/render_resources.h"
#include "imgui/manager.h"
#include "test_shared.h"

namespace
{
	class RenderPassImGui : public Graphics::RenderPass
	{
	public:
		RenderPassImGui(Graphics::RenderGraphBuilder& builder, Graphics::RenderGraphResource rt)
			: Graphics::RenderPass(builder)
		{
			rt_ = builder.UseRTV(this, rt);
		};

		virtual ~RenderPassImGui()
		{
			/// TODO: Should be managed by frame graph.
			GPU::Manager::DestroyResource(fbs_);
		}

		void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
		{
			Graphics::RenderGraphTextureDesc rtTexDesc;
			auto rtTex = res.GetTexture(rt_, &rtTexDesc);
			GPU::FrameBindingSetDesc fbsDesc;
			fbsDesc.rtvs_[0].resource_ = rtTex;
			fbsDesc.rtvs_[0].format_ = rtTexDesc.format_;
			fbsDesc.rtvs_[0].dimension_ = GPU::ViewDimension::TEX2D;
			fbs_ = GPU::Manager::CreateFrameBindingSet(fbsDesc, "RenderPassImGui");

			ImGui::Manager::EndFrame(fbs_, cmdList);
		}

		Graphics::RenderGraphResource rt_;
		GPU::Handle fbs_;
	};

	class ImGuiPipeline : public Graphics::IPipeline
	{
	public:
		ImGuiPipeline() {}

		virtual ~ImGuiPipeline() {}

		Core::Array<Graphics::RenderGraphResource, 2> resources_;

		Core::ArrayView<const char*> GetResourceNames() const override
		{
			static Core::Array<const char*, 2> resourceNames = {
				"in_color", "out_color",
			};
			return Core::ArrayView<const char*>(resourceNames.data(), resourceNames.size());
		}

		i32 GetResourceIdx(const char* name) const override
		{
			const auto& resourceNames = GetResourceNames();
			for(i32 idx = 0; idx < resourceNames.size(); ++idx)
			{
				if(strcmp(resourceNames[idx], name) == 0)
					return idx;
			}
			return -1;
		}

		void SetResource(i32 idx, Graphics::RenderGraphResource res) override
		{
			if(idx >= 0 && idx < resources_.size())
				resources_[idx] = res;
		}

		Graphics::RenderGraphResource GetResource(i32 idx) const override
		{
			if(idx >= 0 && idx < resources_.size())
				return resources_[idx];
			return Graphics::RenderGraphResource();
		}

		void Setup(Graphics::RenderGraph& renderGraph) override
		{
			auto& renderPassImGui = renderGraph.AddRenderPass<RenderPassImGui>("ImGui", resources_[0]);
			resources_[1] = renderPassImGui.rt_;
		}

		bool HaveExecuteErrors() const override { return false; }
	};

}

void loop()
{
	ScopedEngine engine("Test Bed App");
	ImGui::Manager::Scoped imgui;
	ImGuiPipeline imguiPipeline;
	Graphics::RenderGraph graph;

	// Scan for plugins and get number.
	i32 found = Plugin::Manager::Scan(".");
	DBG_ASSERT(found > 0);

	// Grab all plugins of the appropriate type.
	Core::Vector<Graphics::PipelinePlugin> pipelinePlugins(found);
	found = Plugin::Manager::GetPlugins(pipelinePlugins.data(), pipelinePlugins.size());
	pipelinePlugins.resize(found);

	// Select "Graphics.PipelineTest"
	Graphics::PipelinePlugin* selectedPlugin = nullptr;
	for(auto& plugin : pipelinePlugins)
	{
		Core::Log("Found: %s - %s\n", plugin.name_, plugin.desc_);
		if(strcmp(plugin.name_, "Graphics.PipelineTest") == 0)
			selectedPlugin = &plugin;
	}
	DBG_ASSERT(selectedPlugin);

	const Client::IInputProvider& input = engine.window.GetInputProvider();

	i32 w, h;
	engine.window.GetSize(w, h);

	Graphics::IPipeline* pipeline = nullptr;

	while(Client::Manager::Update())
	{
		Graphics::RenderGraphResource scRes;
		Graphics::RenderGraphResource dsRes;

		ImGui::Manager::BeginFrame(input, w, h);

		// Clear graph prior to beginning work.
		graph.Clear();

		// If plugin has changed, reload.
		if(Plugin::Manager::HasChanged(*selectedPlugin))
		{
			// Destroy pipeline ready for reload.
			selectedPlugin->DestroyPipeline(pipeline);

			Core::Log("Plugin changed, reloading...");
			Plugin::Manager::Reload(*selectedPlugin);
			Core::Log("reloaded!");
		}

		// Create pipeline if there isn't one.
		if(pipeline == nullptr)
		{
			pipeline = selectedPlugin->CreatePipeline();
		}
		// Import back buffer.
		auto bbRes = graph.ImportResource("Back Buffer", engine.scHandle);

		// Set color target to back buffer.
		pipeline->SetResource(pipeline->GetResourceIdx("in_color"), bbRes);

		// Have the pipeline setup on the graph.
		pipeline->Setup(graph);

		// Setup ImGui pipeline.
		imguiPipeline.SetResource(
		    imguiPipeline.GetResourceIdx("in_color"), pipeline->GetResource(pipeline->GetResourceIdx("out_color")));
		imguiPipeline.Setup(graph);

		// Execute, and resolve the out color target.
		graph.Execute(imguiPipeline.GetResource(imguiPipeline.GetResourceIdx("out_color")));

		// Require no errors.
		DBG_ASSERT(pipeline->HaveExecuteErrors() == false);

		// Present, next frame, wait.
		GPU::Manager::PresentSwapChain(engine.scHandle);
		GPU::Manager::NextFrame();
		Core::Sleep(1.0 / 60.0);
		Resource::Manager::WaitOnReload();
	}
}

int main(int argc, char* const argv[])
{
	Client::Manager::Scoped clientManager;

	// Change to executable path.
	char path[Core::MAX_PATH_LENGTH];
	if(Core::FileSplitPath(argv[0], path, Core::MAX_PATH_LENGTH, nullptr, 0, nullptr, 0))
	{
		Core::FileChangeDir(path);
	}

	loop();

	return 0;
}
