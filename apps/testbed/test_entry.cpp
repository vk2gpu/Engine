#include "common.h"
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
#include "imgui/manager.h"
#include "job/function_job.h"
#include "test_shared.h"
#include "math/mat44.h"
#include "math/plane.h"

#include "clustered_model.h"
#include "texture_compressor.h"

#include <cmath>

#define LOAD_SPONZA (0)

namespace
{
	using namespace Graphics;

	class Camera
	{
	public:
		enum class CameraState
		{
			IDLE = 0,
			ROTATE,
			PAN
		};

		Camera()
		{
			cameraDistance_ = 1.0f;
			cameraZoom_ = 0.0f;
			moveFast_ = false;
			cameraRotation_ = Math::Vec3(0.0f, 0.0f, 0.0f);
			cameraWalk_ = Math::Vec3(0.0f, 0.0f, 0.0f);
			cameraTarget_ = Math::Vec3(0.0f, 5.0f, 5.0f);
		}

		void Update(const Client::IInputProvider& input, f32 tick)
		{
			if(input.WasMouseButtonPressed(0))
			{
				initialMousePos_ = input.GetMousePosition();
			}

			if(input.WasKeyReleased(Client::KeyCode::LEFT) || input.WasKeyReleased(Client::KeyCode::RIGHT))
				cameraRotationDelta_.y = 0.0f;

			if(input.WasKeyReleased(Client::KeyCode::UP) || input.WasKeyReleased(Client::KeyCode::DOWN))
				cameraRotationDelta_.x = 0.0f;

			if(input.WasKeyReleased('W') || input.WasKeyReleased('w') || input.WasKeyReleased('S') ||
			    input.WasKeyReleased('s'))
				cameraWalk_.z = 0.0f;

			if(input.WasKeyReleased('A') || input.WasKeyReleased('a') || input.WasKeyReleased('D') ||
			    input.WasKeyReleased('d'))
				cameraWalk_.x = 0.0f;

			if(input.WasKeyReleased(Client::KeyCode::LSHIFT))
				moveFast_ = false;


			if(input.WasKeyPressed(Client::KeyCode::LEFT))
				cameraRotationDelta_.y = 1.0f;
			if(input.WasKeyPressed(Client::KeyCode::RIGHT))
				cameraRotationDelta_.y = -1.0f;

			if(input.WasKeyPressed(Client::KeyCode::UP))
				cameraRotationDelta_.x = -1.0f;
			if(input.WasKeyPressed(Client::KeyCode::DOWN))
				cameraRotationDelta_.x = 1.0f;

			if(input.WasKeyPressed('W') || input.WasKeyPressed('w'))
				cameraWalk_.z = 1.0f;

			if(input.WasKeyPressed('S') || input.WasKeyPressed('s'))
				cameraWalk_.z = -1.0f;

			if(input.WasKeyPressed('A') || input.WasKeyPressed('a'))
				cameraWalk_.x = -1.0f;

			if(input.WasKeyPressed('D') || input.WasKeyPressed('d'))
				cameraWalk_.x = 1.0f;

			if(input.WasKeyPressed(Client::KeyCode::LSHIFT))
				moveFast_ = true;

			Math::Vec2 mousePos = input.GetMousePosition();
			Math::Vec2 mouseDelta = oldMousePos_ - mousePos;
			oldMousePos_ = mousePos;
			switch(cameraState_)
			{
			case CameraState::IDLE:
			{
			}
			break;

			case CameraState::ROTATE:
			{
				f32 rotateSpeed = 1.0f / 200.0f;
				Math::Vec3 cameraRotateAmount =
				    Math::Vec3(mousePos.y - initialMousePos_.y, -(mousePos.x - initialMousePos_.x), 0.0f) * rotateSpeed;
				cameraRotation_ = baseCameraRotation_ + cameraRotateAmount;
			}
			break;

			case CameraState::PAN:
			{
				f32 panSpeed = 4.0f;
				Math::Mat44 cameraRotationMatrix = GetCameraRotationMatrix();
				Math::Vec3 offsetVector = Math::Vec3(mouseDelta.x, mouseDelta.y, 0.0f) * cameraRotationMatrix;
				cameraTarget_ += offsetVector * tick * panSpeed;
			}
			break;
			}

			// Rotation.
			cameraRotation_ += cameraRotationDelta_ * tick * 4.0f;

			cameraDistance_ += cameraZoom_ * tick;
			cameraDistance_ = Core::Clamp(cameraDistance_, 1.0f, 4096.0f);
			cameraZoom_ = 0.0f;

			f32 walkSpeed = moveFast_ ? 128.0f : 16.0f;
			Math::Mat44 cameraRotationMatrix = GetCameraRotationMatrix();
			Math::Vec3 offsetVector = -cameraWalk_ * cameraRotationMatrix;
			cameraTarget_ += offsetVector * tick * walkSpeed;


			Math::Vec3 viewDistance = Math::Vec3(0.0f, 0.0f, cameraDistance_);
			viewDistance = viewDistance * cameraRotationMatrix;
			Math::Vec3 viewFromPosition = cameraTarget_ + viewDistance;

			matrix_.Identity();
			matrix_.LookAt(
			    viewFromPosition, cameraTarget_, Math::Vec3(cameraRotationMatrix.Row1().x,
			                                         cameraRotationMatrix.Row1().y, cameraRotationMatrix.Row1().z));
		}

		Math::Mat44 GetCameraRotationMatrix() const
		{
			Math::Mat44 cameraPitchMatrix;
			Math::Mat44 cameraYawMatrix;
			Math::Mat44 cameraRollMatrix;
			cameraPitchMatrix.Rotation(Math::Vec3(cameraRotation_.x, 0.0f, 0.0f));
			cameraYawMatrix.Rotation(Math::Vec3(0.0f, cameraRotation_.y, 0.0f));
			cameraRollMatrix.Rotation(Math::Vec3(0.0f, 0.0f, cameraRotation_.z));
			return cameraRollMatrix * cameraPitchMatrix * cameraYawMatrix;
		}

		CameraState cameraState_ = CameraState::IDLE;
		Math::Vec3 baseCameraRotation_;
		Math::Vec3 cameraTarget_;
		Math::Vec3 cameraRotation_;
		Math::Vec3 cameraWalk_;
		Math::Vec3 cameraRotationDelta_;
		f32 cameraDistance_;
		f32 cameraZoom_;
		bool moveFast_ = false;

		Math::Vec2 initialMousePos_;
		Math::Vec2 oldMousePos_;

		Math::Mat44 matrix_;
	};

	Camera camera_;

	void DrawRenderGraphUI(RenderGraph& renderGraph)
	{
		if(i32 numRenderPasses = renderGraph.GetNumExecutedRenderPasses())
		{
			if(ImGui::Begin("Render Passes"))
			{
				ImGui::Separator();

				Core::Vector<const RenderPass*> renderPasses(numRenderPasses);
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


	Core::Mutex packetMutex_;
	Core::Vector<Testbed::RenderPacketBase*> packets_;
	Core::Vector<Testbed::ShaderTechniques> shaderTechniques_;
	i32 w_, h_;
	bool updateFrustum_ = true;
	bool clusterCulling_ = true;

	void DrawRenderPackets(GPU::CommandList& cmdList, const char* passName, const GPU::DrawState& drawState,
	    GPU::Handle fbs, GPU::Handle viewCBHandle, GPU::Handle objectSBHandle, Testbed::CustomBindFn customBindFn)
	{
		namespace Binding = GPU::Binding;

		{
			Core::ScopedMutex lock(packetMutex_);
			Testbed::SortPackets(packets_);
		}

		rmt_ScopedCPUSample(DrawRenderPackets, RMTSF_None);
		if(auto event = cmdList.Eventf(0, "DrawRenderPackets(\"%s\")", passName))
		{
			// Gather mesh packets for this pass.
			Core::Vector<Testbed::MeshRenderPacket*> meshPackets;
			Core::Vector<i32> meshPassTechIndices;
			meshPackets.reserve(packets_.size());
			meshPassTechIndices.reserve(packets_.size());
			for(auto& packet : packets_)
			{
				if(packet->type_ == Testbed::MeshRenderPacket::TYPE)
				{
					auto* meshPacket = static_cast<Testbed::MeshRenderPacket*>(packet);
					auto passIdxIt = meshPacket->techs_->passIndices_.find(passName);
					if(passIdxIt != meshPacket->techs_->passIndices_.end() &&
					    passIdxIt->second < meshPacket->techs_->passTechniques_.size())
					{
						meshPackets.push_back(meshPacket);
						meshPassTechIndices.push_back(passIdxIt->second);
					}
				}
			}

			Testbed::MeshRenderPacket::DrawPackets(
			    meshPackets, meshPassTechIndices, cmdList, drawState, fbs, viewCBHandle, objectSBHandle, customBindFn);
		}
	}

	void DrawUIGraphicsDebug(Testbed::ForwardPipeline& forwardPipeline)
	{
		if(ImGui::Begin("Graphics Debug"))
		{
			if(ImGui::Button("Launch RenderDoc"))
				GPU::Manager::OpenDebugCapture();
			if(ImGui::Button("Launch RenderDoc & Quit"))
				GPU::Manager::OpenDebugCapture(true);

			if(ImGui::Button("Trigger RenderDoc Capture"))
				GPU::Manager::TriggerDebugCapture();

			ImGui::Checkbox("Update Frustum", &updateFrustum_);
			ImGui::Checkbox("Cluster Culling", &clusterCulling_);

			static int debugMode = 0;
			ImGui::Text("Debug Modes:");
			ImGui::RadioButton("Off", &debugMode, 0);
			ImGui::RadioButton("Light Culling", &debugMode, 1);

			forwardPipeline.debugMode_ = (Testbed::ForwardPipeline::DebugMode)debugMode;
		}
		ImGui::End();
	}
}

void Loop(const Core::CommandLine& cmdLine)
{
	ScopedEngine engine("Test Bed App", cmdLine);
	ImGui::Manager::Scoped imgui;
	Testbed::ImGuiPipeline imguiPipeline;
	Testbed::ForwardPipeline forwardPipeline;
	Testbed::ShadowPipeline shadowPipeline;
	RenderGraph graph;

	MaterialRef testMaterial("test_material.material");
	testMaterial.WaitUntilReady();

	TextureCompressor texCompressor;

	Texture* texture = nullptr;
	Resource::Manager::RequestResource(texture, "test_texture_compress.png");

	// Load shader + teapot model.
	Shader* shader = nullptr;
	Resource::Manager::RequestResource(shader, "shaders/simple-mesh.esf");

	Model* model = nullptr;
	Resource::Manager::RequestResource(model, "model_tests/cube.obj");

	Model* sponzaModel = nullptr;

#if LOAD_SPONZA
	ClusteredModel* testClusteredModel = new ClusteredModel("model_tests/teapot.obj");
#else
	ClusteredModel* testClusteredModel = new ClusteredModel("model_tests/crytek-sponza/sponza.obj");
#endif

#if LOAD_SPONZA
	Resource::Manager::RequestResource(sponzaModel, "model_tests/crytek-sponza/sponza.obj");
#endif

	Resource::Manager::WaitForResource(texture);
	Resource::Manager::WaitForResource(shader);
	Resource::Manager::WaitForResource(model);
#if LOAD_SPONZA
	Resource::Manager::WaitForResource(sponzaModel);
#endif

	GPU::TextureDesc finalTextureDesc = texture->GetDesc();
	finalTextureDesc.format_ = GPU::Format::BC5_UNORM;
	GPU::Handle finalTexture = GPU::Manager::CreateTexture(finalTextureDesc, nullptr, "finalCompressed");
	DBG_ASSERT(finalTexture);

	// Create some render packets.
	// For now, they can be permenent.
	f32 angle = 0.0;
	const Math::Vec3 positions[] = {
	    Math::Vec3(-10.0f, 10.0f, -5.0f), Math::Vec3(-5.0f, 10.0f, -5.0f), Math::Vec3(0.0f, 10.0f, -5.0f),
	    Math::Vec3(5.0f, 10.0f, -5.0f), Math::Vec3(10.0f, 10.0f, -5.0f), Math::Vec3(-10.0f, 10.0f, 5.0f),
	    Math::Vec3(-5.0f, 10.0f, 5.0f), Math::Vec3(0.0f, 10.0f, 5.0f), Math::Vec3(5.0f, 10.0f, 5.0f),
	    Math::Vec3(10.0f, 10.0f, 5.0f),
	};

	Core::Random rng;

	Testbed::Light light = {};

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

	if(true)
	{
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
	}

#if 0
	for(i32 idx = 0; idx < 1000; ++idx)
	{
		light.position_.x = randF32(-100.0f, 100.0f);
		light.position_.y = randF32(0.0f, 50.0f);
		light.position_.z = randF32(-100.0f, 100.0f);
		light.color_.x = randF32(0.0f, 1.0f);
		light.color_.y = randF32(0.0f, 1.0f);
		light.color_.z = randF32(0.0f, 1.0f);
		light.color_ *= 200.0f;
		light.radiusInner_ = 10.0f;
		light.radiusOuter_ = 15.0f;
		forwardPipeline.lights_.push_back(light);
	}

	for(i32 idx = 0; idx < 2000; ++idx)
	{
		light.position_.x = randF32(-100.0f, 100.0f);
		light.position_.y = randF32(0.0f, 50.0f);
		light.position_.z = randF32(-100.0f, 100.0f);
		light.color_.x = randF32(0.0f, 1.0f);
		light.color_.y = randF32(0.0f, 1.0f);
		light.color_.z = randF32(0.0f, 1.0f);
		light.color_ *= 5.0f;
		light.radiusInner_ = 1.0f;
		light.radiusOuter_ = 2.0f;
		forwardPipeline.lights_.push_back(light);
	}
#endif

	shaderTechniques_.reserve(10000);
	Testbed::ShaderTechniques* techniques = &*shaderTechniques_.emplace_back();
	if(!LOAD_SPONZA)
	{
		{
			for(const auto& position : positions)
			{
				for(i32 idx = 0; idx < model->GetNumMeshes(); ++idx)
				{
					ShaderTechniqueDesc techDesc;
					techDesc.SetVertexElements(model->GetMeshVertexElements(idx));
					techDesc.SetTopology(GPU::TopologyType::TRIANGLE);

					Testbed::MeshRenderPacket packet;
					packet.db_ = model->GetMeshDrawBinding(idx);
					packet.draw_ = model->GetMeshDraw(idx);
					packet.techDesc_ = techDesc;
					packet.material_ = model->GetMeshMaterial(idx);
					packet.techs_ = techniques;

					packet.world_ = model->GetMeshWorldTransform(idx);
					packet.angle_ = angle;
					packet.position_ = position;

					angle += 0.5f;

					packets_.emplace_back(new Testbed::MeshRenderPacket(packet));
				}
			}
		}
	}

	if(sponzaModel)
	{
		for(i32 idx = 0; idx < sponzaModel->GetNumMeshes(); ++idx)
		{
			ShaderTechniqueDesc techDesc;
			techDesc.SetVertexElements(sponzaModel->GetMeshVertexElements(idx));
			techDesc.SetTopology(GPU::TopologyType::TRIANGLE);

			Testbed::MeshRenderPacket packet;
			packet.db_ = sponzaModel->GetMeshDrawBinding(idx);
			packet.draw_ = sponzaModel->GetMeshDraw(idx);
			packet.techDesc_ = techDesc;
			packet.material_ = sponzaModel->GetMeshMaterial(idx);

			if(techniques->material_ != packet.material_)
				techniques = &*shaderTechniques_.emplace_back();

			packet.techs_ = techniques;

			packet.world_ = sponzaModel->GetMeshWorldTransform(idx);
			Math::Mat44 scale;
			scale.Scale(Math::Vec3(0.1f, 0.1f, 0.1f));
			packet.world_ = scale * packet.world_;
			packet.angle_ = 0.0f;
			packet.position_ = Math::Vec3(0.0f, 0.0f, 0.0f);

			packets_.emplace_back(new Testbed::MeshRenderPacket(packet));
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

	Job::FunctionJob frameSubmitJob("Frame Submit", [&](i32) {
		// Execute, and resolve the out color target.
		times_.graphExecute_ = Core::Timer::GetAbsoluteTime();
		graph.Execute(imguiPipeline.GetResource("out_color"));
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

	while(Client::Manager::Update())
	{
		const f64 targetFrameTime = 1.0 / 120.0;
		f64 beginFrameTime = Core::Timer::GetAbsoluteTime();

		{
			rmt_ScopedCPUSample(Update, RMTSF_None);

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
					if(packet->type_ == Testbed::MeshRenderPacket::TYPE)
					{
						auto* meshPacket = static_cast<Testbed::MeshRenderPacket*>(packet);

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
			forwardPipeline.SetDrawCallback(
			    [&](GPU::CommandList& cmdList, const char* passName, const GPU::DrawState& drawState, GPU::Handle fbs,
			        GPU::Handle viewCBHandle, GPU::Handle objectSBHandle, Testbed::CustomBindFn customBindFn) {
				    DrawRenderPackets(cmdList, passName, drawState, fbs, viewCBHandle, objectSBHandle, customBindFn);

				    if(testClusteredModel)
				    {
					    testClusteredModel->enableCulling_ = clusterCulling_;

#if LOAD_SPONZA
					    for(auto position : positions)
#else
					    Math::Vec3 position(0.0f, 0.0f, 0.0f);
					    Math::Mat44 scale;
					    scale.Scale(Math::Vec3(0.1f, 0.1f, 0.1f));
#endif
					    {
						    Testbed::ObjectConstants object;
						    object.world_.Rotation(Math::Vec3(0.0f, 0.0f, 0.0f));
						    object.world_.Translation(position);

#if !LOAD_SPONZA
						    object.world_ = object.world_ * scale;
#endif
						    testClusteredModel->DrawClusters(
						        cmdList, passName, drawState, fbs, viewCBHandle, objectSBHandle, customBindFn, object);
					    }
				    }

				    // Testing code.
				    //texCompressor.Compress(cmdList, texture, finalTextureDesc.format_, finalTexture);
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
				}

				// Setup ImGui pipeline.
				{
					rmt_ScopedCPUSample(Setup_ImGuiPipeline, RMTSF_None);

					imguiPipeline.SetResource("in_color", forwardPipeline.GetResource("out_color"));
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
					if(packet->type_ == Testbed::MeshRenderPacket::TYPE)
					{
						auto* meshPacket = static_cast<Testbed::MeshRenderPacket*>(packet);
						forwardPipeline.CreateTechniques(
						    meshPacket->material_, meshPacket->techDesc_, *meshPacket->techs_);
					}
				}

				for(auto& techs : testClusteredModel->techs_)
				{
					forwardPipeline.CreateTechniques(techs.material_, testClusteredModel->techDesc_, techs);
				}
			}


			times_.shaderTechniqueSetup_ = Core::Timer::GetAbsoluteTime() - times_.shaderTechniqueSetup_;

			// Schedule frame submit job.
			{
				rmt_ScopedCPUSample(FrameSubmit, RMTSF_None);
				frameSubmitJob.RunSingle(0, &frameSubmitCounter);
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
		if(packet->type_ == Testbed::MeshRenderPacket::TYPE)
		{
			auto* meshPacket = static_cast<Testbed::MeshRenderPacket*>(packet);
			delete meshPacket;
		}
	}

	delete testClusteredModel;

	GPU::Manager::DestroyResource(finalTexture);

	shaderTechniques_.clear();
	Resource::Manager::ReleaseResource(shader);
	Resource::Manager::ReleaseResource(model);
	Resource::Manager::ReleaseResource(texture);
#if LOAD_SPONZA
	Resource::Manager::ReleaseResource(sponzaModel);
#endif
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

	Core::CommandLine cmdLine(argc, argv);
	Loop(cmdLine);

	return 0;
}
