#include "app.h"
#include "forward_pipeline.h"
#include "imgui_pipeline.h"
#include "shadow_pipeline.h"
#include "render_packets.h"

#include "client/input_provider.h"
#include "client/key_input.h"
#include "client/manager.h"
#include "core/file.h"
#include "core/map.h"
#include "core/misc.h"
#include "core/string.h"
#include "gpu/types.h"
#include "graphics/pipeline.h"
#include "graphics/render_graph.h"
#include "graphics/render_pass.h"
#include "graphics/render_resources.h"
#include "graphics/material.h"
#include "graphics/model.h"
#include "imgui/manager.h"
#include "job/function_job.h"
#include "test_shared.h"
#include "math/mat44.h"
#include "math/plane.h"

namespace
{
	// HACK: dup from ImGui::Manager's GetGPSDesc.
	// Need either get it from ImGui, or bin it entirely.
	Graphics::ShaderTechniqueDesc GetShaderTechniqueDesc()
	{
		Graphics::ShaderTechniqueDesc desc;
		desc.numVertexElements_ = 3;
		desc.vertexElements_[0].usage_ = GPU::VertexUsage::POSITION;
		desc.vertexElements_[0].usageIdx_ = 0;
		desc.vertexElements_[0].streamIdx_ = 0;
		desc.vertexElements_[0].format_ = GPU::Format::R32G32_FLOAT;
		desc.vertexElements_[0].offset_ = 0;
		desc.vertexElements_[1].usage_ = GPU::VertexUsage::TEXCOORD;
		desc.vertexElements_[1].usageIdx_ = 0;
		desc.vertexElements_[1].streamIdx_ = 0;
		desc.vertexElements_[1].format_ = GPU::Format::R32G32_FLOAT;
		desc.vertexElements_[1].offset_ = 8;
		desc.vertexElements_[2].usage_ = GPU::VertexUsage::COLOR;
		desc.vertexElements_[2].usageIdx_ = 0;
		desc.vertexElements_[2].streamIdx_ = 0;
		desc.vertexElements_[2].format_ = GPU::Format::R8G8B8A8_UNORM;
		desc.vertexElements_[2].offset_ = 16;
		desc.topology_ = GPU::TopologyType::TRIANGLE;
		desc.numRTs_ = 1;
		desc.rtvFormats_[0] = GPU::Format::R8G8B8A8_UNORM;
		return desc;
	}
}

static constexpr int COLUMN_WIDTH = 256 + 128;

struct ResourceViewUI
{
	Graphics::RenderGraphResource res_;

	// Internal.
	Graphics::ShaderRef shader_;
	Graphics::ShaderBindingSet bs_;

	enum ViewMode : int
	{
		VIEW_RGBA,
		VIEW_DEPTH,
		VIEW_STENCIL,
		VIEW_VT_FEEDBACK_ID,
		VIEW_VT_FEEDBACK_UV,
		VIEW_VT_FEEDBACK_MIP,
		VIEW_MAX,
	};

	Core::Array<Graphics::ShaderTechnique, VIEW_MAX> techs_;
	ViewMode viewMode_ = VIEW_RGBA;

	GPU::Handle handle_;
	Graphics::RenderGraphBufferDesc bufDesc_;
	Graphics::RenderGraphTextureDesc texDesc_;

	int width = COLUMN_WIDTH;

	bool isOpen_ = true;

	ResourceViewUI()
	{
		shader_ = "shaders/imgui.esf";
		shader_.WaitUntilReady();
		techs_[VIEW_RGBA] = shader_->CreateTechnique("TECH_RGBA", GetShaderTechniqueDesc());
		techs_[VIEW_DEPTH] = shader_->CreateTechnique("TECH_DEPTH", GetShaderTechniqueDesc());
		techs_[VIEW_STENCIL] = shader_->CreateTechnique("TECH_STENCIL", GetShaderTechniqueDesc());
		techs_[VIEW_VT_FEEDBACK_ID] = shader_->CreateTechnique("TECH_VT_FEEDBACK_ID", GetShaderTechniqueDesc());
		techs_[VIEW_VT_FEEDBACK_UV] = shader_->CreateTechnique("TECH_VT_FEEDBACK_UV", GetShaderTechniqueDesc());
		techs_[VIEW_VT_FEEDBACK_MIP] = shader_->CreateTechnique("TECH_VT_FEEDBACK_MIP", GetShaderTechniqueDesc());
		bs_ = shader_->CreateBindingSet("ImGuiBindings");
	}

	static void DrawBindFlags(GPU::BindFlags bindFlags)
	{
		if(ImGui::TreeNode("bind_flags", "Bind flags: 0x%x", (int)bindFlags))
		{
#define APPEND_BIND_FLAG(_flag)                                                                                        \
	if(Core::ContainsAnyFlags(bindFlags, GPU::BindFlags::_flag))                                                       \
	ImGui::Text("%s", #_flag)

			APPEND_BIND_FLAG(VERTEX_BUFFER);
			APPEND_BIND_FLAG(INDEX_BUFFER);
			APPEND_BIND_FLAG(CONSTANT_BUFFER);
			APPEND_BIND_FLAG(INDIRECT_BUFFER);
			APPEND_BIND_FLAG(SHADER_RESOURCE);
			APPEND_BIND_FLAG(STREAM_OUTPUT);
			APPEND_BIND_FLAG(RENDER_TARGET);
			APPEND_BIND_FLAG(DEPTH_STENCIL);
			APPEND_BIND_FLAG(UNORDERED_ACCESS);
			APPEND_BIND_FLAG(PRESENT);

#undef APPEND_BIND_FLAG
			ImGui::TreePop();
		}
	}

	void DrawBuffer(const Graphics::RenderGraphBufferDesc& desc, GPU::Handle handle)
	{
		handle_ = handle;
		bufDesc_ = desc;

		ImGui::Text("%u B (%u KiB)", bufDesc_.size_, bufDesc_.size_ / 1024);

		DrawBindFlags(bufDesc_.bindFlags_);
	}

	void DrawTexture(const Graphics::RenderGraphTextureDesc& desc, GPU::Handle handle)
	{
		handle_ = handle;
		texDesc_ = desc;

		auto texId = ImGui::Manager::AddTextureOverride(
		    [](GPU::CommandList& cmdList, const ImGui::DrawCallData& drawCallData, void* userData) -> void {
			    ResourceViewUI& _this = *(ResourceViewUI*)userData;

			    GPU::Format srvFormat = _this.texDesc_.format_;
			    if(srvFormat == GPU::Format::R24G8_TYPELESS)
				    srvFormat = GPU::GetSRVFormatDepth(_this.texDesc_.format_);

			    if(srvFormat != GPU::Format::INVALID)
			    {
				    _this.bs_.Set(
				        "floatTex", GPU::Binding::Texture2D(_this.handle_, srvFormat, 0, _this.texDesc_.levels_));
				    _this.bs_.Set(
				        "uintTex", GPU::Binding::Texture2D(_this.handle_, srvFormat, 0, _this.texDesc_.levels_));

				    Graphics::ShaderContext shaderCtx(cmdList);

				    shaderCtx.SetBindingSet(_this.bs_);

				    GPU::Handle ps;
				    Core::ArrayView<GPU::PipelineBinding> pbs;

				    auto& tech = _this.techs_[_this.viewMode_];
				    if(shaderCtx.CommitBindings(tech, ps, pbs))
				    {
					    cmdList.Draw(ps, pbs, drawCallData.dbs_, drawCallData.fbs_, drawCallData.ds_,
					        GPU::PrimitiveTopology::TRIANGLE_LIST, drawCallData.indexOffset_, 0,
					        drawCallData.elemCount_, 0, 1);
				    }
			    }
			},
		    this);

		constexpr char* viewModes[] = {
		    "RGBA", "Depth", "Stencil", "VT Feedback (ID)", "VT Feedback (UV)", "VT Feedback (Mip)",
		};

		ImGui::Combo("View Mode", (int*)&viewMode_, viewModes, VIEW_MAX, -1);

		const f32 texW = (f32)texDesc_.width_;
		const f32 texH = (f32)texDesc_.height_;
		const f32 aspect = texW / texH;
		const f32 imgW = (f32)width - 64.0f;
		const f32 imgH = imgW / aspect;

		ImGui::Text("%.0fx%.0f, %s", texW, texH, Core::EnumToString(texDesc_.format_));
		ImGui::Text("%i levels, %i elements", texDesc_.levels_, texDesc_.elements_);
		DrawBindFlags(texDesc_.bindFlags_);

		if(handle_.GetType() == GPU::ResourceType::TEXTURE)
		{
			ImVec2 texScreenPos = ImGui::GetCursorScreenPos();
			ImGui::Image(texId, ImVec2(imgW, imgH));
			if(ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				float focusSize = 32.0f * (imgW / texW);
				float focusX = ImGui::GetMousePos().x - texScreenPos.x - focusSize * 0.5f;
				if(focusX < 0.0f)
					focusX = 0.0f;
				else if(focusX > imgW - focusSize)
					focusX = imgW - focusSize;
				float focusY = ImGui::GetMousePos().y - texScreenPos.y - focusSize * 0.5f;
				if(focusY < 0.0f)
					focusY = 0.0f;
				else if(focusY > texH - focusSize)
					focusY = texH - focusSize;
				ImGui::Text("Min: (%.2f, %.2f)", focusX, focusY);
				ImGui::Text("Max: (%.2f, %.2f)", focusX + focusSize, focusY + focusSize);
				ImVec2 uv0 = ImVec2((focusX) / imgW, (focusY) / imgH);
				ImVec2 uv1 = ImVec2((focusX + focusSize) / imgW, (focusY + focusSize) / imgH);
				ImGui::Image(
				    texId, ImVec2(128, 128), uv0, uv1, ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
				ImGui::EndTooltip();
			}
		}
		else
		{
			ImGui::Text("", "No preview available");
		}
	}

	bool operator()(Graphics::RenderGraph& renderGraph)
	{
		Core::Array<char, 256> dialogName = {};
		const char* name = nullptr;
		renderGraph.GetResourceName(res_, &name);

		Graphics::RenderGraphBufferDesc bufDesc;
		Graphics::RenderGraphTextureDesc texDesc;
		GPU::Handle handle;
		if(renderGraph.GetBuffer(res_, &bufDesc, &handle))
		{
			DrawBuffer(bufDesc, handle);
		}
		else if(renderGraph.GetTexture(res_, &texDesc, &handle))
		{
			DrawTexture(texDesc, handle);
		}

		return isOpen_;
	}
};


void DrawRenderGraphUI(Graphics::RenderGraph& renderGraph)
{
	static Core::Map<Core::String, ResourceViewUI> resourceViews;

	if(i32 numRenderPasses = renderGraph.GetNumExecutedRenderPasses())
	{
		if(ImGui::Begin("Render Passes"))
		{
			ImGui::Separator();

			Core::Vector<const Graphics::RenderPass*> renderPasses(numRenderPasses);
			Core::Vector<const char*> renderPassNames(numRenderPasses);

			renderGraph.GetExecutedRenderPasses(renderPasses.data(), renderPassNames.data());

			// Setup all resource views.
			for(i32 idx = 0; idx < numRenderPasses; ++idx)
			{
				const auto* renderPass = renderPasses[idx];

				auto inputs = renderPass->GetInputs();
				auto outputs = renderPass->GetOutputs();

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

				for(i32 texIdx = 0; texIdx < inputNames.size(); ++texIdx)
				{
					ResourceViewUI resViewUI;
					const char* name = inputNames[texIdx].c_str();
					if(auto* found = resourceViews.find(name))
					{
						std::swap(resViewUI, *found);
					}
					resViewUI.res_ = inputs[texIdx];
					resourceViews.insert(name, std::move(resViewUI));
				}

				for(i32 texIdx = 0; texIdx < outputNames.size(); ++texIdx)
				{
					ResourceViewUI resViewUI;
					const char* name = outputNames[texIdx].c_str();
					if(auto* found = resourceViews.find(name))
					{
						std::swap(resViewUI, *found);
					}
					resViewUI.res_ = outputs[texIdx];
					resourceViews.insert(name, std::move(resViewUI));
				}
			}

			// Draw UI.
			for(i32 idx = 0; idx < numRenderPasses; ++idx)
			{
				const auto* renderPass = renderPasses[idx];
				const char* renderPassName = renderPassNames[idx];

				ImGui::PushID(renderPassName);

				if(ImGui::TreeNode("Render pass: %s", renderPassName))
				{
					auto inputs = renderPass->GetInputs();
					auto outputs = renderPass->GetOutputs();

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

					static int contextSelectedIn = -1;
					static int contextSelectedOut = -1;

					const f32 ioWidth = ImGui::GetWindowWidth() * 0.3f;
					ImGui::PushItemWidth(ioWidth);
					ImVec2 ioSize = ImVec2((f32)COLUMN_WIDTH, (f32)COLUMN_WIDTH);
					{
						if(ImGui::BeginChild("Inputs", ioSize, true))
						{
							ImGui::LabelText("", "Inputs:");
							ImGui::Separator();
							for(i32 texIdx = 0; texIdx < inputNames.size(); ++texIdx)
							{
								const char* name = inputNames[texIdx].c_str();
								if(ImGui::TreeNode(name, name))
								{
									auto* ui = resourceViews.find(name);
									(*ui)(renderGraph);
									ImGui::TreePop();
								}
							}
						}
						ImGui::EndChild();
					}

					{
						ImGui::SameLine();

						if(ImGui::BeginChild("Outputs", ioSize, true))
						{
							ImGui::LabelText("", "Outputs:");
							ImGui::Separator();


							for(i32 texIdx = 0; texIdx < outputNames.size(); ++texIdx)
							{
								ResourceViewUI resViewUI;
								const char* name = outputNames[texIdx].c_str();
								if(ImGui::TreeNode(name, name))
								{
									auto* ui = resourceViews.find(name);
									(*ui)(renderGraph);
									ImGui::TreePop();
								}
							}
						}
						ImGui::EndChild();
					}
					ImGui::PopItemWidth();

					ImGui::TreePop();
				}
				ImGui::PopID();


				ImGui::Separator();
			}
		}
		ImGui::End();
	}
}


void DrawUIJobProfiler(
    bool& profilingEnabled, Core::Vector<Job::ProfilerEntry>& profilerEntries, i32 numProfilerEntries)
{
	if(ImGui::Begin("Job Profiler"))
	{
		bool oldProfilingEnabled = profilingEnabled;
		ImGui::Checkbox("Enable Profiling", &profilingEnabled);
		static f32 totalTimeMS = 16.0f;
		ImGui::SliderFloat("Total Time", &totalTimeMS, 1.0f, 100.0f);
		if(oldProfilingEnabled != profilingEnabled)
		{
			if(profilingEnabled)
				Job::Manager::BeginProfiling();
			else
				Job::Manager::EndProfiling(nullptr, 0);
		}

		Core::Array<ImColor, 12> colors = {
		    ImColor(0.8f, 0.0f, 0.0f, 1.0f), ImColor(0.0f, 0.8f, 0.0f, 1.0f), ImColor(0.0f, 0.0f, 0.8f, 1.0f),
		    ImColor(0.0f, 0.8f, 0.8f, 1.0f), ImColor(0.8f, 0.0f, 0.8f, 1.0f), ImColor(0.8f, 0.8f, 0.0f, 1.0f),
		    ImColor(0.4f, 0.0f, 0.0f, 1.0f), ImColor(0.0f, 0.4f, 0.0f, 1.0f), ImColor(0.0f, 0.0f, 0.4f, 1.0f),
		    ImColor(0.0f, 0.4f, 0.4f, 1.0f), ImColor(0.4f, 0.0f, 0.4f, 1.0f), ImColor(0.4f, 0.4f, 0.0f, 1.0f),
		};

		i32 numJobs = 0;
		i32 numWorkers = 0;
		f64 minTime = Core::Timer::GetAbsoluteTime();
		f64 maxTime = 0.0;

		for(i32 idx = 0; idx < numProfilerEntries; ++idx)
		{
			const auto& profilerEntry = profilerEntries[idx];
			numJobs = Core::Max(numJobs, profilerEntry.jobIdx_ + 1);
			numWorkers = Core::Max(numWorkers, profilerEntry.workerIdx_ + 1);
			minTime = Core::Min(minTime, profilerEntry.startTime_);
			maxTime = Core::Max(maxTime, profilerEntry.endTime_);
		}
		numWorkers = Core::Max(8, numWorkers);

		ImGui::Text("Number of jobs: %i", numJobs);
		ImGui::Text("Number of entries: %i", numProfilerEntries);
		ImGui::Separator();
		ImGui::BeginChildFrame(0, Math::Vec2(ImGui::GetWindowWidth(), numWorkers * 50.0f));

		f32 profileDrawOffsetX = 0.0f;
		f32 profileDrawOffsetY = ImGui::GetCursorPosY();
		f32 profileDrawAdvanceY = 0.0f;
		for(i32 idx = 0; idx < numWorkers; ++idx)
		{
			auto text = Core::String().Printf("Worker %i", idx);
			auto size = ImGui::CalcTextSize(text.c_str(), nullptr);
			ImGui::Text(text.c_str());
			ImGui::Separator();

			profileDrawOffsetX = Core::Max(profileDrawOffsetX, size.x);

			if(profileDrawAdvanceY == 0.0f)
			{
				profileDrawAdvanceY = ImGui::GetCursorPosY() - profileDrawOffsetY;
			}
		}

		if(numProfilerEntries > 0)
		{
			const f64 timeRange = totalTimeMS / 1000.0f; //maxTime - minTime;

			f32 totalWidth = ImGui::GetWindowWidth() - profileDrawOffsetX;

			profileDrawOffsetX += 8.0f;

			auto GetEntryPosition = [&](const Job::ProfilerEntry& entry, Math::Vec2& a, Math::Vec2& b) {
				f32 x = profileDrawOffsetX;
				f32 y = profileDrawOffsetY + (entry.workerIdx_ * profileDrawAdvanceY);

				a.x = x;
				a.y = y;
				b = a;

				f64 normalizedStart = (entry.startTime_ - minTime) / timeRange;
				f64 normalizedEnd = (entry.endTime_ - minTime) / timeRange;
				normalizedStart *= totalWidth;
				normalizedEnd *= totalWidth;

				a.x += (f32)normalizedStart;
				b.x += (f32)normalizedEnd;
				b.y += profileDrawAdvanceY;

				a += ImGui::GetWindowPos();
				b += ImGui::GetWindowPos();
			};

			// Draw bars for each worker.
			i32 hoverEntry = -1;
			auto* drawList = ImGui::GetWindowDrawList();
			for(i32 idx = 0; idx < numProfilerEntries; ++idx)
			{
				const auto& entry = profilerEntries[idx];
				const f64 entryTimeMS = (entry.endTime_ - entry.startTime_) * 1000.0;

				// Only draw > 1us.
				if(entryTimeMS > (1.0 / 1000.0) && entry.jobIdx_ >= 0)
				{
					Math::Vec2 a, b;
					GetEntryPosition(entry, a, b);
					drawList->AddRectFilled(a, b, colors[entry.jobIdx_ % colors.size()]);
					if(ImGui::IsMouseHoveringRect(a, b))
					{
						hoverEntry = idx;
					}

					if((i32)(b.x - a.x) > 8)
					{
						auto name = Core::String().Printf("%s (%.2f ms)", entry.name_.data(), entryTimeMS);
						drawList->PushClipRect(a, b, true);
						drawList->AddText(a, 0xffffffff, name.c_str());
						drawList->PopClipRect();
					}
				}
			}

			const f32 lineHeight = numWorkers * profileDrawAdvanceY;
			for(f64 time = 0.0; time < timeRange; time += 0.001)
			{
				Math::Vec2 a(profileDrawOffsetX, profileDrawOffsetY);
				Math::Vec2 b(profileDrawOffsetX, profileDrawOffsetY + lineHeight);

				f64 x = time / timeRange;
				x *= totalWidth;

				a.x += (f32)x;
				b.x += (f32)x;

				a += ImGui::GetWindowPos();
				b += ImGui::GetWindowPos();

				drawList->AddLine(a, b, ImColor(1.0f, 1.0f, 1.0f, 0.2f));
			}

			for(f64 time = 0.0; time < timeRange; time += 0.0001)
			{
				Math::Vec2 a(profileDrawOffsetX, profileDrawOffsetY);
				Math::Vec2 b(profileDrawOffsetX, profileDrawOffsetY + lineHeight);

				f64 x = time / timeRange;
				x *= totalWidth;

				a.x += (f32)x;
				b.x += (f32)x;

				a += ImGui::GetWindowPos();
				b += ImGui::GetWindowPos();

				drawList->AddLine(a, b, ImColor(1.0f, 1.0f, 1.0f, 0.1f));
			}

			if(hoverEntry >= 0)
			{
				const Math::Vec2 pos(ImGui::GetMousePos());
				const Math::Vec2 borderSize(4.0f, 4.0f);
				const auto& entry = profilerEntries[hoverEntry];

				auto name = Core::String().Printf(
				    "%s (%.4f ms)", entry.name_.data(), (entry.endTime_ - entry.startTime_) * 1000.0);

				Math::Vec2 size = ImGui::CalcTextSize(name.c_str(), nullptr);

				drawList->AddRectFilled(pos - borderSize, pos + size + borderSize, ImColor(0.0f, 0.0f, 0.0f, 0.8f));
				drawList->AddText(ImGui::GetMousePos(), 0xffffffff, name.c_str());
			}
		}
		ImGui::EndChildFrame();
	}
	ImGui::End();
}

void DrawUIGraphicsDebug(ForwardPipeline& forwardPipeline)
{
	if(ImGui::Begin("Graphics Debug"))
	{
		if(ImGui::Button("Launch RenderDoc"))
			GPU::Manager::OpenDebugCapture();
		if(ImGui::Button("Launch RenderDoc & Quit"))
			GPU::Manager::OpenDebugCapture(true);

		if(ImGui::Button("Trigger RenderDoc Capture"))
			GPU::Manager::TriggerDebugCapture();

		static int debugMode = 0;
		ImGui::Text("Debug Modes:");
		ImGui::RadioButton("Off", &debugMode, 0);
		ImGui::RadioButton("Light Culling", &debugMode, 1);

		forwardPipeline.debugMode_ = (ForwardPipeline::DebugMode)debugMode;
	}
	ImGui::End();
}

void DrawRenderPackets(Core::ArrayView<RenderPacketBase*> packets, const DrawContext& drawCtx)
{
	namespace Binding = GPU::Binding;

	rmt_ScopedCPUSample(DrawRenderPackets, RMTSF_None);
	if(auto event = drawCtx.cmdList_.Eventf(0, "DrawRenderPackets(\"%s\")", drawCtx.passName_))
	{
		// Gather mesh packets for this pass.
		Core::Vector<MeshRenderPacket*> meshPackets;
		Core::Vector<i32> meshPassTechIndices;
		meshPackets.reserve(packets.size());
		meshPassTechIndices.reserve(packets.size());
		for(auto& packet : packets)
		{
			if(packet->type_ == MeshRenderPacket::TYPE)
			{
				auto* meshPacket = static_cast<MeshRenderPacket*>(packet);
				auto passIdxIt = meshPacket->techs_->passIndices_.find(drawCtx.passName_);
				if(passIdxIt != nullptr && *passIdxIt < meshPacket->techs_->passTechniques_.size())
				{
					meshPackets.push_back(meshPacket);
					meshPassTechIndices.push_back(*passIdxIt);
				}
			}
		}

		MeshRenderPacket::DrawPackets(meshPackets, meshPassTechIndices, drawCtx);
	}
}


void RunApp(const Core::CommandLine& cmdLine, IApp& app)
{
	ScopedEngine engine(app.GetName(), cmdLine);

	ImGui::Manager::Scoped imgui;
	ImGuiPipeline imguiPipeline;
	ForwardPipeline forwardPipeline;
	ShadowPipeline shadowPipeline;
	Graphics::RenderGraph graph;

	i32 w_ = -1;
	i32 h_ = -1;
	Core::Vector<RenderPacketBase*> packets_;

	const Client::IInputProvider& input = engine.window.GetInputProvider();

	struct
	{
		f64 waitForFrameSubmit_ = 0.0;
		f64 getProfileData_ = 0.0;
		f64 imguiEndFrame_ = 0.0;
		f64 graphSetup_ = 0.0;
		f64 shaderTechniqueSetup_ = 0.0;
		f64 graphExecute_ = 0.0;
		f64 present_ = 0.0;
		f64 processDeletions_ = 0.0;
		f64 frame_ = 0.0;
		f64 tick_ = 0.0;
	} times_;

	Job::Counter* frameSubmitCounter = nullptr;

	bool frameSubmitFailure = false;
	Job::FunctionJob frameSubmitJob("Frame Submit", [&](i32) {
		// Execute, and resolve the out color target.
		times_.graphExecute_ = Core::Timer::GetAbsoluteTime();
		if(!graph.Execute(imguiPipeline.GetResource("out_color")))
		{
			frameSubmitFailure = true;
		}
		times_.graphExecute_ = Core::Timer::GetAbsoluteTime() - times_.graphExecute_;

		// Present, next frame, wait.
		times_.present_ = Core::Timer::GetAbsoluteTime();
		GPU::Manager::PresentSwapChain(engine.scHandle);
		times_.present_ = Core::Timer::GetAbsoluteTime() - times_.present_;

		times_.processDeletions_ = Core::Timer::GetAbsoluteTime();
		GPU::Manager::NextFrame();
		times_.processDeletions_ = Core::Timer::GetAbsoluteTime() - times_.processDeletions_;

	});

	Core::Vector<Job::ProfilerEntry> profilerEntries(65536);
	i32 numProfilerEntries = 0;

	bool profilingEnabled = false;

	app.Initialize();

	while(Client::Manager::Update() && !frameSubmitFailure)
	{
		const f64 targetFrameTime = 1.0 / 1200.0;
		f64 beginFrameTime = Core::Timer::GetAbsoluteTime();

		{
			rmt_ScopedCPUSample(Update, RMTSF_None);

			if(frameSubmitCounter)
			{
				rmt_ScopedCPUSample(WaitForFrameSubmit, RMTSF_None);

				// Wait for previous frame submission to complete.
				// Must update client to pump messages as the present step can send messages.
				times_.waitForFrameSubmit_ = Core::Timer::GetAbsoluteTime();
				while(Job::Manager::GetCounterValue(frameSubmitCounter) > 0)
				{
					//Client::Manager::PumpMessages();
					Job::Manager::YieldCPU();
				}
				Job::Manager::WaitForCounter(frameSubmitCounter, 0);
				times_.waitForFrameSubmit_ = Core::Timer::GetAbsoluteTime() - times_.waitForFrameSubmit_;
			}


			app.Update(input, engine.window, (f32)times_.tick_);

			i32 w = w_;
			i32 h = h_;
			engine.window.GetSize(w_, h_);

			if(w != w_ || h != h_)
			{
				// Resize swapchain.
				GPU::Manager::ResizeSwapChain(engine.scHandle, w_, h_);

				engine.scDesc.width_ = w_;
				engine.scDesc.height_ = h_;
			}

			// Wait for reloading to occur. No important jobs should be running at this point.
			Resource::Manager::WaitOnReload();

			times_.getProfileData_ = Core::Timer::GetAbsoluteTime();

			if(profilingEnabled)
			{
				numProfilerEntries = Job::Manager::EndProfiling(profilerEntries.data(), profilerEntries.size());
				Job::Manager::BeginProfiling();
			}
			times_.getProfileData_ = Core::Timer::GetAbsoluteTime() - times_.getProfileData_;

			Graphics::RenderGraphResource scRes;
			Graphics::RenderGraphResource dsRes;

			ImGui::Manager::BeginFrame(input, w_, h_, (f32)times_.tick_);

			auto DrawTimersUI = [&]() {
				if(ImGui::Begin("Timers"))
				{
					ImGui::Text("Wait on frame submit: %f ms", times_.waitForFrameSubmit_ * 1000.0);
					ImGui::Text("Get profile data: %f ms", times_.getProfileData_ * 1000.0);
					ImGui::Text("ImGui end frame: %f ms", times_.imguiEndFrame_ * 1000.0);
					ImGui::Text("Graph Setup: %f ms", times_.graphSetup_ * 1000.0);
					ImGui::Text("Shader Technique Setup: %f ms", times_.shaderTechniqueSetup_ * 1000.0);
					ImGui::Text("Graph Execute + Submit: %f ms", times_.graphExecute_ * 1000.0);
					ImGui::Text("Present Time: %f ms", times_.present_ * 1000.0);
					ImGui::Text("Process deletions: %f ms", times_.processDeletions_ * 1000.0);
					ImGui::Text("Frame Time: %f ms", times_.frame_ * 1000.0, 1.0f / times_.frame_);
					ImGui::Text("Tick Time: %f ms (%.2f FPS)", times_.tick_ * 1000.0, 1.0f / times_.tick_);

					auto genAllocStats = Core::GeneralAllocator().GetStats();
					auto virAllocStats = Core::VirtualAllocator().GetStats();
					ImGui::Text("General Usage (Peak): %.2f kb (%.2f kb)", (f32)genAllocStats.usage_ / 1024.0f,
					    (f32)genAllocStats.peakUsage_ / 1024.0f);
					ImGui::Text("Virtual Usage (Peak): %.2f kb (%.2f kb)", (f32)virAllocStats.usage_ / 1024.0f,
					    (f32)virAllocStats.peakUsage_ / 1024.0f);
				}
				ImGui::End();
			};

			ImGui::ShowTestWindow();

			DrawTimersUI();
			DrawUIGraphicsDebug(forwardPipeline);
			DrawUIJobProfiler(profilingEnabled, profilerEntries, numProfilerEntries);
			DrawRenderGraphUI(graph);

			app.UpdateGUI();

			times_.imguiEndFrame_ = Core::Timer::GetAbsoluteTime();
			ImGui::Manager::EndFrame();
			times_.imguiEndFrame_ = Core::Timer::GetAbsoluteTime() - times_.imguiEndFrame_;


			// Set draw callback.
			forwardPipeline.SetDrawCallback([&](DrawContext& drawCtx) { DrawRenderPackets(packets_, drawCtx); });

			// Clear graph prior to beginning work.
			graph.Clear();

			app.PreRender(forwardPipeline);


			times_.graphSetup_ = Core::Timer::GetAbsoluteTime();
			{
				rmt_ScopedCPUSample(Setup_Graph, RMTSF_None);

				// Import back buffer.
				Graphics::RenderGraphTextureDesc scDesc;
				scDesc.type_ = GPU::TextureType::TEX2D;
				scDesc.width_ = engine.scDesc.width_;
				scDesc.height_ = engine.scDesc.height_;
				scDesc.format_ = engine.scDesc.format_;
				auto bbRes = graph.ImportResource("Back Buffer", engine.scHandle, scDesc);
				DBG_ASSERT(bbRes);

				// Setup Shadow pipeline.
				{
					rmt_ScopedCPUSample(Setup_ShadowPipeline, RMTSF_None);
					shadowPipeline.Setup(graph);
				}

				// Setup Forward pipeline.
				{
					rmt_ScopedCPUSample(Setup_ForwardPipeline, RMTSF_None);

					forwardPipeline.SetResource("in_color", bbRes);
					forwardPipeline.SetResource("in_shadow_map", shadowPipeline.GetResource("out_shadow_map"));
					forwardPipeline.Setup(graph);

					bbRes = forwardPipeline.GetResource("out_color");
					DBG_ASSERT(bbRes);
				}

				// Setup ImGui pipeline.
				{
					rmt_ScopedCPUSample(Setup_ImGuiPipeline, RMTSF_None);

					imguiPipeline.SetResource("in_color", bbRes);
					imguiPipeline.Setup(graph);
				}
			}
			times_.graphSetup_ = Core::Timer::GetAbsoluteTime() - times_.graphSetup_;

			times_.shaderTechniqueSetup_ = Core::Timer::GetAbsoluteTime();

			// Gather render packets from app.
			{
				rmt_ScopedCPUSample(GatherRenderPackets, RMTSF_None);
				packets_.clear();

				app.Render(forwardPipeline, packets_);
			}
			{
				rmt_ScopedCPUSample(SortRenderPackets, RMTSF_None);
				SortPackets(packets_);
			}


			for(auto& packet : packets_)
			{
				if(packet->type_ == MeshRenderPacket::TYPE)
				{
					auto* meshPacket = static_cast<MeshRenderPacket*>(packet);
					forwardPipeline.CreateTechniques(meshPacket->material_, meshPacket->techDesc_, *meshPacket->techs_);
				}
			}


			times_.shaderTechniqueSetup_ = Core::Timer::GetAbsoluteTime() - times_.shaderTechniqueSetup_;

			// Schedule frame submit job.
			{
				rmt_ScopedCPUSample(FrameSubmit, RMTSF_None);
				frameSubmitJob.RunSingle(Job::Priority::HIGH, 0, &frameSubmitCounter);
				Job::Manager::WaitForCounter(frameSubmitCounter, 0);
			}
		}

		rmt_ScopedCPUSample(Sleep, RMTSF_None);

		// Sleep for the appropriate amount of time
		times_.frame_ = Core::Timer::GetAbsoluteTime() - beginFrameTime;
		if(times_.frame_ < targetFrameTime)
		{
			Core::Sleep(targetFrameTime - times_.frame_);
		}

		times_.tick_ = Core::Timer::GetAbsoluteTime() - beginFrameTime;
	}

	Job::Manager::WaitForCounter(frameSubmitCounter, 0);

	app.Shutdown();
}
