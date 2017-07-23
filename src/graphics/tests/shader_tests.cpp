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

	class TriangleDrawer
	{
	public:
		TriangleDrawer()
		{
			struct Vertex
			{
				Math::Vec4 pos;
				Math::Vec2 uv;
			};

			techDesc_ = Graphics::ShaderTechniqueDesc()
			                .SetVertexElement(0, GPU::VertexElement(0, 0, GPU::Format::R32G32B32A32_FLOAT,
			                                         GPU::VertexUsage::POSITION, 0))
			                .SetVertexElement(
			                    1, GPU::VertexElement(0, 16, GPU::Format::R32G32_FLOAT, GPU::VertexUsage::TEXCOORD, 0))
			                .SetTopology(GPU::TopologyType::TRIANGLE)
			                .SetRTVFormat(0, GPU::Format::R8G8B8A8_UNORM);

			const Vertex vertices[] = {
			    {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, {{0.5f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
			    {{0.0f, 0.5f, 0.0f, 1.0f}, {0.0f, 1.0f}},
			};

			const u16 indices[] = {0, 1, 2};

			GPU::BufferDesc vbDesc;
			vbDesc.size_ = sizeof(vertices);
			vbDesc.bindFlags_ = GPU::BindFlags::VERTEX_BUFFER;
			vbHandle_ = GPU::Manager::CreateBuffer(vbDesc, vertices, "Trangle Drawer VB");

			GPU::BufferDesc ibDesc;
			ibDesc.size_ = sizeof(indices);
			ibDesc.bindFlags_ = GPU::BindFlags::INDEX_BUFFER;
			ibHandle_ = GPU::Manager::CreateBuffer(ibDesc, indices, "Trangle Drawer IB");


			GPU::DrawBindingSetDesc dbsDesc;
			dbsDesc.vbs_[0].offset_ = 0;
			dbsDesc.vbs_[0].size_ = sizeof(Vertex) * 3;
			dbsDesc.vbs_[0].stride_ = sizeof(Vertex);
			dbsDesc.vbs_[0].resource_ = vbHandle_;
			dbsDesc.ib_.offset_ = 0;
			dbsDesc.ib_.size_ = sizeof(u16) * 3;
			dbsDesc.ib_.stride_ = sizeof(u16);
			dbsDesc.ib_.resource_ = ibHandle_;
			dbsHandle_ = GPU::Manager::CreateDrawBindingSet(dbsDesc, "Trangle Drawer DBS");

			REQUIRE(Resource::Manager::RequestResource(texture_, "test_texture_bc7.dds"));
			Resource::Manager::WaitForResource(texture_);

			GPU::SamplerState smpDesc;
			smpHandle_ = GPU::Manager::CreateSamplerState(smpDesc, "sampler");
			DBG_ASSERT(smpHandle_);
		}

		~TriangleDrawer() { REQUIRE(Resource::Manager::ReleaseResource(texture_)); }

		void Draw(GPU::Handle fbs, GPU::DrawState drawState, Graphics::ShaderTechnique& tech, GPU::CommandList& cmdList)
		{
			if(auto pbs = tech.GetBinding())
			{
				cmdList.Draw(pbs, dbsHandle_, fbs, drawState, GPU::PrimitiveTopology::TRIANGLE_LIST, 0, 0, 3, 0, 1);
			}
		}

		Graphics::ShaderTechniqueDesc techDesc_;

		GPU::Handle vbHandle_;
		GPU::Handle ibHandle_;
		GPU::Handle dbsHandle_;
		Graphics::Texture* texture_ = nullptr;
		GPU::Handle smpHandle_;
	};
}

TEST_CASE("graphics-tests-shader-request")
{
	Plugin::Manager::Scoped pluginManager;
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());
	Job::Manager::Scoped jobManager(2, 256, 32 * 1024);
	Resource::Manager::Scoped resourceManager;
	ScopedFactory factory;

	// Init device.
	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	Graphics::Shader* shader = nullptr;
	REQUIRE(Resource::Manager::RequestResource(shader, "shader_tests/00-basic.esf"));
	Resource::Manager::WaitForResource(shader);
	REQUIRE(Resource::Manager::ReleaseResource(shader));
}

TEST_CASE("graphics-tests-shader-create-technique")
{
	Client::Manager::Scoped clientManager;
	Plugin::Manager::Scoped pluginManager;
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());
	Job::Manager::Scoped jobManager(2, 256, 32 * 1024);
	Resource::Manager::Scoped resourceManager;
	ScopedFactory factory;

	// Init device.
	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	Window window("test");
	TriangleDrawer drawer;

	Graphics::Shader* shader = nullptr;
	REQUIRE(Resource::Manager::RequestResource(shader, "shader_tests/00-basic.esf"));
	Resource::Manager::WaitForResource(shader);

	auto techMain = shader->CreateTechnique("TECH_MAIN", drawer.techDesc_);
	auto techShadow = shader->CreateTechnique("TECH_SHADOW", drawer.techDesc_);

	auto texIdx = shader->GetBindingIndex("tex_diffuse");
	auto samplerIdx = shader->GetBindingIndex("SS_DEFAULT");

	DBG_ASSERT(texIdx >= 0);
	DBG_ASSERT(samplerIdx >= 0);
	techMain.SetTexture2D(texIdx, drawer.texture_->GetHandle(), 0, drawer.texture_->GetDesc().levels_);
	techMain.SetSampler(samplerIdx, drawer.smpHandle_);

	i32 testRunCounter = GPU::MAX_GPU_FRAMES * 10;
	while(Client::Manager::Update() && (Core::IsDebuggerAttached() || testRunCounter-- > 0))
	{
		auto& cmdList = window.Begin();

		drawer.Draw(window.fbsHandle_, window.drawState_, techMain, cmdList);

		window.End();
	}


	techMain = Graphics::ShaderTechnique();
	techShadow = Graphics::ShaderTechnique();

	REQUIRE(Resource::Manager::ReleaseResource(shader));
}
