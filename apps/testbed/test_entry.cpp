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
#include "imgui/manager.h"
#include "job/function_job.h"
#include "test_shared.h"
#include "math/mat44.h"
#include "math/plane.h"

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

			f32 walkSpeed = moveFast_ ? 32.0f : 8.0f;
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

	static const char* IMGUI_RESOURCE_NAMES[] = {"in_color", "out_color", nullptr};

	class ImGuiPipeline : public Pipeline
	{
	public:
		ImGuiPipeline()
		    : Pipeline(IMGUI_RESOURCE_NAMES)
		{
		}

		virtual ~ImGuiPipeline() {}

		void Setup(RenderGraph& renderGraph) override
		{
			struct ImGuiPassData
			{
				RenderGraphResource outColor_;
			};

			auto& pass = renderGraph.AddCallbackRenderPass<ImGuiPassData>("ImGui Pass",
			    [&](RenderGraphBuilder& builder, ImGuiPassData& data) {
				    data.outColor_ = builder.SetRTV(0, resources_[0]);
				},

			    [](RenderGraphResources& res, GPU::CommandList& cmdList, const ImGuiPassData& data) {
				    auto fbs = res.GetFrameBindingSet();
				    ImGui::Manager::Render(fbs, cmdList);
				});

			resources_[1] = pass.GetData().outColor_;
		}

		bool HaveExecuteErrors() const override { return false; }
	};


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

	struct ShaderTechniques
	{
		Core::Map<Core::String, i32> passIndices_;
		Core::Vector<ShaderTechnique> passTechniques_;
	};

	struct ObjectConstants
	{
		Math::Mat44 world_;
	};

	struct ViewConstants
	{
		Math::Mat44 view_;
		Math::Mat44 proj_;
		Math::Mat44 viewProj_;
		Math::Mat44 invProj_;
		Math::Vec2 screenDimensions_;
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
		ModelMeshDraw draw_;
		ObjectConstants object_;
		ShaderTechniqueDesc techDesc_;
		Shader* shader_ = nullptr;
		ShaderTechniques* techs_ = nullptr;

		// tmp object state data.
		f32 angle_ = 0.0f;
		Math::Vec3 position_;
	};

	Core::Vector<RenderPacketBase*> packets_;
	Core::Vector<ShaderTechniques> shaderTechniques_;
	i32 w_, h_;

	using CustomBindFn = Core::Function<bool(Shader*, ShaderTechnique&)>;
	void DrawRenderPackets(GPU::CommandList& cmdList, const char* passName, const GPU::DrawState& drawState,
	    GPU::Handle fbs, GPU::Handle viewCBHandle, GPU::Handle objectSBHandle, CustomBindFn customBindFn)
	{
		namespace Binding = GPU::Binding;

		rmt_ScopedCPUSample(DrawRenderPackets, RMTSF_None);
		if(auto event = cmdList.Eventf(0, "DrawRenderPackets(\"%s\")", passName))
		{
			i32 totalObjects = 0;

			// Count up packets.
			{
				for(auto& packet : packets_)
				{
					if(packet->type_ == MeshRenderPacket::TYPE)
					{
						auto* meshPacket = static_cast<MeshRenderPacket*>(packet);
						auto passIdxIt = meshPacket->techs_->passIndices_.find(passName);
						if(passIdxIt != meshPacket->techs_->passIndices_.end() &&
						    passIdxIt->second < meshPacket->techs_->passTechniques_.size())
						{
							totalObjects++;
						}
					}
				}
			}

			// Allocate command list memory.
			auto* objects = cmdList.Alloc<ObjectConstants>(totalObjects);

			// Update all render packet uniforms.
			{
				i32 objectSBOffset = 0;
				for(auto& packet : packets_)
				{
					if(packet->type_ == MeshRenderPacket::TYPE)
					{
						auto* meshPacket = static_cast<MeshRenderPacket*>(packet);
						auto passIdxIt = meshPacket->techs_->passIndices_.find(passName);
						if(passIdxIt != meshPacket->techs_->passIndices_.end() &&
						    passIdxIt->second < meshPacket->techs_->passTechniques_.size())
						{
							objects[objectSBOffset++] = meshPacket->object_;
						}
					}
				}
				cmdList.UpdateBuffer(objectSBHandle, 0, sizeof(ObjectConstants) * totalObjects, objects);
			}

			// Draw all packets.
			{
				i32 objectSBOffset = 0;
				for(auto& packet : packets_)
				{
					if(packet->type_ == MeshRenderPacket::TYPE)
					{
						auto* meshPacket = static_cast<MeshRenderPacket*>(packet);
						auto passIdxIt = meshPacket->techs_->passIndices_.find(passName);
						if(passIdxIt != meshPacket->techs_->passIndices_.end() &&
						    passIdxIt->second < meshPacket->techs_->passTechniques_.size())
						{
							auto& tech = meshPacket->techs_->passTechniques_[passIdxIt->second];
							const i32 objectDataSize = sizeof(meshPacket->object_);
							tech.Set("inObject", Binding::Buffer(objectSBHandle, GPU::Format::INVALID, objectSBOffset,
							                         1, objectDataSize));

							bool doDraw = true;
							if(customBindFn)
								doDraw = customBindFn(meshPacket->shader_, tech);

							if(doDraw)
							{
								tech.Set("ViewCBuffer", Binding::CBuffer(viewCBHandle, 0, sizeof(ViewConstants)));
								if(auto pbs = tech.GetBinding())
								{
									cmdList.Draw(pbs, meshPacket->db_, fbs, drawState,
									    GPU::PrimitiveTopology::TRIANGLE_LIST, meshPacket->draw_.indexOffset_,
									    meshPacket->draw_.vertexOffset_, meshPacket->draw_.noofIndices_, 0, 1);
								}
							}

							++objectSBOffset;
						}
					}
				}
			}
		}
	}

	const RenderGraphBufferDesc objectSBDesc = RenderGraphBufferDesc(sizeof(ObjectConstants) * 1000);

	struct Light
	{
		u32 enabled_ = 0;
		Math::Vec3 position_ = Math::Vec3(0.0f, 0.0f, 0.0f);
		Math::Vec3 color_ = Math::Vec3(0.0f, 0.0f, 0.0f);
		Math::Vec3 attenuation_ = Math::Vec3(1.0f, 0.2f, 0.1f);
		float intensity_ = 0.0f;
	};

	struct LightLink
	{
		i32 lightIdx_;
		i32 next_;
	};

	struct LightConstants
	{
		u32 tileSizeX_;
		u32 tileSizeY_;
		u32 numTilesX_;
		u32 numTilesY_;
		u32 numLights_;
	};

	struct TileInfo
	{
		Math::Plane planes_[4];
		Math::Vec4 depthMinMax_;
	};

	Core::Vector<Light> lights_;
	i32 lightBufferSize_ = 64 * 1024;

	struct CommonBuffers
	{
		RenderGraphResource viewCB_;
		RenderGraphResource objectSB_;
	};

	struct LightCullingData
	{
		RenderGraphResource outLightCB_;
		RenderGraphResource outLightSB_;
		RenderGraphResource outLightLinkSB_;
		RenderGraphResource outDebug_;
	};

	LightCullingData AddLightCullingPasses(
	    RenderGraph& renderGraph, const CommonBuffers& cbs, RenderGraphResource depth, Shader* shader)
	{
		// Grab depth stencil descriptor.
		RenderGraphTextureDesc dsDesc;
		renderGraph.GetTexture(depth, &dsDesc);

		// Setup light constants.
		LightConstants light;
		light.numLights_ = lights_.size();
		light.tileSizeX_ = 16;
		light.tileSizeY_ = 16;
		light.numTilesX_ = dsDesc.width_ / light.tileSizeX_;
		light.numTilesY_ = dsDesc.height_ / light.tileSizeY_;

		// Data for passing between render passes.
		struct UpdateLightsPassData
		{
			LightConstants light_;
			Light* lights_ = nullptr;

			RenderGraphResource outLightCB_, outLightSB_, outLightLinkIndexSB_;
		};

		struct ComputeTileInfoPassData
		{
			LightConstants light_;
			GPU::Format depthFormat_;

			RenderGraphResource inViewCB_, inLightCB_, inDepth_;
			RenderGraphResource outTileInfoSB_;

			mutable ShaderTechnique tech_;
		};

		struct ComputeLightListsPassData
		{
			LightConstants light_;

			RenderGraphResource inViewCB_, inLightCB_, inLightSB_, inTileInfoSB_;
			RenderGraphResource outLightLinkSB_, outLightLinkIndexSB_;

			mutable ShaderTechnique tech_;
		};

		struct DebugOutputPassData
		{
			LightConstants light_;

			RenderGraphResource inViewCB_, inLightCB_, inLightSB_, inTileInfoSB_, inLightLinkSB_;
			RenderGraphResource outDebug_;

			mutable ShaderTechnique tech_;
		};

		auto updateLightsPassData =
		    renderGraph
		        .AddCallbackRenderPass<UpdateLightsPassData>("Update Light Buffers",
		            [&](RenderGraphBuilder& builder, UpdateLightsPassData& data) {
			            data.light_ = light;
			            data.lights_ = builder.Push(lights_.data(), lights_.size());

			            data.outLightCB_ =
			                builder.Write(builder.Create("LC LightCB", RenderGraphBufferDesc(sizeof(LightConstants))));
			            data.outLightSB_ = builder.Write(
			                builder.Create("LC LightSB", RenderGraphBufferDesc(sizeof(Light) * light.numLights_)));
			        },
		            [](RenderGraphResources& res, GPU::CommandList& cmdList, const UpdateLightsPassData& data) {
			            cmdList.UpdateBuffer(res.GetBuffer(data.outLightCB_), 0, sizeof(data.light_), &data.light_);
			            cmdList.UpdateBuffer(
			                res.GetBuffer(data.outLightSB_), 0, sizeof(Light) * data.light_.numLights_, data.lights_);

			        })
		        .GetData();

		auto& computeTileInfoPassData =
		    renderGraph
		        .AddCallbackRenderPass<ComputeTileInfoPassData>("Compute Tile Info",
		            [&](RenderGraphBuilder& builder, ComputeTileInfoPassData& data) {
			            data.light_ = light;
			            data.depthFormat_ = GPU::GetSRVFormatDepth(dsDesc.format_);

			            data.inViewCB_ = builder.Read(cbs.viewCB_, GPU::BindFlags::CONSTANT_BUFFER);
			            data.inLightCB_ =
			                builder.Read(updateLightsPassData.outLightCB_, GPU::BindFlags::CONSTANT_BUFFER);
			            data.inDepth_ = builder.Read(depth, GPU::BindFlags::SHADER_RESOURCE);

			            data.outTileInfoSB_ = builder.Write(
			                builder.Create("LC Tile Info SB",
			                    RenderGraphBufferDesc(sizeof(TileInfo) * light.numTilesX_ * light.numTilesY_)),
			                GPU::BindFlags::UNORDERED_ACCESS);

			            data.tech_ = shader->CreateTechnique("TECH_COMPUTE_TILE_INFO", ShaderTechniqueDesc());
			        },
		            [](RenderGraphResources& res, GPU::CommandList& cmdList, const ComputeTileInfoPassData& data) {
			            data.tech_.Set("ViewCBuffer", res.CBuffer(data.inViewCB_, 0, sizeof(ViewConstants)));
			            data.tech_.Set("LightCBuffer", res.CBuffer(data.inLightCB_, 0, sizeof(LightConstants)));
			            data.tech_.Set("outTileInfo",
			                res.RWBuffer(data.outTileInfoSB_, GPU::Format::INVALID, 0,
			                    data.light_.numTilesX_ * data.light_.numTilesY_, sizeof(TileInfo)));
			            data.tech_.Set("depthTex", res.Texture2D(data.inDepth_, data.depthFormat_, 0, 1));
			            if(auto binding = data.tech_.GetBinding())
			            {
				            cmdList.Dispatch(binding, data.light_.numTilesX_, data.light_.numTilesY_, 1);
			            }
			        })
		        .GetData();

		auto& computeLightListsPassData =
		    renderGraph
		        .AddCallbackRenderPass<ComputeLightListsPassData>("Compute Light Lists",
		            [&](RenderGraphBuilder& builder, ComputeLightListsPassData& data) {
			            data.light_ = light;

			            data.inViewCB_ = builder.Read(cbs.viewCB_, GPU::BindFlags::CONSTANT_BUFFER);
			            data.inLightCB_ =
			                builder.Read(updateLightsPassData.outLightCB_, GPU::BindFlags::CONSTANT_BUFFER);
			            data.inLightSB_ =
			                builder.Read(updateLightsPassData.outLightSB_, GPU::BindFlags::SHADER_RESOURCE);
			            data.inTileInfoSB_ =
			                builder.Read(computeTileInfoPassData.outTileInfoSB_, GPU::BindFlags::SHADER_RESOURCE);

			            data.outLightLinkIndexSB_ =
			                builder.Write(builder.Create("LC Light Link Index SB", RenderGraphBufferDesc(sizeof(u32))),
			                    GPU::BindFlags::UNORDERED_ACCESS);
			            data.outLightLinkSB_ =
			                builder.Write(builder.Create("LC Light Link SB",
			                                  RenderGraphBufferDesc(sizeof(LightLink) * lightBufferSize_)),
			                    GPU::BindFlags::UNORDERED_ACCESS);

			            data.tech_ = shader->CreateTechnique("TECH_COMPUTE_LIGHT_LISTS", ShaderTechniqueDesc());
			        },
		            [](RenderGraphResources& res, GPU::CommandList& cmdList, const ComputeLightListsPassData& data) {
			            i32 baseLightIndex = data.light_.numTilesX_ * data.light_.numTilesY_;
			            cmdList.UpdateBuffer(
			                res.GetBuffer(data.outLightLinkIndexSB_), 0, sizeof(u32), cmdList.Push(&baseLightIndex));

			            data.tech_.Set("ViewCBuffer", res.CBuffer(data.inViewCB_, 0, sizeof(ViewConstants)));
			            data.tech_.Set("LightCBuffer", res.CBuffer(data.inLightCB_, 0, sizeof(LightConstants)));
			            data.tech_.Set("inTileInfo",
			                res.Buffer(data.inTileInfoSB_, GPU::Format::INVALID, 0,
			                    data.light_.numTilesX_ * data.light_.numTilesY_, sizeof(TileInfo)));
			            data.tech_.Set("inLights",
			                res.Buffer(
			                    data.inLightSB_, GPU::Format::INVALID, 0, data.light_.numLights_, sizeof(Light)));
			            data.tech_.Set("lightLinkIndex",
			                res.RWBuffer(data.outLightLinkIndexSB_, GPU::Format::R32_TYPELESS, 0, sizeof(u32), 0));
			            data.tech_.Set("outLightLinks",
			                res.RWBuffer(
			                    data.outLightLinkSB_, GPU::Format::INVALID, 0, lightBufferSize_, sizeof(LightLink)));

			            if(auto binding = data.tech_.GetBinding())
			            {
				            cmdList.Dispatch(binding, data.light_.numTilesX_, data.light_.numTilesY_, 1);
			            }
			        })
		        .GetData();

		auto& debugOutputPassData =
		    renderGraph
		        .AddCallbackRenderPass<DebugOutputPassData>("Debug Light Output",
		            [&](RenderGraphBuilder& builder, DebugOutputPassData& data) {
			            data.light_ = light;

			            data.inViewCB_ = builder.Read(cbs.viewCB_, GPU::BindFlags::CONSTANT_BUFFER);
			            data.inLightCB_ =
			                builder.Read(updateLightsPassData.outLightCB_, GPU::BindFlags::CONSTANT_BUFFER);
			            data.inLightSB_ =
			                builder.Read(updateLightsPassData.outLightSB_, GPU::BindFlags::SHADER_RESOURCE);
			            data.inTileInfoSB_ =
			                builder.Read(computeTileInfoPassData.outTileInfoSB_, GPU::BindFlags::SHADER_RESOURCE);
			            data.inLightLinkSB_ =
			                builder.Read(computeLightListsPassData.outLightLinkSB_, GPU::BindFlags::SHADER_RESOURCE);

			            data.outDebug_ =
			                builder.Write(builder.Create("LC Debug Tile Info",
			                                  RenderGraphTextureDesc(GPU::TextureType::TEX2D,
			                                      GPU::Format::R32G32B32A32_FLOAT, light.numTilesX_, light.numTilesY_)),
			                    GPU::BindFlags::UNORDERED_ACCESS);

			            data.tech_ = shader->CreateTechnique("TECH_DEBUG_TILE_INFO", ShaderTechniqueDesc());
			        },
		            [](RenderGraphResources& res, GPU::CommandList& cmdList, const DebugOutputPassData& data) {
			            data.tech_.Set("ViewCBuffer", res.CBuffer(data.inViewCB_, 0, sizeof(ViewConstants)));
			            data.tech_.Set("LightCBuffer", res.CBuffer(data.inLightCB_, 0, sizeof(LightConstants)));
			            data.tech_.Set("inTileInfo",
			                res.Buffer(data.inTileInfoSB_, GPU::Format::INVALID, 0,
			                    data.light_.numTilesX_ * data.light_.numTilesY_, sizeof(TileInfo)));
			            data.tech_.Set("inLights",
			                res.Buffer(
			                    data.inLightSB_, GPU::Format::INVALID, 0, data.light_.numLights_, sizeof(Light)));
			            data.tech_.Set("inLightLinks",
			                res.Buffer(
			                    data.inLightLinkSB_, GPU::Format::INVALID, 0, lightBufferSize_, sizeof(LightLink)));
			            data.tech_.Set("outDebug", res.RWTexture2D(data.outDebug_, GPU::Format::R32G32B32A32_FLOAT));

			            if(auto binding = data.tech_.GetBinding())
			            {
				            cmdList.Dispatch(binding, data.light_.numTilesX_, data.light_.numTilesY_, 1);
			            }
			        })
		        .GetData();

		// Setup output.
		LightCullingData output;
		output.outLightCB_ = updateLightsPassData.outLightCB_;
		output.outLightSB_ = updateLightsPassData.outLightSB_;
		output.outLightLinkSB_ = computeLightListsPassData.outLightLinkSB_;
		output.outDebug_ = debugOutputPassData.outDebug_;
		return output;
	}

	struct DepthData
	{
		RenderGraphResource outDepth_;
		RenderGraphResource outObjectSB_;

		GPU::FrameBindingSetDesc fbsDesc_;
	};

	DepthData AddDepthPasses(RenderGraph& renderGraph, const CommonBuffers& cbs,
	    const RenderGraphTextureDesc& depthDesc, RenderGraphResource depth, RenderGraphResource objectSB)
	{
		struct ForwardPassData
		{
			GPU::DrawState drawState_;

			RenderGraphResource inViewCB_;

			RenderGraphResource outDepth_;
			RenderGraphResource outObjectSB_;
		};

		auto& pass = renderGraph.AddCallbackRenderPass<ForwardPassData>("Depth Pass",
		    [&](RenderGraphBuilder& builder, ForwardPassData& data) {

			    data.drawState_.scissorRect_.w_ = depthDesc.width_;
			    data.drawState_.scissorRect_.h_ = depthDesc.height_;
			    data.drawState_.viewport_.w_ = (f32)depthDesc.width_;
			    data.drawState_.viewport_.h_ = (f32)depthDesc.height_;

			    // Create depth target if none are provided.
			    if(!depth)
				    depth = builder.Create("Depth", depthDesc);

			    data.inViewCB_ = builder.Read(cbs.viewCB_, GPU::BindFlags::CONSTANT_BUFFER);

			    // Object buffer.
			    DBG_ASSERT(objectSB);
			    data.outObjectSB_ = builder.Write(objectSB, GPU::BindFlags::SHADER_RESOURCE);

			    // Setup frame buffer.
			    data.outDepth_ = builder.SetDSV(depth);
			},

		    [](RenderGraphResources& res, GPU::CommandList& cmdList, const ForwardPassData& data) {
			    auto fbs = res.GetFrameBindingSet();

			    // Clear depth buffer.
			    cmdList.ClearDSV(fbs, 1.0f, 0);

			    // Draw all render packets that are valid for this pass.
			    DrawRenderPackets(cmdList, "RenderPassDepthPrepass", data.drawState_, fbs,
			        res.GetBuffer(data.inViewCB_), res.GetBuffer(data.outObjectSB_), nullptr);
			});

		DepthData output;
		output.outDepth_ = pass.GetData().outDepth_;
		output.outObjectSB_ = pass.GetData().outObjectSB_;
		output.fbsDesc_ = pass.GetFrameBindingDesc();
		return output;
	}

	struct ForwardData
	{
		RenderGraphResource outColor_;
		RenderGraphResource outDepth_;
		RenderGraphResource outObjectSB_;

		GPU::FrameBindingSetDesc fbsDesc_;
	};

	ForwardData AddForwardPasses(RenderGraph& renderGraph, const CommonBuffers& cbs, RenderGraphResource lightCB,
	    RenderGraphResource lightSB, RenderGraphResource lightLinkSB, const RenderGraphTextureDesc& colorDesc,
	    RenderGraphResource color, const RenderGraphTextureDesc& depthDesc, RenderGraphResource depth,
	    RenderGraphResource objectSB)
	{
		struct ForwardPassData
		{
			GPU::DrawState drawState_;

			RenderGraphResource inViewCB_;
			RenderGraphResource inLightCB_;
			RenderGraphResource inLightSB_;
			RenderGraphResource inLightLinkSB_;

			RenderGraphResource outColor_;
			RenderGraphResource outDepth_;
			RenderGraphResource outObjectSB_;
		};

		auto& pass = renderGraph.AddCallbackRenderPass<ForwardPassData>("Forward Pass",
		    [&](RenderGraphBuilder& builder, ForwardPassData& data) {

			    data.drawState_.scissorRect_.w_ = colorDesc.width_;
			    data.drawState_.scissorRect_.h_ = colorDesc.height_;
			    data.drawState_.viewport_.w_ = (f32)colorDesc.width_;
			    data.drawState_.viewport_.h_ = (f32)colorDesc.height_;

			    // Create color & depth targets if none are provided.
			    if(!color)
				    color = builder.Create("Color", colorDesc);
			    if(!depth)
				    depth = builder.Create("Depth", depthDesc);

			    data.inViewCB_ = builder.Read(cbs.viewCB_, GPU::BindFlags::CONSTANT_BUFFER);
			    data.inLightCB_ = builder.Read(lightCB, GPU::BindFlags::CONSTANT_BUFFER);
			    data.inLightSB_ = builder.Read(lightSB, GPU::BindFlags::SHADER_RESOURCE);
			    data.inLightLinkSB_ = builder.Read(lightLinkSB, GPU::BindFlags::SHADER_RESOURCE);

			    // Object buffer.
			    DBG_ASSERT(objectSB);
			    data.outObjectSB_ = builder.Write(objectSB, GPU::BindFlags::SHADER_RESOURCE);

			    // Setup frame buffer.
			    data.outColor_ = builder.SetRTV(0, color);
			    data.outDepth_ = builder.SetDSV(depth);
			},
		    [](RenderGraphResources& res, GPU::CommandList& cmdList, const ForwardPassData& data) {
			    auto fbs = res.GetFrameBindingSet();

			    // Clear color buffer.
			    f32 color[] = {0.1f, 0.1f, 0.2f, 1.0f};
			    cmdList.ClearRTV(fbs, 0, color);

			    // Draw all render packets.
			    DrawRenderPackets(cmdList, "RenderPassForward", data.drawState_, fbs, res.GetBuffer(data.inViewCB_),
			        res.GetBuffer(data.outObjectSB_), [&](Shader* shader, ShaderTechnique& tech) {
				        tech.Set("LightCBuffer", res.CBuffer(data.inLightCB_, 0, sizeof(LightConstants)));
				        tech.Set("inLights",
				            res.Buffer(data.inLightSB_, GPU::Format::INVALID, 0, lights_.size(), sizeof(Light)));
				        tech.Set("inLightLinks",
				            res.Buffer(
				                data.inLightLinkSB_, GPU::Format::INVALID, 0, lightBufferSize_, sizeof(LightLink)));
				        return true;
				    });
			});

		ForwardData output;
		output.outColor_ = pass.GetData().outColor_;
		output.outDepth_ = pass.GetData().outDepth_;
		output.outObjectSB_ = pass.GetData().outObjectSB_;
		output.fbsDesc_ = pass.GetFrameBindingDesc();
		return output;
	}

	struct FullscreenData
	{
		RenderGraphResource outColor_;

		GPU::FrameBindingSetDesc fbsDesc_;
	};

	using FullscreenSetupFn = Core::Function<void(RenderGraphBuilder&)>;
	using FullscreenBindFn = Core::Function<void(RenderGraphResources&, Shader*, ShaderTechnique&)>;

	FullscreenData AddFullscreenPass(RenderGraph& renderGraph, const CommonBuffers& cbs, RenderGraphResource color,
	    Shader* shader, FullscreenSetupFn setupFn, FullscreenBindFn bindFn)
	{
		struct FullscreenPassData
		{
			FullscreenBindFn bindFn_;
			Shader* shader_;
			GPU::DrawState drawState_;

			RenderGraphResource inViewCB_;

			RenderGraphResource outColor_;
		};

		auto& pass = renderGraph.AddCallbackRenderPass<FullscreenPassData>("Depth Pass",
		    [&](RenderGraphBuilder& builder, FullscreenPassData& data) {
			    data.bindFn_ = bindFn;
			    data.shader_ = shader;

			    setupFn(builder);

			    RenderGraphTextureDesc colorDesc;
			    builder.GetTexture(color, &colorDesc);
			    data.drawState_.scissorRect_.w_ = colorDesc.width_;
			    data.drawState_.scissorRect_.h_ = colorDesc.height_;
			    data.drawState_.viewport_.w_ = (f32)colorDesc.width_;
			    data.drawState_.viewport_.h_ = (f32)colorDesc.height_;

			    data.inViewCB_ = builder.Read(cbs.viewCB_, GPU::BindFlags::CONSTANT_BUFFER);

			    // Setup frame buffer.
			    data.outColor_ = builder.SetRTV(0, color);

			},

		    [](RenderGraphResources& res, GPU::CommandList& cmdList, const FullscreenPassData& data) {
			    GPU::FrameBindingSetDesc fbsDesc;
			    auto fbs = res.GetFrameBindingSet(&fbsDesc);

			    ShaderTechniqueDesc techDesc;
			    techDesc.SetFrameBindingSet(fbsDesc);
			    techDesc.SetTopology(GPU::TopologyType::TRIANGLE);
			    auto tech = data.shader_->CreateTechnique("TECH_FULLSCREEN", techDesc);

			    data.bindFn_(res, data.shader_, tech);
			    if(auto binding = tech.GetBinding())
			    {
				    cmdList.Draw(binding, GPU::Handle(), fbs, data.drawState_, GPU::PrimitiveTopology::TRIANGLE_LIST, 0,
				        0, 3, 0, 1);
			    }
			});

		FullscreenData output;
		output.outColor_ = pass.GetData().outColor_;
		output.fbsDesc_ = pass.GetFrameBindingDesc();
		return output;
	}

	static const char* FORWARD_RESOURCE_NAMES[] = {"in_color", "in_depth", "out_color", "out_depth"};

	class ForwardPipeline : public Pipeline
	{
	public:
		enum class DebugMode
		{
			OFF,
			LIGHT_CULLING,

			MAX,
		};

		DebugMode debugMode_ = DebugMode::OFF;

		Shader* shader_ = nullptr;
		ShaderTechnique techComputeTileInfo_;
		ShaderTechnique techComputeLightLists_;
		ShaderTechnique techDebugTileInfo_;

		ForwardPipeline()
		    : Pipeline(FORWARD_RESOURCE_NAMES)
		{
			Resource::Manager::RequestResource(shader_, "shader_tests/forward_pipeline.esf");
			Resource::Manager::WaitForResource(shader_);

			ShaderTechniqueDesc desc;
			techComputeTileInfo_ = shader_->CreateTechnique("TECH_COMPUTE_TILE_INFO", desc);
			techComputeLightLists_ = shader_->CreateTechnique("TECH_COMPUTE_LIGHT_LISTS", desc);
			techDebugTileInfo_ = shader_->CreateTechnique("TECH_DEBUG_TILE_INFO", desc);
		}

		virtual ~ForwardPipeline()
		{
			techComputeTileInfo_ = ShaderTechnique();
			techComputeLightLists_ = ShaderTechnique();
			techDebugTileInfo_ = ShaderTechnique();
			Resource::Manager::ReleaseResource(shader_);
		}

		RenderGraphTextureDesc GetDefaultTextureDesc(i32 w, i32 h)
		{
			RenderGraphTextureDesc desc;
			desc.type_ = GPU::TextureType::TEX2D;
			desc.width_ = w;
			desc.height_ = h;
			desc.format_ = GPU::Format::R8G8B8A8_UNORM;
			return desc;
		}

		RenderGraphTextureDesc GetDepthTextureDesc(i32 w, i32 h)
		{
			RenderGraphTextureDesc desc;
			desc.type_ = GPU::TextureType::TEX2D;
			desc.width_ = w;
			desc.height_ = h;
			desc.format_ = GPU::Format::R24G8_TYPELESS;
			return desc;
		}

		/// Create appropriate shader technique for pipeline.
		void CreateTechniques(Shader* shader, ShaderTechniqueDesc desc, ShaderTechniques& outTechniques)
		{
			auto AddTechnique = [&](const char* name) {
				auto techIt = fbsDescs_.find(name);
				if(techIt != fbsDescs_.end())
				{
					desc.SetFrameBindingSet(techIt->second);
				}

				i32 size = outTechniques.passIndices_.size();
				auto idxIt = outTechniques.passIndices_.find(name);
				if(idxIt != outTechniques.passIndices_.end())
				{
					if(!outTechniques.passTechniques_[idxIt->second])
						outTechniques.passTechniques_[idxIt->second] = shader->CreateTechnique(name, desc);
				}
				else
				{
					outTechniques.passTechniques_.emplace_back(shader->CreateTechnique(name, desc));
					outTechniques.passIndices_.insert(name, size);
				}
			};

			AddTechnique("RenderPassDepthPrepass");
			AddTechnique("RenderPassForward");
		}

		void SetCamera(const Math::Mat44& view, const Math::Mat44& proj, Math::Vec2 screenDimensions)
		{
			view_.view_ = view;
			view_.proj_ = proj;
			view_.viewProj_ = view * proj;
			view_.invProj_ = proj;
			view_.invProj_.Inverse();
			view_.screenDimensions_ = screenDimensions;
		}

		void Setup(RenderGraph& renderGraph) override
		{
			i32 w = w_;
			i32 h = h_;

			struct ViewConstantData
			{
				ViewConstants view_;
				CommonBuffers cbs_;
			};

			auto& renderPassCommonBuffers = renderGraph.AddCallbackRenderPass<ViewConstantData>("Setup Common Buffers",
			    [&](RenderGraphBuilder& builder, ViewConstantData& data) {
				    data.view_ = view_;
				    data.cbs_.viewCB_ =
				        builder.Write(builder.Create("View Constants", objectSBDesc), GPU::BindFlags::CONSTANT_BUFFER);
				    data.cbs_.objectSB_ = builder.Write(
				        builder.Create("Object Constants", objectSBDesc), GPU::BindFlags::SHADER_RESOURCE);
				},
			    [](RenderGraphResources& res, GPU::CommandList& cmdList, const ViewConstantData& data) {
				    cmdList.UpdateBuffer(
				        res.GetBuffer(data.cbs_.viewCB_), 0, sizeof(data.view_), cmdList.Push(&data.view_));
				});

			auto cbs = renderPassCommonBuffers.GetData().cbs_;

			auto renderPassDepth =
			    AddDepthPasses(renderGraph, cbs, GetDepthTextureDesc(w, h), resources_[1], cbs.objectSB_);
			fbsDescs_.insert("RenderPassDepthPrepass", renderPassDepth.fbsDesc_);

			auto lightCulling = AddLightCullingPasses(renderGraph, cbs, renderPassDepth.outDepth_, shader_);

			if(debugMode_ == DebugMode::LIGHT_CULLING)
			{
				RenderGraphResource debugTex = debugTex = lightCulling.outDebug_;
				AddFullscreenPass(renderGraph, cbs, resources_[0], shader_,
				    [&](RenderGraphBuilder& builder) {
					    debugTex = builder.Read(debugTex, GPU::BindFlags::SHADER_RESOURCE);
					},
				    [debugTex](RenderGraphResources& res, Shader* shader, ShaderTechnique& tech) {
					    tech.Set("debugTex", res.Texture2D(debugTex, GPU::Format::INVALID, 0, -1));
					});
			}
			else
			{
				auto renderPassForward = AddForwardPasses(renderGraph, cbs, lightCulling.outLightCB_,
				    lightCulling.outLightSB_, lightCulling.outLightLinkSB_, GetDefaultTextureDesc(w, h), resources_[0],
				    GetDepthTextureDesc(w, h), renderPassDepth.outDepth_, renderPassDepth.outObjectSB_);

				resources_[2] = renderPassForward.outColor_;
				resources_[3] = renderPassForward.outDepth_;
				fbsDescs_.insert("RenderPassForward", renderPassForward.fbsDesc_);
			}
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
	RenderGraph graph;

	// Load shader + teapot model.
	Shader* shader = nullptr;
	Resource::Manager::RequestResource(shader, "shader_tests/simple-mesh.esf");
	Resource::Manager::WaitForResource(shader);

	Model* model = nullptr;
	Resource::Manager::RequestResource(model, "model_tests/teapot.obj");
	Resource::Manager::WaitForResource(model);

	Model* sponzaModel = nullptr;
#if LOAD_SPONZA
	Resource::Manager::RequestResource(sponzaModel, "model_tests/crytek-sponza/sponza.obj");
	Resource::Manager::WaitForResource(sponzaModel);
#endif

	// Create some render packets.
	// For now, they can be permenent.
	f32 angle = 0.0;
	const Math::Vec3 positions[] = {
	    Math::Vec3(-10.0f, 0.0f, -5.0f), Math::Vec3(-5.0f, 0.0f, -5.0f), Math::Vec3(0.0f, 0.0f, -5.0f),
	    Math::Vec3(5.0f, 0.0f, -5.0f), Math::Vec3(10.0f, 0.0f, -5.0f), Math::Vec3(-10.0f, 0.0f, 5.0f),
	    Math::Vec3(-5.0f, 0.0f, 5.0f), Math::Vec3(0.0f, 0.0f, 5.0f), Math::Vec3(5.0f, 0.0f, 5.0f),
	    Math::Vec3(10.0f, 0.0f, 5.0f),
	};

	Light light = {};
	light.enabled_ = 1;
	light.position_ = Math::Vec3(1000.0f, 1000.0f, 1000.0f);
	light.color_.x = 1.0f;
	light.color_.y = 1.0f;
	light.color_.z = 1.0f;
	light.attenuation_ = Math::Vec3(10.0f, 0.0f, 0.0f);
	lights_.push_back(light);

	shaderTechniques_.reserve(model->GetNumMeshes() * 1000);
	{
		f32 r = 0.0f, g = 0.0f, b = 0.0f, h = 0.0f, s = 1.0f, v = 1.0f;
		for(const auto& position : positions)
		{
			ImGui::ColorConvertHSVtoRGB(h, s, v, r, g, b);
			h += 0.1f;
			light.enabled_ = 1;
			light.position_ = position + Math::Vec3(0.0f, 5.0f, 0.0f);
			light.color_.x = r;
			light.color_.y = g;
			light.color_.z = b;
			light.attenuation_ = Math::Vec3(1.0f, 0.5f, 0.1f);
			lights_.push_back(light);
			for(i32 idx = 0; idx < model->GetNumMeshes(); ++idx)
			{
				ShaderTechniques techniques;
				ShaderTechniqueDesc techDesc;
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
	}

	if(sponzaModel)
	{
		for(i32 idx = 0; idx < sponzaModel->GetNumMeshes(); ++idx)
		{
			ShaderTechniques techniques;
			ShaderTechniqueDesc techDesc;
			techDesc.SetVertexElements(sponzaModel->GetMeshVertexElements(idx));
			techDesc.SetTopology(GPU::TopologyType::TRIANGLE);

			MeshRenderPacket packet;
			packet.db_ = sponzaModel->GetMeshDrawBinding(idx);
			packet.draw_ = sponzaModel->GetMeshDraw(idx);
			packet.techDesc_ = techDesc;
			packet.shader_ = shader;
			packet.techs_ = &*shaderTechniques_.emplace_back(std::move(techniques));

			packet.angle_ = angle;
			packet.position_ = Math::Vec3(0.0f, 0.0f, 0.0f);

			angle += 0.5f;

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

			camera_.Update(input, (f32)times_.frame_);

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

			ImGui::Manager::BeginFrame(input, w_, h_);

			times_.profilerUI_ = Core::Timer::GetAbsoluteTime();
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
				ImGui::Text("Frame Time: %f ms", times_.frame_ * 1000.0);
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

						//meshPacket->angle_ += (f32)targetFrameTime;
						meshPacket->object_.world_.Rotation(Math::Vec3(0.0f, meshPacket->angle_, 0.0f));
						meshPacket->object_.world_.Translation(meshPacket->position_);
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
			forwardPipeline.SetCamera(camera_.matrix_, proj, Math::Vec2((f32)w_, (f32)h_));

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

				// Set color target to back buffer.
				forwardPipeline.SetResource("in_color", bbRes);

				// Have the pipeline setup on the graph.
				{
					rmt_ScopedCPUSample(Setup_ForwardPipeline, RMTSF_None);
					forwardPipeline.Setup(graph);
				}

				// Setup ImGui pipeline.
				imguiPipeline.SetResource("in_color", forwardPipeline.GetResource("out_color"));
				{
					rmt_ScopedCPUSample(Setup_ImGuiPipeline, RMTSF_None);
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
						forwardPipeline.CreateTechniques(shader, meshPacket->techDesc_, *meshPacket->techs_);
					}
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

	Loop();

	return 0;
}
