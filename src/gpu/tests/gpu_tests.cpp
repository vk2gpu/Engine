#include "gpu/manager.h"
#include "gpu/utils.h"
#include "client/window.h"
#include "core/array.h"
#include "core/concurrency.h"
#include "core/vector.h"

#include "plugin/manager.h"

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
	GPU::SetupParams GetDefaultSetupParams()
	{
		GPU::SetupParams setupParams;
		setupParams.debugFlags_ = GPU::DebugFlags::NONE;
		return setupParams;
	}
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

	Plugin::Manager::Scoped pluginManager;
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());
}

TEST_CASE("gpu-tests-enumerate")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();

	Plugin::Manager::Scoped pluginManager;
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);
	Core::Vector<GPU::AdapterInfo> adapterInfos;
	adapterInfos.resize(numAdapters);
	GPU::Manager::EnumerateAdapters(adapterInfos.data(), adapterInfos.size());
}

TEST_CASE("gpu-tests-create-adapter")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();

	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);
}

TEST_CASE("gpu-tests-create-swapchain")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();

	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	GPU::SwapChainDesc desc;
	desc.width_ = 640;
	desc.height_ = 480;
	desc.format_ = GPU::Format::R8G8B8A8_UNORM;
	desc.bufferCount_ = 2;
	desc.outputWindow_ = window.GetPlatformData().handle_;

	GPU::Handle handle;
	handle = GPU::Manager::CreateSwapChain(desc, testName.c_str());
	REQUIRE(handle);

	GPU::Manager::DestroyResource(handle);
}

TEST_CASE("gpu-tests-create-buffer")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();

	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	SECTION("no-data")
	{
		GPU::BufferDesc desc;
		desc.bindFlags_ = GPU::BindFlags::VERTEX_BUFFER;
		desc.size_ = 32 * 1024;

		GPU::Handle handle;
		handle = GPU::Manager::CreateBuffer(desc, nullptr, testName.c_str());
		REQUIRE(handle);

		GPU::Manager::DestroyResource(handle);
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
			handle = GPU::Manager::CreateBuffer(desc, data.data(), testName.c_str());
			REQUIRE(handle);

			GPU::Manager::DestroyResource(handle);
		}
	}
}

TEST_CASE("gpu-tests-create-texture")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();

	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	SECTION("1d-no-data")
	{
		GPU::TextureDesc desc;
		desc.type_ = GPU::TextureType::TEX1D;
		desc.format_ = GPU::Format::R8G8B8A8_TYPELESS;
		desc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
		desc.width_ = 256;

		GPU::Handle handle;
		handle = GPU::Manager::CreateTexture(desc, nullptr, testName.c_str());
		REQUIRE(handle);
		GPU::Manager::DestroyResource(handle);
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
		handle = GPU::Manager::CreateTexture(desc, nullptr, testName.c_str());
		REQUIRE(handle);
		GPU::Manager::DestroyResource(handle);
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
		handle = GPU::Manager::CreateTexture(desc, nullptr, testName.c_str());
		REQUIRE(handle);
		GPU::Manager::DestroyResource(handle);
	}

	SECTION("cube-no-data")
	{
		GPU::TextureDesc desc;
		desc.type_ = GPU::TextureType::TEXCUBE;
		desc.format_ = GPU::Format::R8G8B8A8_TYPELESS;
		desc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
		desc.width_ = 256;

		GPU::Handle handle;
		handle = GPU::Manager::CreateTexture(desc, nullptr, testName.c_str());
		REQUIRE(handle);
		GPU::Manager::DestroyResource(handle);
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

		GPU::ConstTextureSubResourceData subResData;
		subResData.data_ = data.data();
		subResData.rowPitch_ = layoutInfo.pitch_;
		subResData.slicePitch_ = layoutInfo.slicePitch_;

		GPU::Handle handle;
		handle = GPU::Manager::CreateTexture(desc, &subResData, testName.c_str());
		REQUIRE(handle);
		GPU::Manager::DestroyResource(handle);
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
		i64 size = GPU::GetTextureSize(desc.format_, desc.width_, desc.height_, 1, 1, 1);
		Core::Vector<u8> data;
		data.resize((i32)size);

		GPU::ConstTextureSubResourceData subResData;
		subResData.data_ = data.data();
		subResData.rowPitch_ = layoutInfo.pitch_;
		subResData.slicePitch_ = layoutInfo.slicePitch_;

		GPU::Handle handle;
		handle = GPU::Manager::CreateTexture(desc, &subResData, testName.c_str());
		REQUIRE(handle);
		GPU::Manager::DestroyResource(handle);
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

		GPU::ConstTextureSubResourceData subResData;
		subResData.data_ = data.data();
		subResData.rowPitch_ = layoutInfo.pitch_;
		subResData.slicePitch_ = layoutInfo.slicePitch_;

		GPU::Handle handle;
		handle = GPU::Manager::CreateTexture(desc, &subResData, testName.c_str());
		REQUIRE(handle);
		GPU::Manager::DestroyResource(handle);
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

		Core::Array<GPU::ConstTextureSubResourceData, 6> subResDatas;
		i64 offset = 0;
		for(i32 i = 0; i < 6; ++i)
		{
			subResDatas[i].data_ = data.data() + offset;
			subResDatas[i].rowPitch_ = layoutInfo.pitch_;
			subResDatas[i].slicePitch_ = layoutInfo.slicePitch_;
			offset += layoutInfo.slicePitch_;
		}

		GPU::Handle handle;
		handle = GPU::Manager::CreateTexture(desc, subResDatas.data(), testName.c_str());
		REQUIRE(handle);
		GPU::Manager::DestroyResource(handle);
	}
}

TEST_CASE("gpu-tests-create-commandlist")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();

	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	GPU::Handle handle;
	handle = GPU::Manager::CreateCommandList(testName.c_str());
	REQUIRE(handle);
	GPU::Manager::DestroyResource(handle);
}

TEST_CASE("gpu-tests-create-shader")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();

	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	GPU::ShaderDesc desc;
	desc.type_ = GPU::ShaderType::CS;
	desc.dataSize_ = sizeof(g_CShader);
	desc.data_ = g_CShader;

	GPU::Handle handle = GPU::Manager::CreateShader(desc, testName.c_str());
	REQUIRE(handle);
	GPU::Manager::DestroyResource(handle);
}


TEST_CASE("gpu-tests-create-compute-pipeline-state")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();

	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	GPU::ShaderDesc shaderDesc;
	shaderDesc.type_ = GPU::ShaderType::CS;
	shaderDesc.dataSize_ = sizeof(g_CShader);
	shaderDesc.data_ = g_CShader;

	GPU::Handle shaderHandle = GPU::Manager::CreateShader(shaderDesc, testName.c_str());
	REQUIRE(shaderHandle);

	GPU::ComputePipelineStateDesc pipelineDesc;
	pipelineDesc.shader_ = shaderHandle;
	GPU::Handle pipelineHandle = GPU::Manager::CreateComputePipelineState(pipelineDesc, testName.c_str());
	REQUIRE(pipelineHandle);

	GPU::Manager::DestroyResource(pipelineHandle);
	GPU::Manager::DestroyResource(shaderHandle);
}

TEST_CASE("gpu-tests-create-pipeline-binding-set")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();

	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	GPU::ShaderDesc shaderDesc;
	shaderDesc.type_ = GPU::ShaderType::CS;
	shaderDesc.dataSize_ = sizeof(g_CShader);
	shaderDesc.data_ = g_CShader;

	// Create resources to test bindings.
	GPU::Handle texHandle;
	GPU::Handle cbHandle;
	{
		GPU::TextureDesc desc;
		desc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE | GPU::BindFlags::UNORDERED_ACCESS;
		desc.type_ = GPU::TextureType::TEX2D;
		desc.width_ = 256;
		desc.height_ = 256;
		desc.format_ = GPU::Format::R8G8B8A8_UNORM;
		texHandle = GPU::Manager::CreateTexture(desc, nullptr, testName.c_str());
		REQUIRE(texHandle);
	}

	{
		GPU::BufferDesc desc;
		desc.bindFlags_ = GPU::BindFlags::CONSTANT_BUFFER;
		desc.size_ = 4096;
		cbHandle = GPU::Manager::CreateBuffer(desc, nullptr, testName.c_str());
		REQUIRE(cbHandle);
	}

	SECTION("no-views")
	{
		GPU::PipelineBindingSetDesc pipelineBindingSetDesc;
		GPU::Handle pipelineBindingSetHandle =
		    GPU::Manager::CreatePipelineBindingSet(pipelineBindingSetDesc, testName.c_str());

		GPU::Manager::DestroyResource(pipelineBindingSetHandle);
	}

	SECTION("srvs")
	{
		GPU::PipelineBindingSetDesc pipelineBindingSetDesc;
		pipelineBindingSetDesc.numSRVs_ = 1;
		GPU::Handle pipelineBindingSetHandle =
		    GPU::Manager::CreatePipelineBindingSet(pipelineBindingSetDesc, testName.c_str());
		REQUIRE(pipelineBindingSetHandle);

		GPU::BindingSRV srv;
		srv.resource_ = texHandle;
		srv.format_ = GPU::Format::R8G8B8A8_UNORM;
		srv.dimension_ = GPU::ViewDimension::TEX2D;
		srv.mipLevels_NumElements_ = -1;
		REQUIRE(GPU::Manager::UpdatePipelineBindings(pipelineBindingSetHandle, 0, srv));

		GPU::Manager::DestroyResource(pipelineBindingSetHandle);
	}

	SECTION("cbvs")
	{
		GPU::PipelineBindingSetDesc pipelineBindingSetDesc;
		pipelineBindingSetDesc.numCBVs_ = 1;
		GPU::Handle pipelineBindingSetHandle =
		    GPU::Manager::CreatePipelineBindingSet(pipelineBindingSetDesc, testName.c_str());
		REQUIRE(pipelineBindingSetHandle);

		GPU::BindingCBV cbv;
		cbv.resource_ = cbHandle;
		cbv.offset_ = 0;
		cbv.size_ = 4096;
		REQUIRE(GPU::Manager::UpdatePipelineBindings(pipelineBindingSetHandle, 0, cbv));

		GPU::Manager::DestroyResource(pipelineBindingSetHandle);
	}

	SECTION("uavs")
	{
		GPU::PipelineBindingSetDesc pipelineBindingSetDesc;
		pipelineBindingSetDesc.numUAVs_ = 1;

		GPU::Handle pipelineBindingSetHandle =
		    GPU::Manager::CreatePipelineBindingSet(pipelineBindingSetDesc, testName.c_str());
		REQUIRE(pipelineBindingSetHandle);

		GPU::BindingUAV uav;
		uav.resource_ = texHandle;
		uav.format_ = GPU::Format::R8G8B8A8_UNORM;
		uav.dimension_ = GPU::ViewDimension::TEX2D;
		REQUIRE(GPU::Manager::UpdatePipelineBindings(pipelineBindingSetHandle, 0, uav));

		GPU::Manager::DestroyResource(pipelineBindingSetHandle);
	}

	GPU::Manager::DestroyResource(cbHandle);
	GPU::Manager::DestroyResource(texHandle);
}

TEST_CASE("gpu-tests-create-graphics-pipeline-state")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();

	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	GPU::ShaderDesc shaderDesc;
	shaderDesc.type_ = GPU::ShaderType::VS;
	shaderDesc.dataSize_ = sizeof(g_VShader);
	shaderDesc.data_ = g_VShader;

	GPU::Handle shaderHandle = GPU::Manager::CreateShader(shaderDesc, testName.c_str());
	REQUIRE(shaderHandle);

	GPU::GraphicsPipelineStateDesc pipelineDesc;
	pipelineDesc.shaders_[(i32)GPU::ShaderType::VS] = shaderHandle;
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
	GPU::Handle pipelineHandle = GPU::Manager::CreateGraphicsPipelineState(pipelineDesc, testName.c_str());
	REQUIRE(pipelineHandle);

	GPU::Manager::DestroyResource(pipelineHandle);
	GPU::Manager::DestroyResource(shaderHandle);
}

TEST_CASE("gpu-tests-create-draw-binding-set")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();

	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	float vertices[] = {
	    0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	};

	GPU::BufferDesc vbDesc;
	vbDesc.bindFlags_ = GPU::BindFlags::VERTEX_BUFFER;
	vbDesc.size_ = sizeof(vertices);
	GPU::Handle vbHandle = GPU::Manager::CreateBuffer(vbDesc, vertices, testName.c_str());
	REQUIRE(vbHandle);

	GPU::DrawBindingSetDesc dbsDesc;
	dbsDesc.vbs_[0].resource_ = vbHandle;
	dbsDesc.vbs_[0].offset_ = 0;
	dbsDesc.vbs_[0].size_ = (i32)vbDesc.size_;

	GPU::Handle dbsHandle = GPU::Manager::CreateDrawBindingSet(dbsDesc, testName.c_str());
	REQUIRE(dbsHandle);

	GPU::Manager::DestroyResource(dbsHandle);
	GPU::Manager::DestroyResource(vbHandle);
}

TEST_CASE("gpu-tests-create-frame-binding-set")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();

	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	GPU::TextureDesc rtDesc;
	rtDesc.type_ = GPU::TextureType::TEX2D;
	rtDesc.bindFlags_ = GPU::BindFlags::RENDER_TARGET | GPU::BindFlags::SHADER_RESOURCE;
	rtDesc.width_ = 128;
	rtDesc.height_ = 128;
	rtDesc.format_ = GPU::Format::R8G8B8A8_UNORM;
	GPU::Handle rtHandle = GPU::Manager::CreateTexture(rtDesc, nullptr, testName.c_str());
	REQUIRE(rtHandle);

	GPU::TextureDesc dsDesc;
	dsDesc.type_ = GPU::TextureType::TEX2D;
	dsDesc.bindFlags_ = GPU::BindFlags::DEPTH_STENCIL;
	dsDesc.width_ = 128;
	dsDesc.height_ = 128;
	dsDesc.format_ = GPU::Format::D24_UNORM_S8_UINT;
	GPU::Handle dsHandle = GPU::Manager::CreateTexture(dsDesc, nullptr, testName.c_str());
	REQUIRE(dsHandle);

	GPU::FrameBindingSetDesc fbDesc;
	fbDesc.rtvs_[0].resource_ = rtHandle;
	fbDesc.rtvs_[0].format_ = rtDesc.format_;
	fbDesc.rtvs_[0].dimension_ = GPU::ViewDimension::TEX2D;
	fbDesc.dsv_.resource_ = dsHandle;
	fbDesc.dsv_.format_ = dsDesc.format_;
	fbDesc.dsv_.dimension_ = GPU::ViewDimension::TEX2D;

	GPU::Handle fbsHandle = GPU::Manager::CreateFrameBindingSet(fbDesc, testName.c_str());
	REQUIRE(fbsHandle);

	GPU::Manager::DestroyResource(fbsHandle);
	GPU::Manager::DestroyResource(dsHandle);
	GPU::Manager::DestroyResource(rtHandle);
}

TEST_CASE("gpu-tests-clears")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	GPU::TextureDesc rtDesc;
	rtDesc.type_ = GPU::TextureType::TEX2D;
	rtDesc.bindFlags_ = GPU::BindFlags::RENDER_TARGET | GPU::BindFlags::SHADER_RESOURCE;
	rtDesc.width_ = 128;
	rtDesc.height_ = 128;
	rtDesc.format_ = GPU::Format::R8G8B8A8_UNORM;
	GPU::Handle rtHandle = GPU::Manager::CreateTexture(rtDesc, nullptr, testName.c_str());
	REQUIRE(rtHandle);

	GPU::TextureDesc dsDesc;
	dsDesc.type_ = GPU::TextureType::TEX2D;
	dsDesc.bindFlags_ = GPU::BindFlags::DEPTH_STENCIL;
	dsDesc.width_ = 128;
	dsDesc.height_ = 128;
	dsDesc.format_ = GPU::Format::D24_UNORM_S8_UINT;
	GPU::Handle dsHandle = GPU::Manager::CreateTexture(dsDesc, nullptr, testName.c_str());
	REQUIRE(dsHandle);

	GPU::FrameBindingSetDesc fbDesc;
	fbDesc.rtvs_[0].resource_ = rtHandle;
	fbDesc.rtvs_[0].format_ = rtDesc.format_;
	fbDesc.rtvs_[0].dimension_ = GPU::ViewDimension::TEX2D;
	fbDesc.dsv_.resource_ = dsHandle;
	fbDesc.dsv_.format_ = dsDesc.format_;
	fbDesc.dsv_.dimension_ = GPU::ViewDimension::TEX2D;

	GPU::Handle fbsHandle = GPU::Manager::CreateFrameBindingSet(fbDesc, testName.c_str());
	REQUIRE(fbsHandle);

	GPU::Handle cmdHandle = GPU::Manager::CreateCommandList(testName.c_str());
	GPU::CommandList cmdList;

	f32 color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	REQUIRE(cmdList.ClearRTV(fbsHandle, 0, color));
	REQUIRE(cmdList.ClearDSV(fbsHandle, 0.0f, 0));
	REQUIRE(GPU::Manager::CompileCommandList(cmdHandle, cmdList));
	REQUIRE(GPU::Manager::SubmitCommandList(cmdHandle));

	GPU::Manager::DestroyResource(cmdHandle);
	GPU::Manager::DestroyResource(fbsHandle);
	GPU::Manager::DestroyResource(dsHandle);
	GPU::Manager::DestroyResource(rtHandle);
}

TEST_CASE("gpu-tests-compile-draw")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	float vertices[] = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f};

	GPU::BufferDesc vbDesc;
	vbDesc.bindFlags_ = GPU::BindFlags::VERTEX_BUFFER;
	vbDesc.size_ = sizeof(vertices);
	GPU::Handle vbHandle = GPU::Manager::CreateBuffer(vbDesc, vertices, testName.c_str());
	REQUIRE(vbHandle);

	GPU::DrawBindingSetDesc dbsDesc;
	dbsDesc.vbs_[0].resource_ = vbHandle;
	dbsDesc.vbs_[0].offset_ = 0;
	dbsDesc.vbs_[0].size_ = (i32)vbDesc.size_;
	dbsDesc.vbs_[0].stride_ = sizeof(float) * 4;

	GPU::Handle dbsHandle = GPU::Manager::CreateDrawBindingSet(dbsDesc, testName.c_str());
	REQUIRE(dbsHandle);

	GPU::TextureDesc rtDesc;
	rtDesc.type_ = GPU::TextureType::TEX2D;
	rtDesc.bindFlags_ = GPU::BindFlags::RENDER_TARGET | GPU::BindFlags::SHADER_RESOURCE;
	rtDesc.width_ = 128;
	rtDesc.height_ = 128;
	rtDesc.format_ = GPU::Format::R8G8B8A8_UNORM;
	GPU::Handle rtHandle = GPU::Manager::CreateTexture(rtDesc, nullptr, testName.c_str());
	REQUIRE(rtHandle);

	GPU::TextureDesc dsDesc;
	dsDesc.type_ = GPU::TextureType::TEX2D;
	dsDesc.bindFlags_ = GPU::BindFlags::DEPTH_STENCIL;
	dsDesc.width_ = 128;
	dsDesc.height_ = 128;
	dsDesc.format_ = GPU::Format::D24_UNORM_S8_UINT;
	GPU::Handle dsHandle = GPU::Manager::CreateTexture(dsDesc, nullptr, testName.c_str());
	REQUIRE(dsHandle);

	GPU::FrameBindingSetDesc fbDesc;
	fbDesc.rtvs_[0].resource_ = rtHandle;
	fbDesc.rtvs_[0].format_ = rtDesc.format_;
	fbDesc.rtvs_[0].dimension_ = GPU::ViewDimension::TEX2D;
	fbDesc.dsv_.resource_ = dsHandle;
	fbDesc.dsv_.format_ = dsDesc.format_;
	fbDesc.dsv_.dimension_ = GPU::ViewDimension::TEX2D;

	GPU::Handle fbsHandle = GPU::Manager::CreateFrameBindingSet(fbDesc, testName.c_str());
	REQUIRE(fbsHandle);

	GPU::ShaderDesc vsDesc;
	vsDesc.type_ = GPU::ShaderType::VS;
	vsDesc.dataSize_ = sizeof(g_VShader);
	vsDesc.data_ = g_VShader;

	GPU::Handle vsHandle = GPU::Manager::CreateShader(vsDesc, testName.c_str());
	REQUIRE(vsHandle);

	GPU::ShaderDesc psDesc;
	psDesc.type_ = GPU::ShaderType::VS;
	psDesc.dataSize_ = sizeof(g_PShader);
	psDesc.data_ = g_PShader;

	GPU::Handle psHandle = GPU::Manager::CreateShader(psDesc, testName.c_str());
	REQUIRE(psHandle);

	GPU::GraphicsPipelineStateDesc pipelineDesc;
	pipelineDesc.shaders_[(i32)GPU::ShaderType::VS] = vsHandle;
	pipelineDesc.shaders_[(i32)GPU::ShaderType::PS] = psHandle;
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
	GPU::Handle pipelineHandle = GPU::Manager::CreateGraphicsPipelineState(pipelineDesc, testName.c_str());

	GPU::Handle cmdHandle = GPU::Manager::CreateCommandList(testName.c_str());
	GPU::CommandList cmdList;

	GPU::DrawState drawState;
	drawState.viewport_.w_ = (f32)rtDesc.width_;
	drawState.viewport_.h_ = (f32)rtDesc.height_;
	drawState.scissorRect_.w_ = rtDesc.width_;
	drawState.scissorRect_.h_ = rtDesc.height_;

	GPU::PipelineBindingSetDesc pbDesc = {};
	GPU::PipelineBinding pb = {};
	pb.pbs_ = GPU::Manager::AllocTemporaryPipelineBindingSet(pbDesc);

	f32 color[4] = {0.2f, 0.2f, 0.2f, 1.0f};
	REQUIRE(cmdList.ClearRTV(fbsHandle, 0, color));
	REQUIRE(cmdList.ClearDSV(fbsHandle, 0.0f, 0));
	REQUIRE(cmdList.Draw(
	    pipelineHandle, pb, dbsHandle, fbsHandle, drawState, GPU::PrimitiveTopology::TRIANGLE_LIST, 0, 1, 3, 0, 1));
	REQUIRE(GPU::Manager::CompileCommandList(cmdHandle, cmdList));
	REQUIRE(GPU::Manager::SubmitCommandList(cmdHandle));

	GPU::Manager::DestroyResource(cmdHandle);
	GPU::Manager::DestroyResource(pipelineHandle);
	GPU::Manager::DestroyResource(psHandle);
	GPU::Manager::DestroyResource(vsHandle);
	GPU::Manager::DestroyResource(dbsHandle);
	GPU::Manager::DestroyResource(vbHandle);
	GPU::Manager::DestroyResource(fbsHandle);
	GPU::Manager::DestroyResource(dsHandle);
	GPU::Manager::DestroyResource(rtHandle);
}

TEST_CASE("gpu-tests-compile-dispatch")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	float vertices[] = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f};

	GPU::BufferDesc bufDesc;
	bufDesc.bindFlags_ = GPU::BindFlags::UNORDERED_ACCESS;
	bufDesc.size_ = 256;
	GPU::Handle bufHandle = GPU::Manager::CreateBuffer(bufDesc, vertices, testName.c_str());
	REQUIRE(bufHandle);

	GPU::TextureDesc texDesc;
	texDesc.type_ = GPU::TextureType::TEX2D;
	texDesc.bindFlags_ = GPU::BindFlags::UNORDERED_ACCESS;
	texDesc.width_ = 128;
	texDesc.height_ = 128;
	texDesc.format_ = GPU::Format::R8G8B8A8_UNORM;
	GPU::Handle texHandle = GPU::Manager::CreateTexture(texDesc, nullptr, testName.c_str());
	REQUIRE(texHandle);

	{
		GPU::ShaderDesc csDesc;
		csDesc.type_ = GPU::ShaderType::CS;
		csDesc.dataSize_ = sizeof(g_CTestBuf);
		csDesc.data_ = g_CTestBuf;

		GPU::Handle csHandle = GPU::Manager::CreateShader(csDesc, testName.c_str());
		REQUIRE(csHandle);

		GPU::ComputePipelineStateDesc pipelineDesc;
		pipelineDesc.shader_ = csHandle;
		GPU::Handle pipelineHandle = GPU::Manager::CreateComputePipelineState(pipelineDesc, testName.c_str());
		REQUIRE(pipelineHandle);

		GPU::PipelineBindingSetDesc pipelineBindingSetDesc;
		pipelineBindingSetDesc.numUAVs_ = 1;

		GPU::Handle pbsHandle = GPU::Manager::CreatePipelineBindingSet(pipelineBindingSetDesc, testName.c_str());
		REQUIRE(pbsHandle);

		GPU::BindingUAV uav;
		uav.resource_ = bufHandle;
		uav.format_ = GPU::Format::R32_TYPELESS;
		uav.dimension_ = GPU::ViewDimension::BUFFER;
		uav.mipSlice_FirstElement_ = 0;
		uav.firstArraySlice_FirstWSlice_NumElements_ = (i32)(bufDesc.size_ / sizeof(i32));
		REQUIRE(GPU::Manager::UpdatePipelineBindings(pbsHandle, 0, uav));

		GPU::Handle cmdHandle = GPU::Manager::CreateCommandList(testName.c_str());
		GPU::CommandList cmdList;

		GPU::PipelineBinding pb = {pbsHandle};
		pb.uavs_.num_ = 1;

		i32 xGroups = (i32)(bufDesc.size_ / sizeof(i32)) / 8;
		REQUIRE(cmdList.Dispatch(pipelineHandle, pb, xGroups, 1, 1));
		REQUIRE(GPU::Manager::CompileCommandList(cmdHandle, cmdList));
		REQUIRE(GPU::Manager::SubmitCommandList(cmdHandle));

		GPU::Manager::DestroyResource(cmdHandle);
		GPU::Manager::DestroyResource(pbsHandle);
		GPU::Manager::DestroyResource(pipelineHandle);
		GPU::Manager::DestroyResource(csHandle);
	}

	{
		GPU::ShaderDesc csDesc;
		csDesc.type_ = GPU::ShaderType::CS;
		csDesc.dataSize_ = sizeof(g_CTestTex);
		csDesc.data_ = g_CTestTex;

		GPU::Handle csHandle = GPU::Manager::CreateShader(csDesc, testName.c_str());
		REQUIRE(csHandle);

		GPU::ComputePipelineStateDesc pipelineDesc;
		pipelineDesc.shader_ = csHandle;
		GPU::Handle pipelineHandle = GPU::Manager::CreateComputePipelineState(pipelineDesc, testName.c_str());
		REQUIRE(pipelineHandle);

		GPU::PipelineBindingSetDesc pipelineBindingSetDesc;
		pipelineBindingSetDesc.numUAVs_ = 1;

		GPU::Handle pbsHandle = GPU::Manager::CreatePipelineBindingSet(pipelineBindingSetDesc, testName.c_str());
		REQUIRE(pbsHandle);

		GPU::BindingUAV uav;
		uav.resource_ = texHandle;
		uav.format_ = GPU::Format::R8G8B8A8_UNORM;
		uav.dimension_ = GPU::ViewDimension::TEX2D;
		uav.mipSlice_FirstElement_ = 0;
		REQUIRE(GPU::Manager::UpdatePipelineBindings(pbsHandle, 0, uav));

		GPU::Handle cmdHandle = GPU::Manager::CreateCommandList(testName.c_str());
		GPU::CommandList cmdList;

		i32 xGroups = texDesc.width_ / 8;
		i32 yGroups = texDesc.height_ / 8;
		GPU::PipelineBinding pipelineBinding[] = {{pbsHandle, 0, 0, 0, 0}};

		REQUIRE(cmdList.Dispatch(pipelineHandle, pipelineBinding, xGroups, yGroups, 1));
		REQUIRE(GPU::Manager::CompileCommandList(cmdHandle, cmdList));
		REQUIRE(GPU::Manager::SubmitCommandList(cmdHandle));

		GPU::Manager::DestroyResource(cmdHandle);
		GPU::Manager::DestroyResource(pbsHandle);
		GPU::Manager::DestroyResource(pipelineHandle);
		GPU::Manager::DestroyResource(csHandle);
	}

	GPU::Manager::DestroyResource(texHandle);
	GPU::Manager::DestroyResource(bufHandle);
}

TEST_CASE("gpu-tests-compile-update-buffer")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	GPU::BufferDesc vbDesc;
	vbDesc.bindFlags_ = GPU::BindFlags::VERTEX_BUFFER;
	vbDesc.size_ = 8 * sizeof(f32);
	GPU::Handle vbHandle = GPU::Manager::CreateBuffer(vbDesc, nullptr, testName.c_str());
	REQUIRE(vbHandle);

	GPU::Handle cmdHandle = GPU::Manager::CreateCommandList(testName.c_str());
	GPU::CommandList cmdList;

	f32 data[8] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f};
	REQUIRE(cmdList.UpdateBuffer(vbHandle, 0, sizeof(data), data));
	REQUIRE(GPU::Manager::CompileCommandList(cmdHandle, cmdList));
	REQUIRE(GPU::Manager::SubmitCommandList(cmdHandle));

	GPU::Manager::DestroyResource(cmdHandle);
	GPU::Manager::DestroyResource(vbHandle);
}

TEST_CASE("gpu-tests-compile-update-texture")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	GPU::TextureDesc texDesc;
	texDesc.type_ = GPU::TextureType::TEX1D;
	texDesc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
	texDesc.format_ = GPU::Format::R8G8B8A8_UNORM;
	texDesc.width_ = 8;
	GPU::Handle texHandle = GPU::Manager::CreateTexture(texDesc, nullptr, testName.c_str());
	REQUIRE(texHandle);

	GPU::Handle cmdHandle = GPU::Manager::CreateCommandList(testName.c_str());
	GPU::CommandList cmdList;

	u32 data[8] = {0xff00ff00, 0xffff0000, 0x0000ffff, 0x00ff00ff, 0xff00ff00, 0xffff0000, 0x0000ffff, 0x00ff00ff};
	GPU::ConstTextureSubResourceData texSubRscData;
	texSubRscData.data_ = data;
	texSubRscData.rowPitch_ = sizeof(data);
	texSubRscData.slicePitch_ = sizeof(data);
	REQUIRE(cmdList.UpdateTextureSubResource(texHandle, 0, texSubRscData));
	REQUIRE(GPU::Manager::CompileCommandList(cmdHandle, cmdList));
	REQUIRE(GPU::Manager::SubmitCommandList(cmdHandle));

	GPU::Manager::DestroyResource(cmdHandle);
	GPU::Manager::DestroyResource(texHandle);
}

TEST_CASE("gpu-tests-compile-copy-buffer")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	f32 data[8] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f};
	GPU::BufferDesc vbDesc;
	vbDesc.bindFlags_ = GPU::BindFlags::VERTEX_BUFFER;
	vbDesc.size_ = 8 * sizeof(f32);
	GPU::Handle vb0Handle = GPU::Manager::CreateBuffer(vbDesc, data, testName.c_str());
	GPU::Handle vb1Handle = GPU::Manager::CreateBuffer(vbDesc, nullptr, testName.c_str());
	REQUIRE(vb0Handle);
	REQUIRE(vb1Handle);

	GPU::Handle cmdHandle = GPU::Manager::CreateCommandList(testName.c_str());
	GPU::CommandList cmdList;

	REQUIRE(cmdList.CopyBuffer(vb1Handle, 0, vb0Handle, 0, sizeof(data)));
	REQUIRE(GPU::Manager::CompileCommandList(cmdHandle, cmdList));
	REQUIRE(GPU::Manager::SubmitCommandList(cmdHandle));

	GPU::Manager::DestroyResource(cmdHandle);
	GPU::Manager::DestroyResource(vb1Handle);
	GPU::Manager::DestroyResource(vb0Handle);
}

TEST_CASE("gpu-tests-compile-copy-texture")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	u32 data[8] = {0xff00ff00, 0xffff0000, 0x0000ffff, 0x00ff00ff, 0xff00ff00, 0xffff0000, 0x0000ffff, 0x00ff00ff};
	GPU::ConstTextureSubResourceData texSubRscData;
	texSubRscData.data_ = data;
	texSubRscData.rowPitch_ = sizeof(data);
	texSubRscData.slicePitch_ = sizeof(data);
	GPU::TextureDesc texDesc;
	texDesc.type_ = GPU::TextureType::TEX1D;
	texDesc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
	texDesc.format_ = GPU::Format::R8G8B8A8_UNORM;
	texDesc.width_ = 8;
	GPU::Handle tex0Handle = GPU::Manager::CreateTexture(texDesc, &texSubRscData, testName.c_str());
	GPU::Handle tex1Handle = GPU::Manager::CreateTexture(texDesc, nullptr, testName.c_str());
	REQUIRE(tex0Handle);
	REQUIRE(tex1Handle);

	GPU::Handle cmdHandle = GPU::Manager::CreateCommandList(testName.c_str());
	GPU::CommandList cmdList;

	GPU::Point dstPoint;
	GPU::Box srcBox;
	texDesc.width_ = 8;

	REQUIRE(cmdList.CopyTextureSubResource(tex1Handle, 0, dstPoint, tex0Handle, 0, srcBox));
	REQUIRE(GPU::Manager::CompileCommandList(cmdHandle, cmdList));
	REQUIRE(GPU::Manager::SubmitCommandList(cmdHandle));

	GPU::Manager::DestroyResource(cmdHandle);
	GPU::Manager::DestroyResource(tex1Handle);
	GPU::Manager::DestroyResource(tex0Handle);
}

TEST_CASE("gpu-tests-compile-present")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, true);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	GPU::SwapChainDesc scDesc;
	scDesc.width_ = 640;
	scDesc.height_ = 480;
	scDesc.format_ = GPU::Format::R8G8B8A8_UNORM;
	scDesc.bufferCount_ = 2;
	scDesc.outputWindow_ = window.GetPlatformData().handle_;

	GPU::Handle scHandle;
	scHandle = GPU::Manager::CreateSwapChain(scDesc, testName.c_str());
	REQUIRE(scHandle);

	GPU::FrameBindingSetDesc fbDesc;
	fbDesc.rtvs_[0].resource_ = scHandle;
	fbDesc.rtvs_[0].format_ = scDesc.format_;
	fbDesc.rtvs_[0].dimension_ = GPU::ViewDimension::TEX2D;

	GPU::Handle fbsHandle = GPU::Manager::CreateFrameBindingSet(fbDesc, testName.c_str());
	REQUIRE(fbsHandle);

	GPU::Handle cmdHandle = GPU::Manager::CreateCommandList(testName.c_str());
	GPU::CommandList cmdList;

	// clang-format off
	f32 colors[] = {
		1.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 0.0f, 1.0f,
	};
	// clang-format on

	for(i32 i = 0; i < 6; ++i)
	{
		REQUIRE(cmdList.ClearRTV(fbsHandle, 0, &colors[i * 4]));
		REQUIRE(GPU::Manager::CompileCommandList(cmdHandle, cmdList));
		REQUIRE(GPU::Manager::SubmitCommandList(cmdHandle));

		GPU::Manager::PresentSwapChain(scHandle);
		cmdList.Reset();
	}

	GPU::Manager::DestroyResource(cmdHandle);
	GPU::Manager::DestroyResource(fbsHandle);
	GPU::Manager::DestroyResource(scHandle);
}

TEST_CASE("gpu-tests-mt-create-buffers")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Plugin::Manager::Scoped pluginManager;
	Client::Window window(testName.c_str(), 0, 0, 640, 480, false);
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	Core::Vector<Core::Thread> threads;

	struct Locals
	{
		volatile i32 sync_ = 0;
		i32 total_ = 32;
	};

	Locals locals;
	for(i32 i = 0; i < locals.total_; ++i)
	{

		threads.emplace_back(Core::Thread(
		    [](void* userData) -> int {
			    Locals& locals = *reinterpret_cast<Locals*>(userData);

			    i32 val = Core::AtomicInc(&locals.sync_);
			    while(locals.sync_ < locals.total_)
				    Core::SwitchThread();

			    GPU::BufferDesc desc;
			    desc.bindFlags_ = GPU::BindFlags::VERTEX_BUFFER;
			    desc.size_ = 32 * 1024;

			    Core::Vector<u32> data;
			    data.resize((i32)desc.size_);

			    GPU::Handle handle;
			    handle = GPU::Manager::CreateBuffer(desc, data.data(), "threaded");
			    DBG_ASSERT(handle);

			    val = Core::AtomicInc(&locals.sync_);
			    while(locals.sync_ < (locals.total_ * 2))
				    Core::SwitchThread();

			    GPU::Manager::DestroyResource(handle);
			    Core::AtomicInc(&(locals.sync_));
			    return 0;
			},
		    &locals));
	}

	while(locals.sync_ < (locals.total_ * 3))
		Core::SwitchThread();
}