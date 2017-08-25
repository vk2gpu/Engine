#include "catch.hpp"
#include "client/manager.h"
#include "core/set.h"
#include "core/string.h"
#include "graphics/pipeline.h"
#include "graphics/render_graph.h"
#include "graphics/render_pass.h"
#include "graphics/render_resources.h"
#include "imgui/manager.h"
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
		Core::Mutex mutex_;
		Core::Set<Core::String> passes_;

		void AddPass(const char* name)
		{
			Core::ScopedMutex lock(mutex_);
			passes_.insert(name);
		}

		bool HavePass(const char* name) { return passes_.find(name) != passes_.end(); }
	};

	namespace Mock
	{
		class RenderPassMain : public Graphics::RenderPass
		{
		public:
			RenderPassMain(Graphics::RenderGraphBuilder& builder, DebugData& debugData,
			    Graphics::RenderGraphResource depth = Graphics::RenderGraphResource())
			    : Graphics::RenderPass(builder)
			    , debugData_(debugData)
			{
				color_ = builder.UseRTV(this, builder.CreateTexture("Color", GetDefaultTextureDesc()));

				if(!depth)
				{
					depth = builder.CreateTexture("Depth", GetDepthTextureDesc());
				}
				depth_ = builder.UseDSV(this, depth, GPU::DSVFlags::NONE);
			}

			virtual ~RenderPassMain() {}
			void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
			{
				debugData_.AddPass("RenderPassMain");
			}

			DebugData& debugData_;

			Graphics::RenderGraphResource color_;
			Graphics::RenderGraphResource depth_;
		};


		class RenderPassHUD : public Graphics::RenderPass
		{
		public:
			RenderPassHUD(
			    Graphics::RenderGraphBuilder& builder, DebugData& debugData, Graphics::RenderGraphResource input)
			    : Graphics::RenderPass(builder)
			    , debugData_(debugData)
			{
				input_ = builder.UseSRV(this, input);
				output_ = builder.UseRTV(this, builder.CreateTexture("FrameBuffer", GetDefaultTextureDesc()));
			}

			virtual ~RenderPassHUD() {}
			void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
			{
				debugData_.AddPass("RenderPassHUD");
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
			void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
			{
				debugData_.AddPass("RenderPassFinal");
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
				depth_ =
				    builder.UseDSV(this, builder.CreateTexture("Depth", GetDepthTextureDesc()), GPU::DSVFlags::NONE);
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
			    Graphics::RenderGraphBuilder& builder, DebugData& debugData, Graphics::RenderGraphResource depth)
			    : Graphics::RenderPass(builder)
			    , debugData_(debugData)
			{
				depth_ = builder.UseSRV(this, depth);

				ssao_ = builder.UseRTV(this, builder.CreateTexture("SSAO", GetSSAOTextureDesc()));
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

		void CreateForward(Graphics::RenderGraph& graph, DebugData& debugData, Graphics::RenderGraphResource& outColor,
		    Graphics::RenderGraphResource& outDepth)
		{
			auto& renderPassDepthPrepass =
			    graph.AddRenderPass<Mock::RenderPassDepthPrepass>("Depth Prepass", debugData);
			outDepth = renderPassDepthPrepass.depth_;

			auto& renderPassMain = graph.AddRenderPass<Mock::RenderPassMain>("Main", debugData, outDepth);
			outDepth = renderPassMain.depth_;

			auto& renderPassHUD = graph.AddRenderPass<Mock::RenderPassHUD>("HUD", debugData, renderPassMain.color_);
			auto& renderPassFinal = graph.AddRenderPass<Mock::RenderPassFinal>(
			    "Final", debugData, renderPassMain.color_, renderPassHUD.output_);

			outDepth = renderPassMain.depth_;
			outColor = renderPassFinal.output_;
		}

		void CreateDeferred(Graphics::RenderGraph& graph, DebugData& debugData, Graphics::RenderGraphResource& outHdr,
		    Graphics::RenderGraphResource& outDepth)
		{
			auto& renderPassDepthPrepass =
			    graph.AddRenderPass<Mock::RenderPassDepthPrepass>("Depth Prepass", debugData);
			auto& renderPassSolid =
			    graph.AddRenderPass<Mock::RenderPassSolid>("Solid", debugData, renderPassDepthPrepass.depth_);
			auto& renderPassSSAO =
			    graph.AddRenderPass<Mock::RenderPassSSAO>("SSAO", debugData, renderPassDepthPrepass.depth_);
			auto& renderPassLighting =
			    graph.AddRenderPass<Mock::RenderPassLighting>("Lighting", debugData, renderPassSolid.depth_,
			        renderPassSolid.albedo_, renderPassSolid.material_, renderPassSolid.normal_, renderPassSSAO.ssao_);

			outDepth = renderPassDepthPrepass.depth_;
			outHdr = renderPassLighting.hdr_;
		}

	} // namespace Mock

	namespace Draw
	{
		class RenderPassDepthPrepass : public Graphics::RenderPass
		{
		public:
			RenderPassDepthPrepass(Graphics::RenderGraphBuilder& builder)
			    : Graphics::RenderPass(builder)
			{
				ds_ = builder.UseDSV(this, builder.CreateTexture("Depth", GetDepthTextureDesc()), GPU::DSVFlags::NONE);
			};

			virtual ~RenderPassDepthPrepass()
			{
				/// TODO: Should be managed by frame graph.
				GPU::Manager::DestroyResource(fbs_);
			}

			void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
			{
				Graphics::RenderGraphTextureDesc dsTexDesc;
				auto dsTex = res.GetTexture(ds_, &dsTexDesc);
				GPU::FrameBindingSetDesc fbsDesc;
				fbsDesc.dsv_.resource_ = dsTex;
				fbsDesc.dsv_.format_ = dsTexDesc.format_;
				fbsDesc.dsv_.dimension_ = GPU::ViewDimension::TEX2D;
				fbs_ = GPU::Manager::CreateFrameBindingSet(fbsDesc, "RenderPassDepthPrepass");

				cmdList.ClearDSV(fbs_, 1.0f, 0);

				// TODO: Draw stuff.
			}

			Graphics::RenderGraphResource ds_;
			GPU::Handle fbs_;
		};

		class RenderPassForward : public Graphics::RenderPass
		{
		public:
			RenderPassForward(Graphics::RenderGraphBuilder& builder, Graphics::RenderGraphResource rt,
			    Graphics::RenderGraphResource ds)
			    : Graphics::RenderPass(builder)
			{
				rt_ = builder.UseRTV(this, rt);
				ds_ = builder.UseDSV(this, ds, GPU::DSVFlags::NONE);
			};

			virtual ~RenderPassForward()
			{
				/// TODO: Should be managed by frame graph.
				GPU::Manager::DestroyResource(fbs_);
			}

			void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
			{
				Graphics::RenderGraphTextureDesc rtTexDesc;
				Graphics::RenderGraphTextureDesc dsTexDesc;
				auto rtTex = rt_ ? res.GetTexture(rt_, &rtTexDesc) : GPU::Handle();
				auto dsTex = ds_ ? res.GetTexture(ds_, &dsTexDesc) : GPU::Handle();
				GPU::FrameBindingSetDesc fbsDesc;
				fbsDesc.rtvs_[0].resource_ = rtTex;
				fbsDesc.rtvs_[0].format_ = rtTexDesc.format_;
				fbsDesc.rtvs_[0].dimension_ = GPU::ViewDimension::TEX2D;
				fbsDesc.dsv_.resource_ = dsTex;
				fbsDesc.dsv_.format_ = dsTexDesc.format_;
				fbsDesc.dsv_.dimension_ = GPU::ViewDimension::TEX2D;
				fbs_ = GPU::Manager::CreateFrameBindingSet(fbsDesc, "RenderPassForward");

				f32 color[] = {0.1f, 0.1f, 0.2f, 1.0f};
				cmdList.ClearRTV(fbs_, 0, color);

				// TODO: Draw stuff.
			}

			Graphics::RenderGraphResource rt_;
			Graphics::RenderGraphResource ds_;
			GPU::Handle fbs_;
		};

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

			void DrawUI(Graphics::RenderGraph& renderGraph)
			{
				if(i32 numRenderPasses = renderGraph.GetNumExecutedRenderPasses())
				{
					if(ImGui::Begin("Render Passes"))
					{
						ImGui::Separator();

						Core::Vector<const Graphics::RenderPass*> renderPasses(numRenderPasses);
						Core::Vector<const char*> renderPassNames(numRenderPasses);

						renderGraph.GetExecutedRenderPasses(renderPasses.data(), renderPassNames.data());

						for(i32 idx = 0; idx < numRenderPasses; ++idx)
						{
							const auto* renderPass = renderPasses[idx];
							const char* renderPassName = renderPassNames[idx];

							auto inputs = renderPass->GetInputs();
							auto outputs = renderPass->GetOutputs();

							ImGui::Text("Render pass: %s", renderPassName);

							Core::Vector<Core::String> inputNames(inputs.size());
							Core::Vector<Core::String> outputNames(outputs.size());

							for(i32 inIdx = 0; inIdx < inputs.size(); ++inIdx)
							{
								auto res = inputs[inIdx];
								const char* resName = nullptr;
								renderGraph.GetResourceName(res, &resName);
								inputNames[inIdx].Printf("%s (v.%i)", resName, res.version_);
							}

							for(i32 outIdx = 0; outIdx < outputs.size(); ++outIdx)
							{
								auto res = outputs[outIdx];
								const char* resName = nullptr;
								renderGraph.GetResourceName(res, &resName);
								outputNames[outIdx].Printf("%s (v.%i)", resName, res.version_);
							}

							auto ResourceGetter = [](void* data, int idx, const char** out_text) -> bool {
								const auto& items = *(const Core::Vector<Core::String>*)data;
								if(out_text)
									*out_text = items[idx].c_str();
								return true;
							};

							int selectedIn = -1;
							int selectedOut = -1;
							const f32 ioWidth = ImGui::GetWindowWidth() * 0.3f;
							ImGui::PushID(idx);
							ImGui::PushItemWidth(ioWidth);
							ImGui::ListBox("Inputs", &selectedIn, ResourceGetter, &inputNames, inputs.size());
							ImGui::SameLine();
							ImGui::ListBox("Outputs", &selectedOut, ResourceGetter, &outputNames, outputs.size());
							ImGui::PopItemWidth();
							ImGui::PopID();

							ImGui::Separator();
						}
					}
					ImGui::End();
				}
			}

			void Setup(Graphics::RenderGraph& renderGraph) override
			{
				auto& renderPassImGui = renderGraph.AddRenderPass<Draw::RenderPassImGui>("ImGui", resources_[0]);
				resources_[1] = renderPassImGui.rt_;
			}

			bool HaveExecuteErrors() const override { return false; }
		};

	} // namespace Draw
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

	auto& renderPassMain = graph.AddRenderPass<Mock::RenderPassMain>("Main", debugData);
	(void)renderPassMain;

	graph.Execute(renderPassMain.color_);

	REQUIRE(debugData.passes_.size() == 1);
	REQUIRE(debugData.HavePass("RenderPassMain"));
}

TEST_CASE("render-graph-tests-forward-advanced")
{
	ScopedEngine engine;
	Graphics::RenderGraph graph;

	DebugData debugData;

	Graphics::RenderGraphResource colorRes;
	Graphics::RenderGraphResource depthRes;
	Mock::CreateForward(graph, debugData, colorRes, depthRes);

	graph.Execute(colorRes);

	REQUIRE(debugData.passes_.size() == 3);
	REQUIRE(debugData.HavePass("RenderPassMain"));
	REQUIRE(debugData.HavePass("RenderPassHUD"));
	REQUIRE(debugData.HavePass("RenderPassFinal"));
}


TEST_CASE("render-graph-tests-deferred-simple")
{
	ScopedEngine engine;
	Graphics::RenderGraph graph;

	DebugData debugData;

	Graphics::RenderGraphResource hdrRes;
	Graphics::RenderGraphResource depthRes;
	Mock::CreateDeferred(graph, debugData, hdrRes, depthRes);

	graph.Execute(hdrRes);

	REQUIRE(debugData.passes_.size() == 4);
	REQUIRE(debugData.HavePass("RenderPassDepthPrepass"));
	REQUIRE(debugData.HavePass("RenderPassSSAO"));
	REQUIRE(debugData.HavePass("RenderPassSolid"));
	REQUIRE(debugData.HavePass("RenderPassLighting"));
}

TEST_CASE("render-graph-tests-pipeline-plugin")
{
	ScopedEngine engine;
	ImGui::Manager::Scoped imgui;
	Draw::ImGuiPipeline imguiPipeline;
	Graphics::RenderGraph graph;

	// Scan for plugins and get number.
	i32 found = Plugin::Manager::Scan(".");
	REQUIRE(found > 0);

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
	REQUIRE(selectedPlugin);

	i64 testRunCounter = GPU::MAX_GPU_FRAMES * 10;
	const Client::IInputProvider& input = engine.window.GetInputProvider();

	i32 w, h;
	engine.window.GetSize(w, h);

	Graphics::IPipeline* pipeline = nullptr;

	const bool startedWithDebugger = Core::IsDebuggerAttached();
	while(Client::Manager::Update() && (startedWithDebugger || testRunCounter-- > 0))
	{
		Graphics::RenderGraphResource scRes;
		Graphics::RenderGraphResource dsRes;

		ImGui::Manager::BeginFrame(input, w, h);

		// Draw UI for previous frame's graph.
		imguiPipeline.DrawUI(graph);

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
		REQUIRE(pipeline->HaveExecuteErrors() == false);

		// Present, next frame, wait.
		GPU::Manager::PresentSwapChain(engine.scHandle);
		GPU::Manager::NextFrame();
		Core::Sleep(1.0 / 60.0);
	}
}

TEST_CASE("render-graph-tests-draw-simple")
{
	ScopedEngine engine;
	ImGui::Manager::Scoped imgui;
	Draw::ImGuiPipeline imguiPipeline;
	Graphics::RenderGraph graph;

	f32 color[] = {0.1f, 0.1f, 0.2f, 1.0f};
	i32 testRunCounter = GPU::MAX_GPU_FRAMES * 10;
	const Client::IInputProvider& input = engine.window.GetInputProvider();

	i32 w, h;
	engine.window.GetSize(w, h);

	Graphics::RenderGraphResource scRes;
	Graphics::RenderGraphResource dsRes;

	while(Client::Manager::Update() && (Core::IsDebuggerAttached() || testRunCounter-- > 0))
	{
		ImGui::Manager::BeginFrame(input, w, h);

		// Setup render graph.
		graph.Clear();
		scRes = graph.ImportResource("Back buffer", engine.scHandle);

		DebugData debugData;
		Mock::CreateForward(graph, debugData, scRes, dsRes);

		// Setup ImGui pipeline.
		imguiPipeline.SetResource(imguiPipeline.GetResourceIdx("in_color"), scRes);
		imguiPipeline.Setup(graph);

		// Execute render graph.
		graph.Execute(imguiPipeline.GetResource(imguiPipeline.GetResourceIdx("out_color")));

		// Present.
		GPU::Manager::PresentSwapChain(engine.scHandle);

		// Next frame.
		GPU::Manager::NextFrame();

		// Force a sleep.
		Core::Sleep(1.0 / 60.0);
	}
}
