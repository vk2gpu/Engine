#include "client/manager.h"
#include "core/file.h"
#include "core/map.h"
#include "core/misc.h"
#include "core/string.h"
#include "graphics/pipeline.h"
#include "graphics/render_graph.h"
#include "graphics/render_pass.h"
#include "graphics/render_resources.h"
#include "imgui/manager.h"
#include "job/function_job.h"
#include "test_shared.h"
#include "math/mat44.h"

namespace
{
	class RenderPassImGui : public Graphics::RenderPass
	{
	public:
		static const char* StaticGetName() { return "RenderPassImGui"; }

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
			fbs_ = GPU::Manager::CreateFrameBindingSet(fbsDesc, StaticGetName());

			ImGui::Manager::Render(fbs_, cmdList);
		}

		Graphics::RenderGraphResource rt_;
		GPU::Handle fbs_;
	};

	static const char* IMGUI_RESOURCE_NAMES[] = {"in_color", "out_color", nullptr};

	class ImGuiPipeline : public Graphics::Pipeline
	{
	public:
		ImGuiPipeline() 
			: Graphics::Pipeline(IMGUI_RESOURCE_NAMES)
		{
		}

		virtual ~ImGuiPipeline() {}

		void Setup(Graphics::RenderGraph& renderGraph) override
		{
			auto& renderPassImGui = renderGraph.AddRenderPass<RenderPassImGui>("ImGui", resources_[0]);
			resources_[1] = renderPassImGui.rt_;
		}

		bool HaveExecuteErrors() const override { return false; }
	};


	void DrawRenderGraphUI(Graphics::RenderGraph& renderGraph)
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

	struct ShaderTechniques
	{
		Core::Map<Core::String, i32> passIndices_;
		Core::Vector<Graphics::ShaderTechnique> passTechniques_;
	};

	struct ObjectConstants
	{
		Math::Mat44 world_;
	};

	struct ViewConstants
	{
		Math::Mat44 view_;
		Math::Mat44 viewProj_;
	};

	enum class RenderPacketType : i16
	{
		UNKNOWN = 0,
		MESH,

		MAX,
	};

	struct RenderPacketBase
	{
		RenderPacketType type_ = RenderPacketType::UNKNOWN;
		i16 size_ = 0;
	};

	template<typename TYPE>
	struct RenderPacket : RenderPacketBase
	{
		RenderPacket()
		{
			DBG_ASSERT(TYPE::TYPE != RenderPacketType::UNKNOWN);
			DBG_ASSERT(sizeof(TYPE) <= SHRT_MAX);
			type_ = TYPE::TYPE;
			size_ = sizeof(TYPE);
		}
	};

	struct MeshRenderPacket : RenderPacket<MeshRenderPacket>
	{
		static const RenderPacketType TYPE = RenderPacketType::MESH;

		GPU::Handle db_;
		Graphics::ModelMeshDraw draw_;
		ObjectConstants object_;
		Graphics::ShaderTechniqueDesc techDesc_;
		Graphics::Shader* shader_ = nullptr;
		ShaderTechniques* techs_ = nullptr;

		// tmp object state data.
		f32 angle_ = 0.0f;
		Math::Vec3 position_;
	};
	
	Core::Vector<RenderPacketBase*> packets_;
	Core::Vector<ShaderTechniques> shaderTechniques_;

	void DrawRenderPackets(GPU::CommandList& cmdList, const char* passName, const GPU::DrawState& drawState, GPU::Handle fbs, GPU::Handle viewCBHandle, GPU::Handle objectCBHandle)
	{
		// Draw all packets.
		for(auto& packet : packets_)
		{
			if(packet->type_ == MeshRenderPacket::TYPE)
			{
				auto* meshPacket = static_cast<MeshRenderPacket*>(packet);
				auto passIdxIt = meshPacket->techs_->passIndices_.find(passName);
				if(passIdxIt != meshPacket->techs_->passIndices_.end() && passIdxIt->second < meshPacket->techs_->passTechniques_.size())
				{
					auto& tech = meshPacket->techs_->passTechniques_[passIdxIt->second];

					i32 viewIdx = meshPacket->shader_->GetBindingIndex("ViewCBuffer");
					i32 objectIdx = meshPacket->shader_->GetBindingIndex("ObjectCBuffer");
					tech.SetCBV(viewIdx, viewCBHandle, 0, sizeof(ViewConstants));
					tech.SetCBV(objectIdx, objectCBHandle, 0, sizeof(ObjectConstants));
					if(auto pbs = tech.GetBinding())
					{
						cmdList.UpdateBuffer(objectCBHandle, 0, sizeof(meshPacket->object_), &meshPacket->object_);
						cmdList.Draw(pbs, meshPacket->db_, fbs, drawState,
							GPU::PrimitiveTopology::TRIANGLE_LIST, meshPacket->draw_.indexOffset_, meshPacket->draw_.vertexOffset_,
							meshPacket->draw_.noofIndices_, 0, 1);
					}
				}
			}
		}
	}

	class RenderPassDepthPrepass : public Graphics::RenderPass
	{
	public:
		static const char* StaticGetName() { return "RenderPassDepthPrepass"; }

		Graphics::RenderGraphResource ds_;
		Graphics::RenderGraphResource viewCB_;
		Graphics::RenderGraphResource objectCB_;
		GPU::FrameBindingSetDesc fbsDesc_;
		GPU::Handle fbs_;
		ViewConstants view_;

		RenderPassDepthPrepass(Graphics::RenderGraphBuilder& builder, const Graphics::RenderGraphTextureDesc& dsDesc, Graphics::RenderGraphResource ds, const ViewConstants& view)
			: Graphics::RenderPass(builder)
		{
			if(!ds)
				ds = builder.CreateTexture("Depth", dsDesc);
			ds_ = builder.UseDSV(this, ds, GPU::DSVFlags::NONE);

			Graphics::RenderGraphTextureDesc dsTexDesc;
			if(builder.GetTexture(ds_, &dsTexDesc))
			{
				fbsDesc_.dsv_.format_ = dsTexDesc.format_;
				fbsDesc_.dsv_.dimension_ = GPU::ViewDimension::TEX2D;
			}

			Graphics::RenderGraphBufferDesc viewCBDesc;
			viewCBDesc.size_ = sizeof(ViewConstants);
			viewCB_ = builder.UseCBV(this, builder.CreateBuffer("DPP View Constants", viewCBDesc), true);

			Graphics::RenderGraphBufferDesc objectCBDesc;
			objectCBDesc.size_ = sizeof(ObjectConstants);
			objectCB_ = builder.UseCBV(this, builder.CreateBuffer("DPP Object Constants", objectCBDesc), true);

			view_ = view;
		};

		virtual ~RenderPassDepthPrepass()
		{
			/// TODO: Should be managed by frame graph.
			GPU::Manager::DestroyResource(fbs_);
		}

		void Execute(Graphics::RenderGraphResources& res, GPU::CommandList& cmdList) override
		{
			Graphics::RenderGraphTextureDesc dsTexDesc;
			fbsDesc_.dsv_.resource_ = res.GetTexture(ds_, &dsTexDesc);
			fbs_ = GPU::Manager::CreateFrameBindingSet(fbsDesc_, StaticGetName());

			// Grab constant buffers.
			GPU::Handle viewCBHandle = res.GetBuffer(viewCB_);
			GPU::Handle objectCBHandle = res.GetBuffer(objectCB_);

			// Update view constants.
			cmdList.UpdateBuffer(viewCBHandle, 0, sizeof(view_), &view_);

			// Clear depth buffer.
			cmdList.ClearDSV(fbs_, 1.0f, 0);

			GPU::DrawState drawState;
			drawState.scissorRect_.w_ = dsTexDesc.width_;
			drawState.scissorRect_.h_ = dsTexDesc.height_;
			drawState.viewport_.w_ = (f32)dsTexDesc.width_;
			drawState.viewport_.h_ = (f32)dsTexDesc.height_;

			DrawRenderPackets(cmdList, StaticGetName(), drawState, fbs_, viewCBHandle, objectCBHandle);
		}
	};

	class RenderPassForward : public Graphics::RenderPass
	{
	public:
		static const char* StaticGetName() { return "RenderPassForward"; }

		Graphics::RenderGraphResource color_;
		Graphics::RenderGraphResource ds_;
		Graphics::RenderGraphResource viewCB_;
		Graphics::RenderGraphResource objectCB_;
		GPU::FrameBindingSetDesc fbsDesc_;
		GPU::Handle fbs_;
		ViewConstants view_;

		RenderPassForward(Graphics::RenderGraphBuilder& builder, const Graphics::RenderGraphTextureDesc& colorDesc, Graphics::RenderGraphResource color,
			const Graphics::RenderGraphTextureDesc& dsDesc, Graphics::RenderGraphResource ds, const ViewConstants& view)
			: Graphics::RenderPass(builder)
		{
			color_ = builder.UseRTV(this, color);
			if(!ds)
				ds = builder.CreateTexture("Depth", dsDesc);
			ds_ = builder.UseDSV(this, ds, GPU::DSVFlags::NONE);

			Graphics::RenderGraphTextureDesc rtTexDesc;
			Graphics::RenderGraphTextureDesc dsTexDesc;
			if(builder.GetTexture(color_, &rtTexDesc) && builder.GetTexture(ds_, &dsTexDesc))
			{
				fbsDesc_.rtvs_[0].format_ = rtTexDesc.format_;
				fbsDesc_.rtvs_[0].dimension_ = GPU::ViewDimension::TEX2D;
				fbsDesc_.dsv_.format_ = dsTexDesc.format_;
				fbsDesc_.dsv_.dimension_ = GPU::ViewDimension::TEX2D;
			}

			Graphics::RenderGraphBufferDesc viewCBDesc;
			viewCBDesc.size_ = sizeof(ViewConstants);
			viewCB_ = builder.UseCBV(this, builder.CreateBuffer("FWD View Constants", viewCBDesc), true);

			Graphics::RenderGraphBufferDesc objectCBDesc;
			objectCBDesc.size_ = sizeof(ObjectConstants);
			objectCB_ = builder.UseCBV(this, builder.CreateBuffer("FWD Object Constants", objectCBDesc), true);

			view_ = view;
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
			fbsDesc_.rtvs_[0].resource_ = res.GetTexture(color_, &rtTexDesc);
			fbsDesc_.dsv_.resource_ = res.GetTexture(ds_, &dsTexDesc);
			fbs_ = GPU::Manager::CreateFrameBindingSet(fbsDesc_, StaticGetName());

			// Grab constant buffers.
			GPU::Handle viewCBHandle = res.GetBuffer(viewCB_);
			GPU::Handle objectCBHandle = res.GetBuffer(objectCB_);

			// Update view constants.
			cmdList.UpdateBuffer(viewCBHandle, 0, sizeof(view_), &view_);

			// Clear color buffer.
			f32 color[] = {0.1f, 0.1f, 0.2f, 1.0f};
			cmdList.ClearRTV(fbs_, 0, color);

			GPU::DrawState drawState;
			drawState.scissorRect_.w_ = rtTexDesc.width_;
			drawState.scissorRect_.h_ = rtTexDesc.height_;
			drawState.viewport_.w_ = (f32)rtTexDesc.width_;
			drawState.viewport_.h_ = (f32)rtTexDesc.height_;

			DrawRenderPackets(cmdList, StaticGetName(), drawState, fbs_, viewCBHandle, objectCBHandle);
		}
	};


	static const char* FORWARD_RESOURCE_NAMES[] = {"in_color", "in_depth", "out_color", "out_depth"};

	class ForwardPipeline : public Graphics::Pipeline
	{
	public:
		ForwardPipeline()
			: Graphics::Pipeline(FORWARD_RESOURCE_NAMES)
		{
		}

		virtual ~ForwardPipeline() {}
		
		Graphics::RenderGraphTextureDesc GetDefaultTextureDesc(i32 w, i32 h)
		{
			Graphics::RenderGraphTextureDesc desc;
			desc.type_ = GPU::TextureType::TEX2D;
			desc.width_ = w;
			desc.height_ = h;
			desc.format_ = GPU::Format::R8G8B8A8_UNORM;
			return desc;
		}

		Graphics::RenderGraphTextureDesc GetDepthTextureDesc(i32 w, i32 h)
		{
			Graphics::RenderGraphTextureDesc desc;
			desc.type_ = GPU::TextureType::TEX2D;
			desc.width_ = w;
			desc.height_ = h;
			desc.format_ = GPU::Format::D24_UNORM_S8_UINT;
			return desc;
		}

		/// Create appropriate shader technique for pipeline.
		void CreateTechniques(Graphics::Shader* shader, Graphics::ShaderTechniqueDesc desc, ShaderTechniques& outTechniques)
		{
			auto AddTechnique = [&](const char* name)
			{
				auto techIt = fbsDescs_.find(name);
				if(techIt != fbsDescs_.end())
				{
					desc.SetFrameBindingSet(techIt->second);
				}

				i32 size = outTechniques.passIndices_.size();
				auto idxIt = outTechniques.passIndices_.find(name);
				if(idxIt != outTechniques.passIndices_.end())
				{
					outTechniques.passTechniques_[idxIt->second] = shader->CreateTechnique(name, desc);
				}
				else
				{
					outTechniques.passTechniques_.emplace_back(shader->CreateTechnique(name, desc));
					outTechniques.passIndices_.insert(name, size);
				}
			};

			AddTechnique(RenderPassDepthPrepass::StaticGetName());
			AddTechnique(RenderPassForward::StaticGetName());
		}

		void SetCamera(const Math::Mat44& view, const Math::Mat44& proj)
		{
			view_.view_ = view;
			view_.viewProj_ = view * proj;
		}
		
		void Setup(Graphics::RenderGraph& renderGraph) override
		{
			i32 w = 1024;
			i32 h = 768;
						
			auto& renderPassDepthPrepass = renderGraph.AddRenderPass<RenderPassDepthPrepass>("Depth Prepass", GetDepthTextureDesc(w, h), resources_[1], view_);

			auto& renderPassForward = renderGraph.AddRenderPass<RenderPassForward>("Forward", GetDefaultTextureDesc(w, h), resources_[0], GetDepthTextureDesc(w, h), renderPassDepthPrepass.ds_, view_);
			resources_[2] = renderPassForward.color_;
			resources_[3] = renderPassForward.ds_;

			fbsDescs_.insert(RenderPassDepthPrepass::StaticGetName(), renderPassDepthPrepass.fbsDesc_);
			fbsDescs_.insert(RenderPassForward::StaticGetName(), renderPassForward.fbsDesc_);
		}

		bool HaveExecuteErrors() const override { return false; }

		Core::Map<Core::String, GPU::FrameBindingSetDesc> fbsDescs_;

		ViewConstants view_;
	};
}

void Loop()
{
	ScopedEngine engine("Test Bed App");
	ImGui::Manager::Scoped imgui;
	ImGuiPipeline imguiPipeline;
	ForwardPipeline forwardPipeline;
	Graphics::RenderGraph graph;

	// Load shader + teapot model.
	Graphics::Shader* shader = nullptr;
	DBG_ASSERT(Resource::Manager::RequestResource(shader, "shader_tests/simple-mesh.esf"));
	Resource::Manager::WaitForResource(shader);

	Graphics::Model* model = nullptr;
	DBG_ASSERT(Resource::Manager::RequestResource(model, "model_tests/teapot.obj"));
	Resource::Manager::WaitForResource(model);

	// Create some render packets.
	// For now, they can be permenent.
	f32 angle = 0.0;
	const Math::Vec3 positions[] =
	{
		Math::Vec3(-10.0f,  0.0f,  -5.0f),
		Math::Vec3( -5.0f,  0.0f,  -5.0f),
		Math::Vec3(  0.0f,  0.0f,  -5.0f),
		Math::Vec3(  5.0f,  0.0f,  -5.0f),
		Math::Vec3( 10.0f,  0.0f,  -5.0f),
		Math::Vec3(-10.0f,  0.0f,   5.0f),
		Math::Vec3( -5.0f,  0.0f,   5.0f),
		Math::Vec3(  0.0f,  0.0f,   5.0f),
		Math::Vec3(  5.0f,  0.0f,   5.0f),
		Math::Vec3( 10.0f,  0.0f,   5.0f),	
	};
	shaderTechniques_.reserve(model->GetNumMeshes() * 100);
	for(const auto& position : positions)
	{
		for(i32 idx = 0; idx < model->GetNumMeshes(); ++idx)
		{
			ShaderTechniques techniques;
			Graphics::ShaderTechniqueDesc techDesc;
			techDesc.SetVertexElements(model->GetMeshVertexElements(idx));				
			techDesc.SetTopology(GPU::TopologyType::TRIANGLE);
		
			MeshRenderPacket packet;
			packet.db_ = model->GetMeshDrawBinding(idx);
			packet.draw_ = model->GetMeshDraw(idx);
			packet.techDesc_ = techDesc;
			packet.shader_ = shader;
			packet.techs_ = &*shaderTechniques_.emplace_back(std::move(techniques));

			packet.angle_ = angle;
			packet.position_ = position;

			angle += 0.5f;

			packets_.emplace_back(new MeshRenderPacket(packet));
		}
	}

	const Client::IInputProvider& input = engine.window.GetInputProvider();

	struct
	{
		f64 graphSetup_ = 0.0;
		f64 shaderTechniqueSetup_ = 0.0;
		f64 graphExecute_ = 0.0;
		f64 present_ = 0.0;
		f64 frame_ = 0.0;		
	} times_;

	i32 w, h;
	engine.window.GetSize(w, h);

	Job::Counter* frameSubmitCounter = nullptr;

	Job::FunctionJob frameSubmitJob("Frame Submit", 
	[&](i32)
	{
		times_.graphExecute_ = Core::Timer::GetAbsoluteTime();

		// Execute, and resolve the out color target.
		graph.Execute(imguiPipeline.GetResource("out_color"));

		times_.graphExecute_ = Core::Timer::GetAbsoluteTime() - times_.graphExecute_;

		times_.present_ = Core::Timer::GetAbsoluteTime();

		// Present, next frame, wait.
		GPU::Manager::PresentSwapChain(engine.scHandle);
		GPU::Manager::NextFrame();

		times_.present_ = Core::Timer::GetAbsoluteTime() - times_.present_;
	});

	Core::Vector<Job::ProfilerEntry> profilerEntries(65536);
	i32 numProfilerEntries = 0;

	bool profilingEnabled = false; 

	while(Client::Manager::Update())
	{
		const f64 targetFrameTime = 1.0 / 120.0;
		f64 beginFrameTime = Core::Timer::GetAbsoluteTime();

		// Wait for previous frame submission to complete.
		// Must update client to pump messages as the present step can send messages.
		while(Job::Manager::GetCounterValue(frameSubmitCounter) > 0)
		{
			Client::Manager::Update();
			Job::Manager::YieldCPU();
		}
		Job::Manager::WaitForCounter(frameSubmitCounter, 0);

		// Wait for reloading to occur. No important jobs should be running at this point.
		Resource::Manager::WaitOnReload();

		if(profilingEnabled)
		{
			numProfilerEntries = Job::Manager::EndProfiling(profilerEntries.data(), profilerEntries.size());
			Job::Manager::BeginProfiling();
		}

		Graphics::RenderGraphResource scRes;
		Graphics::RenderGraphResource dsRes;

		ImGui::Manager::BeginFrame(input, w, h);

		if(ImGui::Begin("Timers"))
		{
			ImGui::Text("Graph Setup: %f ms", times_.graphSetup_ * 1000.0);
			ImGui::Text("Shader Technique Setup: %f ms", times_.shaderTechniqueSetup_ * 1000.0);
			ImGui::Text("Graph Execute + Submit: %f ms", times_.graphExecute_ * 1000.0);
			ImGui::Text("Present Time: %f ms", times_.present_ * 1000.0);
			ImGui::Text("Frame Time: %f ms", times_.frame_ * 1000.0);
		}
		ImGui::End();

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

			Core::Array<ImColor, 12> colors = 
			{
				ImColor(0.8f, 0.0f, 0.0f, 1.0f),
				ImColor(0.0f, 0.8f, 0.0f, 1.0f),
				ImColor(0.0f, 0.0f, 0.8f, 1.0f),
				ImColor(0.0f, 0.8f, 0.8f, 1.0f),
				ImColor(0.8f, 0.0f, 0.8f, 1.0f),
				ImColor(0.8f, 0.8f, 0.0f, 1.0f),
				ImColor(0.4f, 0.0f, 0.0f, 1.0f),
				ImColor(0.0f, 0.4f, 0.0f, 1.0f),
				ImColor(0.0f, 0.0f, 0.4f, 1.0f),
				ImColor(0.0f, 0.4f, 0.4f, 1.0f),
				ImColor(0.4f, 0.0f, 0.4f, 1.0f),
				ImColor(0.4f, 0.4f, 0.0f, 1.0f),
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
				const f64 timeRange = totalTimeMS / 1000.0f;//maxTime - minTime;



				f32 totalWidth = ImGui::GetWindowWidth() - profileDrawOffsetX; 

				profileDrawOffsetX += 8.0f;

				auto GetEntryPosition = [&](const Job::ProfilerEntry& entry, Math::Vec2& a, Math::Vec2& b)
				{
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

				if(hoverEntry >= 0)
				{
					const Math::Vec2 pos(ImGui::GetMousePos());
					const Math::Vec2 borderSize(4.0f, 4.0f);
					const auto& entry = profilerEntries[hoverEntry];

					auto name = Core::String().Printf("%s (%.4f ms)", entry.name_.data(), (entry.endTime_ - entry.startTime_) * 1000.0);

					Math::Vec2 size = ImGui::CalcTextSize(name.c_str(), nullptr);
				
					drawList->AddRectFilled(pos - borderSize, pos + size + borderSize, ImColor(0.0f, 0.0f, 0.0f, 0.8f));
					drawList->AddText(ImGui::GetMousePos(), 0xffffffff, name.c_str());
								
				}
			}			
			ImGui::EndChildFrame();
		}
		ImGui::End();

		DrawRenderGraphUI(graph);

		// Update render packet positions.
		for(auto& packet : packets_)
		{
			if(packet->type_ == MeshRenderPacket::TYPE)
			{
				auto* meshPacket = static_cast<MeshRenderPacket*>(packet);

				meshPacket->angle_ += (f32)targetFrameTime;
				meshPacket->object_.world_.Rotation(Math::Vec3(0.0f, meshPacket->angle_, 0.0f));
				meshPacket->object_.world_.Translation(meshPacket->position_);
			}
		}
		ImGui::Manager::EndFrame();

		// Setup pipeline camera.
		Math::Mat44 view;
		Math::Mat44 proj;
		view.LookAt(Math::Vec3(0.0f, 5.0f, 10.0f), Math::Vec3(0.0f, 1.0f, 0.0f), Math::Vec3(0.0f, 1.0f, 0.0f));
		proj.PerspProjectionVertical(Core::F32_PIDIV4, (f32)h / (f32)w, 0.01f, 300.0f);
		forwardPipeline.SetCamera(view, proj);

		// Clear graph prior to beginning work.
		graph.Clear();

		times_.graphSetup_ = Core::Timer::GetAbsoluteTime();

		// Import back buffer.
		Graphics::RenderGraphTextureDesc scDesc;
		scDesc.type_ = GPU::TextureType::TEX2D;
		scDesc.width_ = engine.scDesc.width_;
		scDesc.height_ = engine.scDesc.height_;
		scDesc.format_ = engine.scDesc.format_;
		auto bbRes = graph.ImportResource("Back Buffer", engine.scHandle, scDesc);

		// Set color target to back buffer.
		forwardPipeline.SetResource("in_color", bbRes);

		// Have the pipeline setup on the graph.
		forwardPipeline.Setup(graph);

		// Setup ImGui pipeline.
		imguiPipeline.SetResource("in_color", forwardPipeline.GetResource("out_color"));
		imguiPipeline.Setup(graph);

		times_.graphSetup_ = Core::Timer::GetAbsoluteTime() - times_.graphSetup_;

		times_.shaderTechniqueSetup_ = Core::Timer::GetAbsoluteTime();

		// Setup all shader techniques for the built graph.
		for(auto& packet : packets_)
		{
			if(packet->type_ == MeshRenderPacket::TYPE)
			{
				auto* meshPacket = static_cast<MeshRenderPacket*>(packet);
				forwardPipeline.CreateTechniques(shader, meshPacket->techDesc_, *meshPacket->techs_);
			}
		}

		times_.shaderTechniqueSetup_ = Core::Timer::GetAbsoluteTime() - times_.shaderTechniqueSetup_;

		// Schedule frame submit job.
		frameSubmitJob.RunSingle(0, &frameSubmitCounter);

		// Sleep for the appropriate amount of time
		times_.frame_ = Core::Timer::GetAbsoluteTime() - beginFrameTime;
		if(times_.frame_ < targetFrameTime)
		{
			Core::Sleep(targetFrameTime - times_.frame_);
		}
	}

	// Clean up.
	for(auto* packet : packets_)
	{
		if(packet->type_ == MeshRenderPacket::TYPE)
		{
			auto* meshPacket = static_cast<MeshRenderPacket*>(packet);
			delete meshPacket;
		}
	}
	shaderTechniques_.clear();
	Resource::Manager::ReleaseResource(shader);
	Resource::Manager::ReleaseResource(model);
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

	Loop();

	return 0;
}
