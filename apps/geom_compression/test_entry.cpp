#include "common.h"
#include "app.h"
#include "camera.h"
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
#include "core/random.h"
#include "core/string.h"
#include "gpu/types.h"
#include "graphics/pipeline.h"
#include "graphics/render_graph.h"
#include "graphics/render_pass.h"
#include "graphics/render_resources.h"
#include "graphics/material.h"
#include "graphics/model.h"
#include "image/color.h"
#include "imgui/manager.h"
#include "job/function_job.h"
#include "test_shared.h"
#include "math/mat44.h"
#include "math/plane.h"

#include "compressed_model.h"

#include <cmath>


class App : public IApp
{
public:
	const char* GetName() const override
	{
		return "Geometry Compression";
	}

	void Initialize() override
	{
		modelNames_.push_back("model_tests/teapot.obj");
		modelNames_.push_back("model_tests/cube.obj");
		modelNames_.push_back("model_tests/crytek-sponza/sponza.obj");

		Graphics::ModelRef model = modelNames_[selectedModelIdx_];
		pendingModels_.push_back(model);
		models_.emplace_back(std::move(model));
	}

	void Shutdown() override
	{
		models_.clear();
	}

	void Update(const Client::IInputProvider& input, const Client::Window& window, f32 tick) override
	{
		camera_.Update(input, tick);

		window.GetSize(w_, h_);
		view_.LookAt(Math::Vec3(0.0f, 5.0f, -15.0f), Math::Vec3(0.0f, 1.0f, 0.0f), Math::Vec3(0.0f, 1.0f, 0.0f));
		proj_.PerspProjectionVertical(Core::F32_PIDIV4, (f32)h_ / (f32)w_, 0.1f, 2000.0f);

		for(auto it = pendingModels_.begin(); it != pendingModels_.end(); )
		{
			Graphics::Model* model = *it;
			if(model->IsReady())
			{
				CreateRenderPackets(model);
				pendingModels_.erase(it);
			}
			else
			{
				++it;
			}
		}

		if(numLights_ != lightMoveInfos_.size())
		{
			if(numLights_ > lightMoveInfos_.size())
			{
				auto RandomF32 = [this](f32 min, f32 max)
				{
					const i32 v = rng_.Generate() & 0xffff;
					const f32 r = max - min;
					return ((f32(v) / f32(0xffff)) * r) + min;
				};

				auto RandomVec3 = [&](f32 min, f32 max)
				{
					return Math::Vec3(
						RandomF32(min, max), 
						RandomF32(min, max), 
						RandomF32(min, max));
				};

				// Add new lights.
				const i32 numAddLights = numLights_ - lightMoveInfos_.size();
				for(i32 i = 0; i < numAddLights; ++i)
				{
					LightMoveInfo lightInfo = {};
					lightInfo.time_ = RandomF32(-1000.0f, 1000.0f);
					lightInfo.axisTimeMultiplier_ = RandomVec3(-0.1f, 0.1f);
					lightInfo.axisSizeMultiplier_ = RandomVec3(-1.0f, 1.0f).Normal() * RandomF32(100.0f, 2000.0f);

					Image::HSVColor hsvColor;
					hsvColor.h = RandomF32(0.0f, 1.0f);
					hsvColor.s = 0.9f;
					hsvColor.v = 1.0f;
					auto color = Image::ToRGB(hsvColor);
					lightInfo.color_.x = color.r;
					lightInfo.color_.y = color.g;
					lightInfo.color_.z = color.b;

					lightMoveInfos_.push_back(lightInfo);
				}
			}
			else
			{
				// Shrink.
				lightMoveInfos_.resize(numLights_);
			}
		}


		for(auto& lightInfo : lightMoveInfos_)
		{
			lightInfo.time_ += tick;
		}

	}


	void UpdateGUI()
	{
		// ImGui stuff.
		if(ImGui::Begin("Options"))
		{
			ImGui::Combo("Model", &selectedModelIdx_, modelNames_.data(), modelNames_.size());
			ImGui::SameLine();
			if(ImGui::Button("Load Model"))
			{
				Core::ScopedMutex lock(packetMutex_);
				for(auto* packet : packets_)
				{
					delete packet;
				}
				packets_.clear();
				shaderTechniques_.clear();
				pendingModels_.clear();
				models_.clear();

				Graphics::ModelRef model = modelNames_[selectedModelIdx_];
				pendingModels_.push_back(model);
				models_.emplace_back(std::move(model));
			}


			ImGui::SliderInt("Num Lights", &numLights_, 0, 1000);
			ImGui::SliderFloat("Brightness", &lightBrightness_, 1.0f, 1000.0f);
		}
		ImGui::End();
	}


	void PreRender(Graphics::Pipeline& pipeline) override
	{
		ForwardPipeline& forwardPipeline = static_cast<ForwardPipeline&>(pipeline);

		forwardPipeline.SetCamera(camera_.matrix_, proj_, Math::Vec2((f32)w_, (f32)h_), true);

		// Setup lights.
		Light light = {};

		forwardPipeline.lights_.clear();
		if(lightMoveInfos_.size() == 0)
		{
			light.position_ = Math::Vec3(1000.0f, 1000.0f, 1000.0f);
			light.color_.x = 1.0f;
			light.color_.y = 1.0f;
			light.color_.z = 1.0f;
			light.color_ *= 980000.0f;
			light.radiusInner_ = 10000.0f;
			light.radiusOuter_ = 20000.0f;
			forwardPipeline.lights_.push_back(light);
		}

		for(auto& lightInfo : lightMoveInfos_)
		{
			light.position_ = Math::Vec3(
				(f32)std::sin(lightInfo.axisTimeMultiplier_.x * lightInfo.time_),
				(f32)std::sin(lightInfo.axisTimeMultiplier_.y * lightInfo.time_),
				(f32)std::sin(lightInfo.axisTimeMultiplier_.z * lightInfo.time_)) * lightInfo.axisSizeMultiplier_;
			light.color_ = lightInfo.color_ * lightBrightness_;
			light.radiusInner_ = 50.0f;
			light.radiusOuter_ = 100.0f;
			forwardPipeline.lights_.push_back(light);
		}
	}


	void Render(Graphics::Pipeline& pipeline, Core::Vector<RenderPacketBase*>& outPackets) override
	{
		Core::ScopedMutex lock(packetMutex_);
		outPackets.insert(packets_.begin(), packets_.end());
	}


	void CreateRenderPackets(Graphics::Model* model)
	{
		Core::ScopedMutex lock(packetMutex_);
		// TODO: Calculate how many of these we need overall.
		shaderTechniques_.reserve(100000);
		ShaderTechniques* techniques = &*shaderTechniques_.emplace_back();

		for(i32 idx = 0; idx < model->GetNumMeshes(); ++idx)
		{
			Graphics::ShaderTechniqueDesc techDesc;
			techDesc.SetVertexElements(model->GetMeshVertexElements(idx));
			techDesc.SetTopology(GPU::TopologyType::TRIANGLE);

			MeshRenderPacket packet;
			packet.db_ = model->GetMeshDrawBinding(idx);
			packet.draw_ = model->GetMeshDraw(idx);
			packet.techDesc_ = techDesc;
			packet.material_ = model->GetMeshMaterial(idx);

			if(techniques->material_ != packet.material_)
				techniques = &*shaderTechniques_.emplace_back();

			packet.techs_ = techniques;
			packet.object_.world_ = model->GetMeshWorldTransform(idx);

			packets_.emplace_back(new MeshRenderPacket(packet));
		}
	}

	Camera camera_;
	Math::Mat44 view_;
	Math::Mat44 proj_;
	i32 w_, h_;

	Core::Vector<Graphics::ModelRef> models_;
	Core::Vector<Graphics::Model*> pendingModels_;


	Core::Mutex packetMutex_;
	Core::Vector<RenderPacketBase*> packets_;
	Core::Vector<ShaderTechniques> shaderTechniques_;

	int selectedModelIdx_ = 0;
	int numLights_ = 0;

	struct LightMoveInfo
	{
		Math::Vec3 axisTimeMultiplier_ = Math::Vec3(0.0f, 0.0f, 0.0f);
		Math::Vec3 axisSizeMultiplier_ = Math::Vec3(0.0f, 0.0f, 0.0f);
		f64 time_ = 0.0;
		Math::Vec3 color_ = Math::Vec3(1.0f, 1.0f, 1.0f);
	};
	Core::Vector<LightMoveInfo> lightMoveInfos_;
	f32 lightBrightness_ = 40.0f;
	Core::Random rng_;

	Core::Vector<const char*> modelNames_;

};

#if 0
void Loop(const Core::CommandLine& cmdLine)
{
	ScopedEngine engine("Geometry Compression App", cmdLine);
	ImGui::Manager::Scoped imgui;
	ImGuiPipeline imguiPipeline;
	ForwardPipeline forwardPipeline;
	ShadowPipeline shadowPipeline;
	RenderGraph graph;

	MaterialRef testMaterial("test_material.material");
	testMaterial.WaitUntilReady();

	Model* model = nullptr;
//	Resource::Manager::RequestResource(model, "model_tests/SpeedTree/Azalea/HighPoly/Azalea.fbx");
//	Resource::Manager::RequestResource(model, "model_tests/SpeedTree/Backyard Grass/HighPoly/Backyard_Grass.fbx");
//	Resource::Manager::RequestResource(model, "model_tests/SpeedTree/Boston Fern/HighPoly/Boston_Fern.fbx");
//	Resource::Manager::RequestResource(model, "model_tests/SpeedTree/European Linden/HighPoly/European_Linden.fbx");
//	Resource::Manager::RequestResource(model, "model_tests/SpeedTree/Hedge/HighPoly/Hedge.fbx");
//	Resource::Manager::RequestResource(model, "model_tests/SpeedTree/Japanese Maple/HighPoly/Japanese_Maple.fbx");
//	Resource::Manager::RequestResource(model, "model_tests/SpeedTree/Red Maple Young/HighPoly/Red_Maple_Young.fbx");
//	Resource::Manager::RequestResource(model, "model_tests/SpeedTree/White Oak/HighPoly/White_Oak.fbx");

	Resource::Manager::RequestResource(model, "model_tests/teapot.obj");
	Model* sponzaModel = nullptr;

	Resource::Manager::WaitForResource(model);

	// Create some render packets.
	// For now, they can be permenent.
	f32 angle = 0.0;
	const Math::Vec3 positions[] = {
	    Math::Vec3(-10.0f, 10.0f, -5.0f),
		Math::Vec3(-5.0f, 10.0f, -5.0f),
	    Math::Vec3(0.0f, 10.0f, -5.0f),
	    Math::Vec3(5.0f, 10.0f, -5.0f),
	    Math::Vec3(10.0f, 10.0f, -5.0f),
	    Math::Vec3(-10.0f, 10.0f, 5.0f),
	    Math::Vec3(-5.0f, 10.0f, 5.0f),
	    Math::Vec3(0.0f, 10.0f, 5.0f),
	    Math::Vec3(5.0f, 10.0f, 5.0f),
	    Math::Vec3(10.0f, 10.0f, 5.0f),
	};

	Core::Random rng;

	Light light = {};

	light.position_ = Math::Vec3(1000.0f, 1000.0f, 1000.0f);
	light.color_.x = 1.0f;
	light.color_.y = 1.0f;
	light.color_.z = 1.0f;
	light.color_ *= 980000.0f;
	light.radiusInner_ = 10000.0f;
	light.radiusOuter_ = 20000.0f;
	forwardPipeline.lights_.push_back(light);

	auto randF32 = [&rng](f32 min, f32 max) {
		const u32 large = 65536;
		auto val = (f32)(((u32)rng.Generate()) % large);
		val /= (f32)large;
		return min + val * (max - min);
	};

	f32 r = 0.0f, g = 0.0f, b = 0.0f, h = 0.0f, s = 1.0f, v = 1.0f;
	for(const auto& position : positions)
	{
		ImGui::ColorConvertHSVtoRGB(h, s, v, r, g, b);
		h += 0.1f;
		light.position_ = position + Math::Vec3(0.0f, 10.0f, 0.0f);
		light.color_.x = r;
		light.color_.y = g;
		light.color_.z = b;
		light.color_ *= 4.0f;
		light.radiusInner_ = 15.0f;
		light.radiusOuter_ = 25.0f;

		forwardPipeline.lights_.push_back(light);
	}

	shaderTechniques_.reserve(100000);
	ShaderTechniques* techniques = &*shaderTechniques_.emplace_back();

	for(const auto& position : positions)
	{
		for(i32 idx = 0; idx < model->GetNumMeshes(); ++idx)
		{
			ShaderTechniqueDesc techDesc;
			techDesc.SetVertexElements(model->GetMeshVertexElements(idx));
			techDesc.SetTopology(GPU::TopologyType::TRIANGLE);

			MeshRenderPacket packet;
			packet.db_ = model->GetMeshDrawBinding(idx);
			packet.draw_ = model->GetMeshDraw(idx);
			packet.techDesc_ = techDesc;
			packet.material_ = model->GetMeshMaterial(idx);

			if(techniques->material_ != packet.material_)
				techniques = &*shaderTechniques_.emplace_back();

			packet.techs_ = techniques;

			packet.world_ = model->GetMeshWorldTransform(idx);
			packet.angle_ = angle;
			packet.position_ = position;

			angle += 0.5f;

			packets_.emplace_back(new MeshRenderPacket(packet));
		}
	}

	if(sponzaModel)
	{
		for(i32 idx = 0; idx < sponzaModel->GetNumMeshes(); ++idx)
		{
			ShaderTechniqueDesc techDesc;
			techDesc.SetVertexElements(sponzaModel->GetMeshVertexElements(idx));
			techDesc.SetTopology(GPU::TopologyType::TRIANGLE);

			MeshRenderPacket packet;
			packet.db_ = sponzaModel->GetMeshDrawBinding(idx);
			packet.draw_ = sponzaModel->GetMeshDraw(idx);
			packet.techDesc_ = techDesc;
			packet.material_ = sponzaModel->GetMeshMaterial(idx);

			if(techniques->material_ != packet.material_)
				techniques = &*shaderTechniques_.emplace_back();

			packet.techs_ = techniques;

			packet.world_ = sponzaModel->GetMeshWorldTransform(idx);
			Math::Mat44 scale;
			scale.Scale(Math::Vec3(1.0f, 1.0f, 1.0f));
			packet.world_ = scale * packet.world_;
			packet.angle_ = 0.0f;
			packet.position_ = Math::Vec3(0.0f, 0.0f, 0.0f);

			packets_.emplace_back(new MeshRenderPacket(packet));
		}
	}


	const Client::IInputProvider& input = engine.window.GetInputProvider();

	struct
	{
		f64 waitForFrameSubmit_ = 0.0;
		f64 getProfileData_ = 0.0;
		f64 profilerUI_ = 0.0;
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


			camera_.Update(input, (f32)times_.tick_);

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

			RenderGraphResource scRes;
			RenderGraphResource dsRes;

			ImGui::Manager::BeginFrame(input, w_, h_, (f32)times_.tick_);

			times_.profilerUI_ = Core::Timer::GetAbsoluteTime();
			DrawUIGraphicsDebug(forwardPipeline);
			DrawUIJobProfiler(profilingEnabled, profilerEntries, numProfilerEntries);
			times_.profilerUI_ = Core::Timer::GetAbsoluteTime() - times_.profilerUI_;

			if(ImGui::Begin("Timers"))
			{
				ImGui::Text("Wait on frame submit: %f ms", times_.waitForFrameSubmit_ * 1000.0);
				ImGui::Text("Get profile data: %f ms", times_.getProfileData_ * 1000.0);
				ImGui::Text("Profiler UI: %f ms", times_.profilerUI_ * 1000.0);
				ImGui::Text("ImGui end frame: %f ms", times_.imguiEndFrame_ * 1000.0);
				ImGui::Text("Graph Setup: %f ms", times_.graphSetup_ * 1000.0);
				ImGui::Text("Shader Technique Setup: %f ms", times_.shaderTechniqueSetup_ * 1000.0);
				ImGui::Text("Graph Execute + Submit: %f ms", times_.graphExecute_ * 1000.0);
				ImGui::Text("Present Time: %f ms", times_.present_ * 1000.0);
				ImGui::Text("Process deletions: %f ms", times_.processDeletions_ * 1000.0);
				ImGui::Text("Frame Time: %f ms", times_.frame_ * 1000.0, 1.0f / times_.frame_);
				ImGui::Text("Tick Time: %f ms (%.2f FPS)", times_.tick_ * 1000.0, 1.0f / times_.tick_);
			}
			ImGui::End();

			DrawRenderGraphUI(graph);

			// Update render packet positions.
			{
				rmt_ScopedCPUSample(UpdateRenderPackets, RMTSF_None);

				for(auto& packet : packets_)
				{
					if(packet->type_ == MeshRenderPacket::TYPE)
					{
						auto* meshPacket = static_cast<MeshRenderPacket*>(packet);

						//meshPacket->angle_ += (f32)times_.frame_;
						meshPacket->object_.world_.Rotation(Math::Vec3(0.0f, meshPacket->angle_, 0.0f));
						meshPacket->object_.world_.Translation(meshPacket->position_);
						meshPacket->object_.world_ = meshPacket->world_ * meshPacket->object_.world_;
					}
				}
			}

			times_.imguiEndFrame_ = Core::Timer::GetAbsoluteTime();
			ImGui::Manager::EndFrame();
			times_.imguiEndFrame_ = Core::Timer::GetAbsoluteTime() - times_.imguiEndFrame_;

			// Setup pipeline camera.
			Math::Mat44 view;
			Math::Mat44 proj;
			view.LookAt(Math::Vec3(0.0f, 5.0f, -15.0f), Math::Vec3(0.0f, 1.0f, 0.0f), Math::Vec3(0.0f, 1.0f, 0.0f));
			proj.PerspProjectionVertical(Core::F32_PIDIV4, (f32)h_ / (f32)w_, 0.1f, 2000.0f);
			forwardPipeline.SetCamera(camera_.matrix_, proj, Math::Vec2((f32)w_, (f32)h_), updateFrustum_);

			// Setup shadow light + eye pos.
			shadowPipeline.SetDirectionalLight(camera_.cameraTarget_, forwardPipeline.lights_[0]);

			// Set draw callback.
			forwardPipeline.SetDrawCallback([&](DrawContext& drawCtx) {
				DrawRenderPackets(drawCtx);
			});

			// Clear graph prior to beginning work.
			graph.Clear();

			times_.graphSetup_ = Core::Timer::GetAbsoluteTime();
			{
				rmt_ScopedCPUSample(Setup_Graph, RMTSF_None);

				// Import back buffer.
				RenderGraphTextureDesc scDesc;
				scDesc.type_ = GPU::TextureType::TEX2D;
				scDesc.width_ = engine.scDesc.width_;
				scDesc.height_ = engine.scDesc.height_;
				scDesc.format_ = engine.scDesc.format_;
				auto bbRes = graph.ImportResource("Back Buffer", engine.scHandle, scDesc);

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

			// Setup all shader techniques for the built graph.
			{
				rmt_ScopedCPUSample(CreateTechniques, RMTSF_None);
				for(auto& packet : packets_)
				{
					if(packet->type_ == MeshRenderPacket::TYPE)
					{
						auto* meshPacket = static_cast<MeshRenderPacket*>(packet);
						forwardPipeline.CreateTechniques(
						    meshPacket->material_, meshPacket->techDesc_, *meshPacket->techs_);
					}
				}
			}


			times_.shaderTechniqueSetup_ = Core::Timer::GetAbsoluteTime() - times_.shaderTechniqueSetup_;

			// Schedule frame submit job.
			{
				rmt_ScopedCPUSample(FrameSubmit, RMTSF_None);
				frameSubmitJob.RunSingle(0, &frameSubmitCounter);
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
	Resource::Manager::ReleaseResource(texture);
}
#endif

int main(int argc, char* const argv[])
{
	Client::Manager::Scoped clientManager;

	// Change to executable path.
	char path[Core::MAX_PATH_LENGTH];
	if(Core::FileSplitPath(argv[0], path, Core::MAX_PATH_LENGTH, nullptr, 0, nullptr, 0))
	{
		Core::FileChangeDir(path);
	}

	Core::CommandLine cmdLine(argc, argv);

	App app;
	RunApp(cmdLine, app);

	return 0;
}
