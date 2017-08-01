#include "catch.hpp"
#include "client/manager.h"
#include "client/window.h"
#include "math/vec2.h"
#include "math/vec4.h"
#include "test_shared.h"

namespace
{
	class Window
	{
	public:
		Window(const char* name)
		    : window_(name, 100, 100, 1024, 768, true)
		    , cmdList_(GPU::Manager::GetHandleAllocator())
		{
			GPU::SwapChainDesc scDesc;
			scDesc.width_ = 1024;
			scDesc.height_ = 768;
			scDesc.format_ = GPU::Format::R8G8B8A8_UNORM;
			scDesc.bufferCount_ = 2;
			scDesc.outputWindow_ = window_.GetPlatformData().handle_;

			scHandle_ = GPU::Manager::CreateSwapChain(scDesc, name);
			REQUIRE(scHandle_);

			GPU::FrameBindingSetDesc fbDesc;
			fbDesc.rtvs_[0].resource_ = scHandle_;
			fbDesc.rtvs_[0].format_ = scDesc.format_;
			fbDesc.rtvs_[0].dimension_ = GPU::ViewDimension::TEX2D;

			fbsHandle_ = GPU::Manager::CreateFrameBindingSet(fbDesc, name);
			REQUIRE(fbsHandle_);

			cmdHandle_ = GPU::Manager::CreateCommandList(name);
			REQUIRE(cmdHandle_);

			drawState_.viewport_.w_ = 1024;
			drawState_.viewport_.h_ = 768;
			drawState_.scissorRect_.w_ = 1024;
			drawState_.scissorRect_.h_ = 768;
		}

		~Window()
		{
			GPU::Manager::DestroyResource(cmdHandle_);
			GPU::Manager::DestroyResource(fbsHandle_);
			GPU::Manager::DestroyResource(scHandle_);
		}

		GPU::CommandList& Begin()
		{
			// Reset command list to reuse.
			cmdList_.Reset();

			// Clear swapchain.
			f32 color[] = {0.1f, 0.1f, 0.2f, 1.0f};
			cmdList_.ClearRTV(fbsHandle_, 0, color);

			return cmdList_;
		}

		void End()
		{
			// Compile and submit.
			GPU::Manager::CompileCommandList(cmdHandle_, cmdList_);
			GPU::Manager::SubmitCommandList(cmdHandle_);

			// Present.
			GPU::Manager::PresentSwapChain(scHandle_);

			// Next frame.
			GPU::Manager::NextFrame();
		}

		Client::Window window_;
		GPU::CommandList cmdList_;
		GPU::DrawState drawState_;
		GPU::Handle scHandle_;
		GPU::Handle fbsHandle_;
		GPU::Handle cmdHandle_;
	};
}

TEST_CASE("graphics-tests-model-request")
{
	ScopedEngine engine;

	Graphics::Model* model = nullptr;
	REQUIRE(Resource::Manager::RequestResource(model, "model_tests/teapot.esf"));
	Resource::Manager::WaitForResource(model);
	REQUIRE(Resource::Manager::ReleaseResource(model));
}

TEST_CASE("graphics-tests-model-draw")
{
	ScopedEngine engine;

	Window window("test");

	Graphics::Shader* shader = nullptr;
	REQUIRE(Resource::Manager::RequestResource(shader, "shader_tests/00-basic.esf"));
	Resource::Manager::WaitForResource(shader);

	Graphics::Model* model = nullptr;
	REQUIRE(Resource::Manager::RequestResource(model, "model_tests/teapot.obj"));
	Resource::Manager::WaitForResource(model);

	GPU::SamplerState smpDesc;
	auto smpHandle = GPU::Manager::CreateSamplerState(smpDesc, "sampler");
	DBG_ASSERT(smpHandle);

	Graphics::ShaderTechniqueDesc techDesc;

	struct DrawStuff
	{
		GPU::Handle db;
		Graphics::ModelMeshDraw draw;
		Graphics::ShaderTechnique tech;
	};

	Core::Vector<DrawStuff> drawStuffs;
	for(i32 idx = 0; idx < model->GetNumMeshes(); ++idx)
	{
		auto elements = model->GetMeshVertexElements(idx);
		techDesc.numVertexElements_ = elements.size();
		for(i32 elementIdx = 0; elementIdx < elements.size(); ++elementIdx)
			techDesc.vertexElements_[elementIdx] = *(elements.data() + elementIdx);
		techDesc.SetTopology(GPU::TopologyType::TRIANGLE);
		techDesc.SetRTVFormat(0, GPU::Format::R8G8B8A8_UNORM);

		DrawStuff drawStuff;
		drawStuff.db = model->GetMeshDrawBinding(idx);
		drawStuff.draw = model->GetMeshDraw(idx);
		drawStuff.tech = shader->CreateTechnique("TECH_DEBUG", techDesc);
		drawStuffs.emplace_back(std::move(drawStuff));
	}

	i32 testRunCounter = GPU::MAX_GPU_FRAMES * 10;
	while(Client::Manager::Update() && (Core::IsDebuggerAttached() || testRunCounter-- > 0))
	{
		auto& cmdList = window.Begin();

		for(auto& drawStuff : drawStuffs)
		{
			if(auto pbs = drawStuff.tech.GetBinding())
			{
				cmdList.Draw(pbs, drawStuff.db, window.fbsHandle_, window.drawState_,
				    GPU::PrimitiveTopology::TRIANGLE_LIST, drawStuff.draw.indexOffset_, drawStuff.draw.vertexOffset_,
				    drawStuff.draw.noofIndices_, 0, 1);
			}
		}

		window.End();

		// Wait for reloading to complete.
		Resource::Manager::WaitOnReload();
	}

	drawStuffs.clear();

	GPU::Manager::DestroyResource(smpHandle);

	REQUIRE(Resource::Manager::ReleaseResource(shader));
	REQUIRE(Resource::Manager::ReleaseResource(model));
}
