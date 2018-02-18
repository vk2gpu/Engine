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

#include "core/allocator_overrides.h"

DECLARE_MODULE_ALLOCATOR("General/" MODULE_NAME);

#include <cmath>


class App : public IApp
{
public:
	const char* GetName() const override { return "Geometry Compression"; }

	void Initialize() override
	{
		modelNames_.push_back("model_tests/teapot.obj");
		modelNames_.push_back("model_tests/cube.obj");
		modelNames_.push_back("model_tests/crytek-sponza/sponza.obj");
		modelNames_.push_back("model_tests/Bistro/Bistro_Research_Interior.fbx");
		modelNames_.push_back("model_tests/Bistro/Bistro_Research_Exterior.fbx");

		Graphics::ModelRef model = modelNames_[selectedModelIdx_];
		pendingModels_.push_back(model);
		models_.emplace_back(std::move(model));
	}

	void Shutdown() override
	{
		shaderTechniques_.clear();
		models_.clear();

		for(auto* packet : packets_)
		{
			delete packet;
		}
		packets_.clear();
	}

	void Update(const Client::IInputProvider& input, const Client::Window& window, f32 tick) override
	{
		camera_.Update(input, tick);

		window.GetSize(w_, h_);
		view_.LookAt(Math::Vec3(0.0f, 5.0f, -15.0f), Math::Vec3(0.0f, 1.0f, 0.0f), Math::Vec3(0.0f, 1.0f, 0.0f));
		proj_.PerspProjectionVertical(Core::F32_PIDIV4, (f32)h_ / (f32)w_, 0.1f, 2000.0f);

		for(auto it = pendingModels_.begin(); it != pendingModels_.end();)
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
				auto RandomF32 = [this](f32 min, f32 max) {
					const i32 v = rng_.Generate() & 0xffff;
					const f32 r = max - min;
					return ((f32(v) / f32(0xffff)) * r) + min;
				};

				auto RandomVec3 = [&](f32 min, f32 max) {
					return Math::Vec3(RandomF32(min, max), RandomF32(min, max), RandomF32(min, max));
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

			if(ImGui::Button("Dump Allocations"))
			{
				Core::GeneralAllocator().LogAllocs();
				Core::GeneralAllocator().LogStats();
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
			light.position_ = Math::Vec3((f32)std::sin(lightInfo.axisTimeMultiplier_.x * lightInfo.time_),
			                      (f32)std::sin(lightInfo.axisTimeMultiplier_.y * lightInfo.time_),
			                      (f32)std::sin(lightInfo.axisTimeMultiplier_.z * lightInfo.time_)) *
			                  lightInfo.axisSizeMultiplier_;
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
