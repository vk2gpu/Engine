#include "gpu/manager.h"
#include "gpu/utils.h"
#include "client/window.h"
#include "core/array.h"
#include "core/vector.h"

#include "catch.hpp"

#pragma warning(disable : 4189)

typedef u8 BYTE;
#include "gpu_d3d12/private/shaders/default_cs.h"
#include "gpu_d3d12/private/shaders/default_vs.h"
#include "gpu_d3d12/private/shaders/default_ps.h"

#include "gpu/tests/shaders/test_buf_cs.h"
#include "gpu/tests/shaders/test_tex_cs.h"

namespace
{
	GPU::DebuggerIntegrationFlags debuggerIntegrationFlags = GPU::DebuggerIntegrationFlags::RENDERDOC;
}

TEST_CASE("gpu-tests-formats")
{
	for(i32 i = 0; i < (i32)GPU::Format::MAX; ++i)
	{
		auto info = GPU::GetFormatInfo((GPU::Format)i);
		REQUIRE(info.blockW_ > 0);
		REQUIRE(info.blockH_ > 0);
		REQUIRE(info.blockBits_ > 0);
	}
}

TEST_CASE("gpu-tests-manager")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_, debuggerIntegrationFlags);
}

TEST_CASE("gpu-tests-enumerate")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_, debuggerIntegrationFlags);
	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);
	Core::Vector<GPU::AdapterInfo> adapterInfos;
	adapterInfos.resize(numAdapters);
	manager.EnumerateAdapters(adapterInfos.data(), adapterInfos.size());
}

TEST_CASE("gpu-tests-initialize")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_, debuggerIntegrationFlags);


	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);
}

TEST_CASE("gpu-tests-create-swapchain")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_, debuggerIntegrationFlags);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(manager, testName.c_str());

	GPU::SwapChainDesc desc;
	desc.width_ = 640;
	desc.height_ = 480;
	desc.format_ = GPU::Format::R8G8B8A8_UNORM;
	desc.bufferCount_ = 2;
	desc.outputWindow_ = window.GetPlatformData().handle_;

	GPU::Handle handle;
	handle = manager.CreateSwapChain(desc, testName.c_str());
	REQUIRE(handle);

	manager.DestroyResource(handle);
}

TEST_CASE("gpu-tests-create-buffer")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_, debuggerIntegrationFlags);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(manager, testName.c_str());

	SECTION("no-data")
	{
		GPU::BufferDesc desc;
		desc.bindFlags_ = GPU::BindFlags::VERTEX_BUFFER;
		desc.size_ = 32 * 1024;

		GPU::Handle handle;
		handle = manager.CreateBuffer(desc, nullptr, testName.c_str());
		REQUIRE(handle);

		manager.DestroyResource(handle);
	}

	SECTION("data")
	{
		for(i32 i = 0; i < 32; ++i)
		{
			GPU::BufferDesc desc;
			desc.bindFlags_ = GPU::BindFlags::VERTEX_BUFFER;
			desc.size_ = 32 * 1024;

			Core::Vector<u32> data;
			data.resize((i32)desc.size_);

			GPU::Handle handle;
			handle = manager.CreateBuffer(desc, data.data(), testName.c_str());
			REQUIRE(handle);

			manager.DestroyResource(handle);
		}
	}
}

TEST_CASE("gpu-tests-create-texture")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_, debuggerIntegrationFlags);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(manager, testName.c_str());

	SECTION("1d-no-data")
	{
		GPU::TextureDesc desc;
		desc.type_ = GPU::TextureType::TEX1D;
		desc.format_ = GPU::Format::R8G8B8A8_TYPELESS;
		desc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
		desc.width_ = 256;

		GPU::Handle handle;
		handle = manager.CreateTexture(desc, nullptr, testName.c_str());
		REQUIRE(handle);
		manager.DestroyResource(handle);
	}

	SECTION("2d-no-data")
	{
		GPU::TextureDesc desc;
		desc.type_ = GPU::TextureType::TEX2D;
		desc.format_ = GPU::Format::R8G8B8A8_TYPELESS;
		desc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
		desc.width_ = 256;
		desc.height_ = 256;

		GPU::Handle handle;
		handle = manager.CreateTexture(desc, nullptr, testName.c_str());
		REQUIRE(handle);
		manager.DestroyResource(handle);
	}

	SECTION("3d-no-data")
	{
		GPU::TextureDesc desc;
		desc.type_ = GPU::TextureType::TEX3D;
		desc.format_ = GPU::Format::R8G8B8A8_TYPELESS;
		desc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
		desc.width_ = 256;
		desc.height_ = 256;
		desc.depth_ = 256;

		GPU::Handle handle;
		handle = manager.CreateTexture(desc, nullptr, testName.c_str());
		REQUIRE(handle);
		manager.DestroyResource(handle);
	}

	SECTION("cube-no-data")
	{
		GPU::TextureDesc desc;
		desc.type_ = GPU::TextureType::TEXCUBE;
		desc.format_ = GPU::Format::R8G8B8A8_TYPELESS;
		desc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
		desc.width_ = 256;

		GPU::Handle handle;
		handle = manager.CreateTexture(desc, nullptr, testName.c_str());
		REQUIRE(handle);
		manager.DestroyResource(handle);
	}

	SECTION("1d-data")
	{
		GPU::TextureDesc desc;
		desc.type_ = GPU::TextureType::TEX1D;
		desc.format_ = GPU::Format::R8G8B8A8_TYPELESS;
		desc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
		desc.width_ = 256;

		auto layoutInfo = GPU::GetTextureLayoutInfo(desc.format_, desc.width_, 1);
		i64 size = GPU::GetTextureSize(desc.format_, desc.width_, 1, 1, 1, 1);
		Core::Vector<u8> data;
		data.resize((i32)size);

		GPU::TextureSubResourceData subResData;
		subResData.data_ = data.data();
		subResData.rowPitch_ = layoutInfo.pitch_;
		subResData.slicePitch_ = layoutInfo.slicePitch_;

		GPU::Handle handle;
		handle = manager.CreateTexture(desc, &subResData, testName.c_str());
		REQUIRE(handle);
		manager.DestroyResource(handle);
	}

	SECTION("2d-data")
	{
		GPU::TextureDesc desc;
		desc.type_ = GPU::TextureType::TEX2D;
		desc.format_ = GPU::Format::R8G8B8A8_TYPELESS;
		desc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
		desc.width_ = 256;
		desc.height_ = 256;

		auto layoutInfo = GPU::GetTextureLayoutInfo(desc.format_, desc.width_, desc.height_);
		i64 size = GPU::GetTextureSize(desc.format_, desc.width_, 1, 1, 1, 1);
		Core::Vector<u8> data;
		data.resize((i32)size);

		GPU::TextureSubResourceData subResData;
		subResData.data_ = data.data();
		subResData.rowPitch_ = layoutInfo.pitch_;
		subResData.slicePitch_ = layoutInfo.slicePitch_;

		GPU::Handle handle;
		handle = manager.CreateTexture(desc, &subResData, testName.c_str());
		REQUIRE(handle);
		manager.DestroyResource(handle);
	}

	SECTION("3d-data")
	{
		GPU::TextureDesc desc;
		desc.type_ = GPU::TextureType::TEX3D;
		desc.format_ = GPU::Format::R8G8B8A8_TYPELESS;
		desc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
		desc.width_ = 256;
		desc.height_ = 256;
		desc.depth_ = 256;

		auto layoutInfo = GPU::GetTextureLayoutInfo(desc.format_, desc.width_, desc.height_);
		i64 size = GPU::GetTextureSize(desc.format_, desc.width_, desc.height_, desc.depth_, 1, 1);
		Core::Vector<u8> data;
		data.resize((i32)size);

		GPU::TextureSubResourceData subResData;
		subResData.data_ = data.data();
		subResData.rowPitch_ = layoutInfo.pitch_;
		subResData.slicePitch_ = layoutInfo.slicePitch_;

		GPU::Handle handle;
		handle = manager.CreateTexture(desc, &subResData, testName.c_str());
		REQUIRE(handle);
		manager.DestroyResource(handle);
	}

	SECTION("cube-data")
	{
		GPU::TextureDesc desc;
		desc.type_ = GPU::TextureType::TEXCUBE;
		desc.format_ = GPU::Format::R8G8B8A8_TYPELESS;
		desc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
		desc.width_ = 256;

		auto layoutInfo = GPU::GetTextureLayoutInfo(desc.format_, desc.width_, desc.width_);
		i64 size = GPU::GetTextureSize(desc.format_, desc.width_, desc.width_, 1, 1, 6);
		Core::Vector<u8> data;
		data.resize((i32)size);

		Core::Array<GPU::TextureSubResourceData, 6> subResDatas;
		i64 offset = 0;
		for(i32 i = 0; i < 6; ++i)
		{
			subResDatas[i].data_ = data.data() + offset;
			subResDatas[i].rowPitch_ = layoutInfo.pitch_;
			subResDatas[i].slicePitch_ = layoutInfo.slicePitch_;
			offset += layoutInfo.slicePitch_;
		}

		GPU::Handle handle;
		handle = manager.CreateTexture(desc, subResDatas.data(), testName.c_str());
		REQUIRE(handle);
		manager.DestroyResource(handle);
	}
}

TEST_CASE("gpu-tests-create-commandlist")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_, debuggerIntegrationFlags);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(manager, testName.c_str());

	GPU::Handle handle;
	handle = manager.CreateCommandList(testName.c_str());
	REQUIRE(handle);
	manager.DestroyResource(handle);
}

TEST_CASE("gpu-tests-create-shader")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_, debuggerIntegrationFlags);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(manager, testName.c_str());

	GPU::ShaderDesc desc;
	desc.type_ = GPU::ShaderType::COMPUTE;
	desc.dataSize_ = sizeof(g_CShader);
	desc.data_ = g_CShader;

	GPU::Handle handle = manager.CreateShader(desc, testName.c_str());
	REQUIRE(handle);
	manager.DestroyResource(handle);
}


TEST_CASE("gpu-tests-create-compute-pipeline-state")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_, debuggerIntegrationFlags);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(manager, testName.c_str());

	GPU::ShaderDesc shaderDesc;
	shaderDesc.type_ = GPU::ShaderType::COMPUTE;
	shaderDesc.dataSize_ = sizeof(g_CShader);
	shaderDesc.data_ = g_CShader;

	GPU::Handle shaderHandle = manager.CreateShader(shaderDesc, testName.c_str());
	REQUIRE(shaderHandle);

	GPU::ComputePipelineStateDesc pipelineDesc;
	pipelineDesc.shader_ = shaderHandle;
	GPU::Handle pipelineHandle = manager.CreateComputePipelineState(pipelineDesc, testName.c_str());

	manager.DestroyResource(pipelineHandle);
	manager.DestroyResource(shaderHandle);
}

TEST_CASE("gpu-tests-create-pipeline-binding-set")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_, debuggerIntegrationFlags);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(manager, testName.c_str());

	GPU::ShaderDesc shaderDesc;
	shaderDesc.type_ = GPU::ShaderType::COMPUTE;
	shaderDesc.dataSize_ = sizeof(g_CShader);
	shaderDesc.data_ = g_CShader;

	// Create resources to test bindings.
	GPU::Handle texHandle;
	GPU::Handle cbHandle;
	GPU::Handle samplerHandle;
	{
		GPU::TextureDesc desc;
		desc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE | GPU::BindFlags::UNORDERED_ACCESS;
		desc.type_ = GPU::TextureType::TEX2D;
		desc.width_ = 256;
		desc.height_ = 256;
		desc.format_ = GPU::Format::R8G8B8A8_UNORM;
		texHandle = manager.CreateTexture(desc, nullptr, testName.c_str());
		REQUIRE(texHandle);
	}

	{
		GPU::BufferDesc desc;
		desc.bindFlags_ = GPU::BindFlags::CONSTANT_BUFFER;
		desc.size_ = 4096;
		cbHandle = manager.CreateBuffer(desc, nullptr, testName.c_str());
		REQUIRE(cbHandle);
	}

	{
		GPU::SamplerState desc;
		samplerHandle = manager.CreateSamplerState(desc, testName.c_str());
		REQUIRE(samplerHandle);
	}

	GPU::Handle shaderHandle = manager.CreateShader(shaderDesc, testName.c_str());
	REQUIRE(shaderHandle);

	GPU::ComputePipelineStateDesc pipelineDesc;
	pipelineDesc.shader_ = shaderHandle;
	GPU::Handle pipelineHandle = manager.CreateComputePipelineState(pipelineDesc, testName.c_str());

	SECTION("no-views")
	{
		GPU::PipelineBindingSetDesc pipelineBindingSetDesc;
		pipelineBindingSetDesc.pipelineState_ = pipelineHandle;
		GPU::Handle pipelineBindingSetHandle =
		    manager.CreatePipelineBindingSet(pipelineBindingSetDesc, testName.c_str());

		manager.DestroyResource(pipelineBindingSetHandle);
	}

	SECTION("srvs")
	{
		GPU::PipelineBindingSetDesc pipelineBindingSetDesc;
		pipelineBindingSetDesc.pipelineState_ = pipelineHandle;
		pipelineBindingSetDesc.numSRVs_ = 1;
		pipelineBindingSetDesc.srvs_[0].resource_ = texHandle;
		pipelineBindingSetDesc.srvs_[0].format_ = GPU::Format::R8G8B8A8_UNORM;
		pipelineBindingSetDesc.srvs_[0].dimension_ = GPU::ViewDimension::TEX2D;
		pipelineBindingSetDesc.srvs_[0].mipLevels_NumElements_ = -1;
		GPU::Handle pipelineBindingSetHandle =
		    manager.CreatePipelineBindingSet(pipelineBindingSetDesc, testName.c_str());

		manager.DestroyResource(pipelineBindingSetHandle);
	}

	SECTION("cbvs")
	{
		GPU::PipelineBindingSetDesc pipelineBindingSetDesc;
		pipelineBindingSetDesc.pipelineState_ = pipelineHandle;
		pipelineBindingSetDesc.numCBVs_ = 1;
		pipelineBindingSetDesc.cbvs_[0].resource_ = cbHandle;
		pipelineBindingSetDesc.cbvs_[0].offset_ = 0;
		pipelineBindingSetDesc.cbvs_[0].size_ = 4096;
		GPU::Handle pipelineBindingSetHandle =
		    manager.CreatePipelineBindingSet(pipelineBindingSetDesc, testName.c_str());

		manager.DestroyResource(pipelineBindingSetHandle);
	}

	SECTION("uavs")
	{
		GPU::PipelineBindingSetDesc pipelineBindingSetDesc;
		pipelineBindingSetDesc.pipelineState_ = pipelineHandle;
		pipelineBindingSetDesc.numUAVs_ = 1;
		pipelineBindingSetDesc.uavs_[0].resource_ = texHandle;
		pipelineBindingSetDesc.uavs_[0].format_ = GPU::Format::R8G8B8A8_UNORM;
		pipelineBindingSetDesc.uavs_[0].dimension_ = GPU::ViewDimension::TEX2D;
		GPU::Handle pipelineBindingSetHandle =
		    manager.CreatePipelineBindingSet(pipelineBindingSetDesc, testName.c_str());

		manager.DestroyResource(pipelineBindingSetHandle);
	}

	manager.DestroyResource(samplerHandle);
	manager.DestroyResource(cbHandle);
	manager.DestroyResource(texHandle);
	manager.DestroyResource(pipelineHandle);
	manager.DestroyResource(shaderHandle);
}

TEST_CASE("gpu-tests-create-graphics-pipeline-state")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_, debuggerIntegrationFlags);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(manager, testName.c_str());

	GPU::ShaderDesc shaderDesc;
	shaderDesc.type_ = GPU::ShaderType::VERTEX;
	shaderDesc.dataSize_ = sizeof(g_VShader);
	shaderDesc.data_ = g_VShader;

	GPU::Handle shaderHandle = manager.CreateShader(shaderDesc, testName.c_str());
	REQUIRE(shaderHandle);

	GPU::GraphicsPipelineStateDesc pipelineDesc;
	pipelineDesc.shaders_[(i32)GPU::ShaderType::VERTEX] = shaderHandle;
	pipelineDesc.renderState_ = GPU::RenderState();
	pipelineDesc.numVertexElements_ = 1;
	pipelineDesc.vertexElements_[0].streamIdx_ = 0;
	pipelineDesc.vertexElements_[0].offset_ = 0;
	pipelineDesc.vertexElements_[0].format_ = GPU::Format::R32G32B32A32_FLOAT;
	pipelineDesc.vertexElements_[0].usage_ = GPU::VertexUsage::POSITION;
	pipelineDesc.vertexElements_[0].usageIdx_ = 0;
	pipelineDesc.topology_ = GPU::TopologyType::TRIANGLE;
	pipelineDesc.numRTs_ = 0;
	pipelineDesc.dsvFormat_ = GPU::Format::D24_UNORM_S8_UINT;
	GPU::Handle pipelineHandle = manager.CreateGraphicsPipelineState(pipelineDesc, testName.c_str());

	manager.DestroyResource(pipelineHandle);
	manager.DestroyResource(shaderHandle);
}

TEST_CASE("gpu-tests-create-draw-binding-set")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_, debuggerIntegrationFlags);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(manager, testName.c_str());

	float vertices[] = {
	    0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	};

	GPU::BufferDesc vbDesc;
	vbDesc.bindFlags_ = GPU::BindFlags::VERTEX_BUFFER;
	vbDesc.size_ = sizeof(vertices);
	GPU::Handle vbHandle = manager.CreateBuffer(vbDesc, vertices, testName.c_str());
	REQUIRE(vbHandle);

	GPU::DrawBindingSetDesc dbsDesc;
	dbsDesc.vbs_[0].resource_ = vbHandle;
	dbsDesc.vbs_[0].offset_ = 0;
	dbsDesc.vbs_[0].size_ = (i32)vbDesc.size_;

	GPU::Handle dbsHandle = manager.CreateDrawBindingSet(dbsDesc, testName.c_str());
	REQUIRE(dbsHandle);

	manager.DestroyResource(dbsHandle);
	manager.DestroyResource(vbHandle);
}

TEST_CASE("gpu-tests-create-frame-binding-set")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_, debuggerIntegrationFlags);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(manager, testName.c_str());

	GPU::TextureDesc rtDesc;
	rtDesc.type_ = GPU::TextureType::TEX2D;
	rtDesc.bindFlags_ = GPU::BindFlags::RENDER_TARGET | GPU::BindFlags::SHADER_RESOURCE;
	rtDesc.width_ = 128;
	rtDesc.height_ = 128;
	rtDesc.format_ = GPU::Format::R8G8B8A8_UNORM;
	GPU::Handle rtHandle = manager.CreateTexture(rtDesc, nullptr, testName.c_str());
	REQUIRE(rtHandle);

	GPU::TextureDesc dsDesc;
	dsDesc.type_ = GPU::TextureType::TEX2D;
	dsDesc.bindFlags_ = GPU::BindFlags::DEPTH_STENCIL;
	dsDesc.width_ = 128;
	dsDesc.height_ = 128;
	dsDesc.format_ = GPU::Format::D24_UNORM_S8_UINT;
	GPU::Handle dsHandle = manager.CreateTexture(dsDesc, nullptr, testName.c_str());
	REQUIRE(dsHandle);

	GPU::FrameBindingSetDesc fbDesc;
	fbDesc.rtvs_[0].resource_ = rtHandle;
	fbDesc.rtvs_[0].format_ = rtDesc.format_;
	fbDesc.rtvs_[0].dimension_ = GPU::ViewDimension::TEX2D;
	fbDesc.dsv_.resource_ = dsHandle;
	fbDesc.dsv_.format_ = dsDesc.format_;
	fbDesc.dsv_.dimension_ = GPU::ViewDimension::TEX2D;

	GPU::Handle fbsHandle = manager.CreateFrameBindingSet(fbDesc, testName.c_str());
	REQUIRE(fbsHandle);

	manager.DestroyResource(fbsHandle);
	manager.DestroyResource(dsHandle);
	manager.DestroyResource(rtHandle);
}

TEST_CASE("gpu-tests-clears")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_, debuggerIntegrationFlags);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(manager, testName.c_str());

	GPU::TextureDesc rtDesc;
	rtDesc.type_ = GPU::TextureType::TEX2D;
	rtDesc.bindFlags_ = GPU::BindFlags::RENDER_TARGET | GPU::BindFlags::SHADER_RESOURCE;
	rtDesc.width_ = 128;
	rtDesc.height_ = 128;
	rtDesc.format_ = GPU::Format::R8G8B8A8_UNORM;
	GPU::Handle rtHandle = manager.CreateTexture(rtDesc, nullptr, testName.c_str());
	REQUIRE(rtHandle);

	GPU::TextureDesc dsDesc;
	dsDesc.type_ = GPU::TextureType::TEX2D;
	dsDesc.bindFlags_ = GPU::BindFlags::DEPTH_STENCIL;
	dsDesc.width_ = 128;
	dsDesc.height_ = 128;
	dsDesc.format_ = GPU::Format::D24_UNORM_S8_UINT;
	GPU::Handle dsHandle = manager.CreateTexture(dsDesc, nullptr, testName.c_str());
	REQUIRE(dsHandle);

	GPU::FrameBindingSetDesc fbDesc;
	fbDesc.rtvs_[0].resource_ = rtHandle;
	fbDesc.rtvs_[0].format_ = rtDesc.format_;
	fbDesc.rtvs_[0].dimension_ = GPU::ViewDimension::TEX2D;
	fbDesc.dsv_.resource_ = dsHandle;
	fbDesc.dsv_.format_ = dsDesc.format_;
	fbDesc.dsv_.dimension_ = GPU::ViewDimension::TEX2D;

	GPU::Handle fbsHandle = manager.CreateFrameBindingSet(fbDesc, testName.c_str());
	REQUIRE(fbsHandle);

	GPU::Handle cmdHandle = manager.CreateCommandList(testName.c_str());
	GPU::CommandList cmdList(manager.GetHandleAllocator());

	f32 color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	REQUIRE(cmdList.ClearRTV(fbsHandle, 0, color));
	REQUIRE(cmdList.ClearDSV(fbsHandle, 0.0f, 0));
	REQUIRE(manager.CompileCommandList(cmdHandle, cmdList));
	REQUIRE(manager.SubmitCommandList(cmdHandle));

	manager.DestroyResource(cmdHandle);
	manager.DestroyResource(fbsHandle);
	manager.DestroyResource(dsHandle);
	manager.DestroyResource(rtHandle);
}

TEST_CASE("gpu-tests-compile-draw")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_, debuggerIntegrationFlags);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(manager, testName.c_str());

	float vertices[] = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f};

	GPU::BufferDesc vbDesc;
	vbDesc.bindFlags_ = GPU::BindFlags::VERTEX_BUFFER;
	vbDesc.size_ = sizeof(vertices);
	GPU::Handle vbHandle = manager.CreateBuffer(vbDesc, vertices, testName.c_str());
	REQUIRE(vbHandle);

	GPU::DrawBindingSetDesc dbsDesc;
	dbsDesc.vbs_[0].resource_ = vbHandle;
	dbsDesc.vbs_[0].offset_ = 0;
	dbsDesc.vbs_[0].size_ = (i32)vbDesc.size_;
	dbsDesc.vbs_[0].stride_ = sizeof(float) * 4;

	GPU::Handle dbsHandle = manager.CreateDrawBindingSet(dbsDesc, testName.c_str());
	REQUIRE(dbsHandle);

	GPU::TextureDesc rtDesc;
	rtDesc.type_ = GPU::TextureType::TEX2D;
	rtDesc.bindFlags_ = GPU::BindFlags::RENDER_TARGET | GPU::BindFlags::SHADER_RESOURCE;
	rtDesc.width_ = 128;
	rtDesc.height_ = 128;
	rtDesc.format_ = GPU::Format::R8G8B8A8_UNORM;
	GPU::Handle rtHandle = manager.CreateTexture(rtDesc, nullptr, testName.c_str());
	REQUIRE(rtHandle);

	GPU::TextureDesc dsDesc;
	dsDesc.type_ = GPU::TextureType::TEX2D;
	dsDesc.bindFlags_ = GPU::BindFlags::DEPTH_STENCIL;
	dsDesc.width_ = 128;
	dsDesc.height_ = 128;
	dsDesc.format_ = GPU::Format::D24_UNORM_S8_UINT;
	GPU::Handle dsHandle = manager.CreateTexture(dsDesc, nullptr, testName.c_str());
	REQUIRE(dsHandle);

	GPU::FrameBindingSetDesc fbDesc;
	fbDesc.rtvs_[0].resource_ = rtHandle;
	fbDesc.rtvs_[0].format_ = rtDesc.format_;
	fbDesc.rtvs_[0].dimension_ = GPU::ViewDimension::TEX2D;
	fbDesc.dsv_.resource_ = dsHandle;
	fbDesc.dsv_.format_ = dsDesc.format_;
	fbDesc.dsv_.dimension_ = GPU::ViewDimension::TEX2D;

	GPU::Handle fbsHandle = manager.CreateFrameBindingSet(fbDesc, testName.c_str());
	REQUIRE(fbsHandle);

	GPU::ShaderDesc vsDesc;
	vsDesc.type_ = GPU::ShaderType::VERTEX;
	vsDesc.dataSize_ = sizeof(g_VShader);
	vsDesc.data_ = g_VShader;

	GPU::Handle vsHandle = manager.CreateShader(vsDesc, testName.c_str());
	REQUIRE(vsHandle);

	GPU::ShaderDesc psDesc;
	psDesc.type_ = GPU::ShaderType::VERTEX;
	psDesc.dataSize_ = sizeof(g_PShader);
	psDesc.data_ = g_PShader;

	GPU::Handle psHandle = manager.CreateShader(psDesc, testName.c_str());
	REQUIRE(psHandle);

	GPU::GraphicsPipelineStateDesc pipelineDesc;
	pipelineDesc.shaders_[(i32)GPU::ShaderType::VERTEX] = vsHandle;
	pipelineDesc.shaders_[(i32)GPU::ShaderType::PIXEL] = psHandle;
	pipelineDesc.renderState_ = GPU::RenderState();
	pipelineDesc.numVertexElements_ = 1;
	pipelineDesc.vertexElements_[0].streamIdx_ = 0;
	pipelineDesc.vertexElements_[0].offset_ = 0;
	pipelineDesc.vertexElements_[0].format_ = GPU::Format::R32G32B32A32_FLOAT;
	pipelineDesc.vertexElements_[0].usage_ = GPU::VertexUsage::POSITION;
	pipelineDesc.vertexElements_[0].usageIdx_ = 0;
	pipelineDesc.topology_ = GPU::TopologyType::TRIANGLE;
	pipelineDesc.numRTs_ = 1;
	pipelineDesc.rtvFormats_[0] = GPU::Format::R8G8B8A8_UNORM;
	pipelineDesc.dsvFormat_ = GPU::Format::D24_UNORM_S8_UINT;
	pipelineDesc.renderState_.cullMode_ = GPU::CullMode::NONE;
	GPU::Handle pipelineHandle = manager.CreateGraphicsPipelineState(pipelineDesc, testName.c_str());

	GPU::PipelineBindingSetDesc pipelineBindingSetDesc;
	pipelineBindingSetDesc.pipelineState_ = pipelineHandle;
	GPU::Handle pbsHandle = manager.CreatePipelineBindingSet(pipelineBindingSetDesc, testName.c_str());

	GPU::Handle cmdHandle = manager.CreateCommandList(testName.c_str());
	GPU::CommandList cmdList(manager.GetHandleAllocator());

	f32 color[4] = {0.2f, 0.2f, 0.2f, 1.0f};
	REQUIRE(cmdList.ClearRTV(fbsHandle, 0, color));
	REQUIRE(cmdList.ClearDSV(fbsHandle, 0.0f, 0));
	REQUIRE(cmdList.Draw(pbsHandle, dbsHandle, fbsHandle, GPU::PrimitiveTopology::TRIANGLE_LIST, 0, 1, 3, 0, 1));
	REQUIRE(manager.CompileCommandList(cmdHandle, cmdList));
	REQUIRE(manager.SubmitCommandList(cmdHandle));

	manager.DestroyResource(cmdHandle);
	manager.DestroyResource(pbsHandle);
	manager.DestroyResource(pipelineHandle);
	manager.DestroyResource(psHandle);
	manager.DestroyResource(vsHandle);
	manager.DestroyResource(dbsHandle);
	manager.DestroyResource(vbHandle);
	manager.DestroyResource(fbsHandle);
	manager.DestroyResource(dsHandle);
	manager.DestroyResource(rtHandle);
}

TEST_CASE("gpu-tests-compile-dispatch")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_, debuggerIntegrationFlags);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(manager, testName.c_str());

	float vertices[] = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f};

	GPU::BufferDesc bufDesc;
	bufDesc.bindFlags_ = GPU::BindFlags::UNORDERED_ACCESS;
	bufDesc.size_ = 256;
	GPU::Handle bufHandle = manager.CreateBuffer(bufDesc, vertices, testName.c_str());
	REQUIRE(bufHandle);

	GPU::TextureDesc texDesc;
	texDesc.type_ = GPU::TextureType::TEX2D;
	texDesc.bindFlags_ = GPU::BindFlags::UNORDERED_ACCESS;
	texDesc.width_ = 128;
	texDesc.height_ = 128;
	texDesc.format_ = GPU::Format::R8G8B8A8_UNORM;
	GPU::Handle texHandle = manager.CreateTexture(texDesc, nullptr, testName.c_str());
	REQUIRE(texHandle);

	{
		GPU::ShaderDesc csDesc;
		csDesc.type_ = GPU::ShaderType::COMPUTE;
		csDesc.dataSize_ = sizeof(g_CTestBuf);
		csDesc.data_ = g_CTestBuf;

		GPU::Handle csHandle = manager.CreateShader(csDesc, testName.c_str());
		REQUIRE(csHandle);

		GPU::ComputePipelineStateDesc pipelineDesc;
		pipelineDesc.shader_ = csHandle;
		GPU::Handle pipelineHandle = manager.CreateComputePipelineState(pipelineDesc, testName.c_str());
		REQUIRE(pipelineHandle);

		GPU::PipelineBindingSetDesc pipelineBindingSetDesc;
		pipelineBindingSetDesc.pipelineState_ = pipelineHandle;
		pipelineBindingSetDesc.numUAVs_ = 1;
		pipelineBindingSetDesc.uavs_[0].resource_ = bufHandle;
		pipelineBindingSetDesc.uavs_[0].format_ = GPU::Format::R32_SINT;
		pipelineBindingSetDesc.uavs_[0].dimension_ = GPU::ViewDimension::BUFFER;
		pipelineBindingSetDesc.uavs_[0].mipSlice_FirstElement_ = 0;
		pipelineBindingSetDesc.uavs_[0].firstArraySlice_FirstWSlice_NumElements_ = (i32)(bufDesc.size_ / sizeof(i32));
		GPU::Handle pbsHandle = manager.CreatePipelineBindingSet(pipelineBindingSetDesc, testName.c_str());
		REQUIRE(pbsHandle);

		GPU::Handle cmdHandle = manager.CreateCommandList(testName.c_str());
		GPU::CommandList cmdList(manager.GetHandleAllocator());

		i32 xGroups = (i32)(bufDesc.size_ / sizeof(i32)) / 8;
		REQUIRE(cmdList.Dispatch(pbsHandle, xGroups, 1, 1));
		REQUIRE(manager.CompileCommandList(cmdHandle, cmdList));
		REQUIRE(manager.SubmitCommandList(cmdHandle));

		manager.DestroyResource(cmdHandle);
		manager.DestroyResource(pbsHandle);
		manager.DestroyResource(pipelineHandle);
		manager.DestroyResource(csHandle);
	}

	{
		GPU::ShaderDesc csDesc;
		csDesc.type_ = GPU::ShaderType::COMPUTE;
		csDesc.dataSize_ = sizeof(g_CTestTex);
		csDesc.data_ = g_CTestTex;

		GPU::Handle csHandle = manager.CreateShader(csDesc, testName.c_str());
		REQUIRE(csHandle);

		GPU::ComputePipelineStateDesc pipelineDesc;
		pipelineDesc.shader_ = csHandle;
		GPU::Handle pipelineHandle = manager.CreateComputePipelineState(pipelineDesc, testName.c_str());
		REQUIRE(pipelineHandle);

		GPU::PipelineBindingSetDesc pipelineBindingSetDesc;
		pipelineBindingSetDesc.pipelineState_ = pipelineHandle;
		pipelineBindingSetDesc.numUAVs_ = 1;
		pipelineBindingSetDesc.uavs_[0].resource_ = texHandle;
		pipelineBindingSetDesc.uavs_[0].format_ = GPU::Format::R8G8B8A8_UNORM;
		pipelineBindingSetDesc.uavs_[0].dimension_ = GPU::ViewDimension::TEX2D;
		pipelineBindingSetDesc.uavs_[0].mipSlice_FirstElement_ = 0;
		GPU::Handle pbsHandle = manager.CreatePipelineBindingSet(pipelineBindingSetDesc, testName.c_str());
		REQUIRE(pbsHandle);

		GPU::Handle cmdHandle = manager.CreateCommandList(testName.c_str());
		GPU::CommandList cmdList(manager.GetHandleAllocator());

		i32 xGroups = texDesc.width_ / 8;
		i32 yGroups = texDesc.height_ / 8;
		REQUIRE(cmdList.Dispatch(pbsHandle, xGroups, yGroups, 1));
		REQUIRE(manager.CompileCommandList(cmdHandle, cmdList));
		REQUIRE(manager.SubmitCommandList(cmdHandle));

		manager.DestroyResource(cmdHandle);
		manager.DestroyResource(pbsHandle);
		manager.DestroyResource(pipelineHandle);
		manager.DestroyResource(csHandle);
	}

	manager.DestroyResource(texHandle);
	manager.DestroyResource(bufHandle);
}

TEST_CASE("gpu-tests-compile-update-buffer")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_, debuggerIntegrationFlags);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(manager, testName.c_str());

	GPU::BufferDesc vbDesc;
	vbDesc.bindFlags_ = GPU::BindFlags::VERTEX_BUFFER;
	vbDesc.size_ = 8 * sizeof(f32);
	GPU::Handle vbHandle = manager.CreateBuffer(vbDesc, nullptr, testName.c_str());
	REQUIRE(vbHandle);

	GPU::Handle cmdHandle = manager.CreateCommandList(testName.c_str());
	GPU::CommandList cmdList(manager.GetHandleAllocator());

	f32 data[8] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f};
	REQUIRE(cmdList.UpdateBuffer(vbHandle, 0, sizeof(data), data));
	REQUIRE(manager.CompileCommandList(cmdHandle, cmdList));
	REQUIRE(manager.SubmitCommandList(cmdHandle));

	manager.DestroyResource(cmdHandle);
	manager.DestroyResource(vbHandle);
}

TEST_CASE("gpu-tests-compile-update-texture")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_, debuggerIntegrationFlags);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(manager, testName.c_str());

	GPU::TextureDesc texDesc;
	texDesc.type_ = GPU::TextureType::TEX1D;
	texDesc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
	texDesc.format_ = GPU::Format::R8G8B8A8_UNORM;
	texDesc.width_ = 8;
	GPU::Handle texHandle = manager.CreateTexture(texDesc, nullptr, testName.c_str());
	REQUIRE(texHandle);

	GPU::Handle cmdHandle = manager.CreateCommandList(testName.c_str());
	GPU::CommandList cmdList(manager.GetHandleAllocator());

	u32 data[8] = {0xff00ff00, 0xffff0000, 0x0000ffff, 0x00ff00ff, 0xff00ff00, 0xffff0000, 0x0000ffff, 0x00ff00ff};
	GPU::TextureSubResourceData texSubRscData;
	texSubRscData.data_ = data;
	texSubRscData.rowPitch_ = sizeof(data);
	texSubRscData.slicePitch_ = sizeof(data);
	REQUIRE(cmdList.UpdateTextureSubResource(texHandle, 0, texSubRscData));
	REQUIRE(manager.CompileCommandList(cmdHandle, cmdList));
	REQUIRE(manager.SubmitCommandList(cmdHandle));

	manager.DestroyResource(cmdHandle);
	manager.DestroyResource(texHandle);
}
