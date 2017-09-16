#include "catch.hpp"
#include "client/manager.h"
#include "client/window.h"
#include "core/misc.h"
#include "math/vec2.h"
#include "math/vec4.h"
#include "math/mat44.h"
#include "test_shared.h"

namespace
{
	class Window
	{
	public:
		Window(const char* name)
		    : window_(name, 100, 100, 1024, 768, true)
		    , cmdList_()
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

			REQUIRE(Resource::Manager::RequestResource(texture_, "test_texture.png"));
			Resource::Manager::WaitForResource(texture_);

			GPU::SamplerState smpDesc;
			smpHandle_ = GPU::Manager::CreateSamplerState(smpDesc, "sampler");
			DBG_ASSERT(smpHandle_);
		}

		~TriangleDrawer()
		{
			REQUIRE(Resource::Manager::ReleaseResource(texture_));

			GPU::Manager::DestroyResource(vbHandle_);
			GPU::Manager::DestroyResource(ibHandle_);
			GPU::Manager::DestroyResource(dbsHandle_);
			GPU::Manager::DestroyResource(smpHandle_);
		}

		void Draw(GPU::Handle fbs, GPU::DrawState drawState, Graphics::ShaderTechnique& tech, GPU::CommandList& cmdList,
		    i32 numInstanes = 1)
		{
			if(auto pbs = tech.GetBinding())
			{
				cmdList.Draw(
				    pbs, dbsHandle_, fbs, drawState, GPU::PrimitiveTopology::TRIANGLE_LIST, 0, 0, 3, 0, numInstanes);
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
	ScopedEngine engine;

	Graphics::Shader* shader = nullptr;
	REQUIRE(Resource::Manager::RequestResource(shader, "shader_tests/00-basic.esf"));
	Resource::Manager::WaitForResource(shader);
	REQUIRE(Resource::Manager::ReleaseResource(shader));
}

TEST_CASE("graphics-tests-shader-graphics-create-technique")
{
	ScopedEngine engine;

	Window window("test");
	TriangleDrawer drawer;

	Graphics::Shader* shader = nullptr;
	REQUIRE(Resource::Manager::RequestResource(shader, "shader_tests/00-basic.esf"));
	Resource::Manager::WaitForResource(shader);

	auto techUpdate = shader->CreateTechnique("TECH_MAIN", drawer.techDesc_);
	auto techShadow = shader->CreateTechnique("TECH_SHADOW", drawer.techDesc_);

	i32 testRunCounter = GPU::MAX_GPU_FRAMES * 10;
	while(Client::Manager::Update() && (Core::IsDebuggerAttached() || testRunCounter-- > 0))
	{
		auto& cmdList = window.Begin();

		const auto texIdx = shader->GetBindingIndex("tex_diffuse");
		const auto samplerIdx = shader->GetBindingIndex("SS_DEFAULT");
		if(texIdx >= 0)
			techUpdate.Set(texIdx, GPU::Binding::Texture2D(drawer.texture_->GetHandle(), GPU::Format::INVALID, 0,
			                           drawer.texture_->GetDesc().levels_));
		if(samplerIdx >= 0)
			techUpdate.SetSampler(samplerIdx, drawer.smpHandle_);

		drawer.Draw(window.fbsHandle_, window.drawState_, techUpdate, cmdList);

		window.End();

		// Wait for reloading to complete.
		Resource::Manager::WaitOnReload();
	}


	techUpdate = Graphics::ShaderTechnique();
	techShadow = Graphics::ShaderTechnique();

	REQUIRE(Resource::Manager::ReleaseResource(shader));
}

TEST_CASE("graphics-tests-shader-compute-create-technique")
{
	ScopedEngine engine;

	Window window("test");
	TriangleDrawer drawer;

	Graphics::Shader* shader = nullptr;
	REQUIRE(Resource::Manager::RequestResource(shader, "shader_tests/00-particle.esf"));
	Resource::Manager::WaitForResource(shader);

	struct Particle
	{
		Math::Vec3 position;
		Math::Vec3 velocity;
	};

	struct ParticleParams
	{
		Math::Vec4 time;
		Math::Vec4 tick;
		i32 maxWidth;
	};

	struct Camera
	{
		Math::Mat44 view;
		Math::Mat44 viewProj;
	};

	const i32 numParticles = 8 * 1024;
	const i32 maxParticleWidth = 4096;

	const f32 tick = 1.0f / 600.0f;
	ParticleParams params;
	params.time = Math::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
	params.tick = Math::Vec4(tick, tick * 2.0f, tick * 0.5f, tick * 0.25f);
	params.maxWidth = maxParticleWidth;

	Camera camera;
	camera.view.LookAt(Math::Vec3(0.0f, 5.0f, 10.0f), Math::Vec3(0.0f, 1.0f, 0.0f), Math::Vec3(0.0f, 1.0f, 0.0f));
	camera.viewProj.PerspProjectionVertical(Core::F32_PIDIV4, 1024.0f / 720.0f, 0.01f, 300.0f);
	camera.viewProj = camera.view * camera.viewProj;

	GPU::BufferDesc particleParamsDesc;
	particleParamsDesc.bindFlags_ = GPU::BindFlags::CONSTANT_BUFFER;
	particleParamsDesc.size_ = sizeof(ParticleParams);
	GPU::Handle particleParams = GPU::Manager::CreateBuffer(particleParamsDesc, &params, "particleParams");

	GPU::BufferDesc cameraParamsDesc;
	cameraParamsDesc.bindFlags_ = GPU::BindFlags::CONSTANT_BUFFER;
	cameraParamsDesc.size_ = sizeof(Camera);
	GPU::Handle cameraParams = GPU::Manager::CreateBuffer(cameraParamsDesc, &camera, "cameraParams");

	Core::Random rng;

	Core::Vector<Particle> particles(numParticles);

	auto RNG_F32 = [&rng](f32 min, f32 max) {
		f64 val = rng.Generate() / (f64)INT_MAX;
		val = (min + (val * (max - min)));
		val = Core::Clamp(val, min, max);
		return (f32)val;
	};

	for(auto& particle : particles)
	{
		{
			f32 xVal = RNG_F32(-4.0f, 4.0f);
			f32 yVal = RNG_F32(4.0f, 4.0f);
			f32 zVal = RNG_F32(-4.0f, 4.0f);
			particle.position = Math::Vec3(xVal, yVal, zVal);
		}

		{
			f32 xVal = RNG_F32(-2.0f, 2.0f);
			f32 yVal = RNG_F32(2.0f, 8.0f);
			f32 zVal = RNG_F32(-2.0f, 2.0f);
			particle.velocity.x = xVal;
			particle.velocity.y = yVal;
			particle.velocity.z = zVal;
		}
	}

	GPU::BufferDesc particleBufferDesc;
	particleBufferDesc.bindFlags_ = GPU::BindFlags::UNORDERED_ACCESS | GPU::BindFlags::VERTEX_BUFFER;
	particleBufferDesc.size_ = sizeof(Particle) * numParticles;
	GPU::Handle particleBuffer = GPU::Manager::CreateBuffer(particleBufferDesc, particles.data(), "particleBuffer");

	auto techUpdate = shader->CreateTechnique("TECH_PARTICLE_UPDATE", Graphics::ShaderTechniqueDesc());
	auto techDraw = shader->CreateTechnique("TECH_PARTICLE_DRAW", drawer.techDesc_);

	i32 testRunCounter = GPU::MAX_GPU_FRAMES * 10;
	while(Client::Manager::Update() && (Core::IsDebuggerAttached() || testRunCounter-- > 0))
	{
		auto& cmdList = window.Begin();

		const auto particleParamsIdx = shader->GetBindingIndex("ParticleConfig");
		const auto cameraIdx = shader->GetBindingIndex("Camera");
		const auto inoutParticlesIdx = shader->GetBindingIndex("inout_particles");
		const auto inParticlesIdx = shader->GetBindingIndex("in_particles");
		if(particleParamsIdx >= 0)
		{
			techUpdate.Set(particleParamsIdx, GPU::Binding::CBuffer(particleParams, 0, sizeof(params)));
			techDraw.Set(particleParamsIdx, GPU::Binding::CBuffer(particleParams, 0, sizeof(params)));
		}
		if(cameraIdx >= 0)
		{
			techUpdate.Set(cameraIdx, GPU::Binding::CBuffer(cameraParams, 0, sizeof(camera)));
			techDraw.Set(cameraIdx, GPU::Binding::CBuffer(cameraParams, 0, sizeof(camera)));
		}
		if(inoutParticlesIdx >= 0)
		{
			techUpdate.Set(inoutParticlesIdx,
			    GPU::Binding::RWBuffer(particleBuffer, GPU::Format::INVALID, 0, numParticles, sizeof(Particle)));
			techDraw.Set(inoutParticlesIdx,
			    GPU::Binding::RWBuffer(particleBuffer, GPU::Format::INVALID, 0, numParticles, sizeof(Particle)));
		}
		if(inParticlesIdx >= 0)
		{
			techUpdate.Set(inParticlesIdx,
			    GPU::Binding::Buffer(particleBuffer, GPU::Format::INVALID, 0, numParticles, sizeof(Particle)));
			techDraw.Set(inParticlesIdx,
			    GPU::Binding::Buffer(particleBuffer, GPU::Format::INVALID, 0, numParticles, sizeof(Particle)));
		}

		cmdList.Dispatch(techUpdate.GetBinding(), Core::Min(numParticles, maxParticleWidth),
		    Core::Max(1, numParticles / maxParticleWidth), 1);

		drawer.Draw(window.fbsHandle_, window.drawState_, techDraw, cmdList, numParticles);

		params.time += params.tick;
		cmdList.UpdateBuffer(particleParams, 0, sizeof(params), &params);
		window.End();

		// Wait for reloading to complete.
		Resource::Manager::WaitOnReload();
	}

	techUpdate = Graphics::ShaderTechnique();
	techDraw = Graphics::ShaderTechnique();
	GPU::Manager::DestroyResource(particleParams);
	GPU::Manager::DestroyResource(cameraParams);
	GPU::Manager::DestroyResource(particleBuffer);

	REQUIRE(Resource::Manager::ReleaseResource(shader));
}
