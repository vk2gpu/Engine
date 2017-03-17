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
	Client::Window window("gpu_tests", 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_);
}

TEST_CASE("gpu-tests-enumerate")
{
	Client::Window window("gpu_tests", 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_);
	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);
	Core::Vector<GPU::AdapterInfo> adapterInfos;
	adapterInfos.resize(numAdapters);
	manager.EnumerateAdapters(adapterInfos.data(), adapterInfos.size());
	Core::Log("gpu-tests-enumerate: Adapters:\n");
	for(const auto& adapterInfo : adapterInfos)
		Core::Log(" - %s\n", adapterInfo.description_);
}

TEST_CASE("gpu-tests-initialize")
{
	Client::Window window("gpu_tests", 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_);


	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);
}

TEST_CASE("gpu-tests-create-swapchain")
{
	Client::Window window("gpu_tests", 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::SwapChainDesc desc;
	desc.width_ = 640;
	desc.height_ = 480;
	desc.format_ = GPU::Format::R8G8B8A8_UNORM;
	desc.bufferCount_ = 2;
	desc.outputWindow_ = window.GetPlatformData().handle_;

	GPU::Handle handle;
	handle = manager.CreateSwapChain(desc, "gpu-tests-create-swapchain");
	REQUIRE(handle);

	manager.DestroyResource(handle);
}

TEST_CASE("gpu-tests-create-buffer")
{
	Client::Window window("gpu_tests", 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	SECTION("no-data")
	{
		GPU::BufferDesc desc;
		desc.bindFlags_ = GPU::BindFlags::VERTEX_BUFFER;
		desc.size_ = 32 * 1024;
		desc.stride_ = 0;

		GPU::Handle handle;
		handle = manager.CreateBuffer(desc, nullptr, "gpu-tests-create-buffer-no-data");
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
			desc.stride_ = 0;

			Core::Vector<u32> data;
			data.resize((i32)desc.size_);

			GPU::Handle handle;
			handle = manager.CreateBuffer(desc, data.data(), "gpu-tests-create-buffer-data");
			REQUIRE(handle);

			manager.DestroyResource(handle);
		}
	}
}

TEST_CASE("gpu-tests-create-texture")
{
	Client::Window window("gpu_tests", 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	SECTION("1d-no-data")
	{
		GPU::TextureDesc desc;
		desc.type_ = GPU::TextureType::TEX1D;
		desc.format_ = GPU::Format::R8G8B8A8_TYPELESS;
		desc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
		desc.width_ = 256;

		GPU::Handle handle;
		handle = manager.CreateTexture(desc, nullptr, "gpu-tests-create-texture-1d");
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
		handle = manager.CreateTexture(desc, nullptr, "gpu-tests-create-texture-2d");
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
		handle = manager.CreateTexture(desc, nullptr, "gpu-tests-create-texture-3d");
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
		handle = manager.CreateTexture(desc, nullptr, "gpu-tests-create-texture-cube");
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
		handle = manager.CreateTexture(desc, &subResData, "gpu-tests-create-texture-1d");
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
		handle = manager.CreateTexture(desc, &subResData, "gpu-tests-create-texture-2d");
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
		handle = manager.CreateTexture(desc, &subResData, "gpu-tests-create-texture-3d");
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
		handle = manager.CreateTexture(desc, subResDatas.data(), "gpu-tests-create-texture-cube");
		REQUIRE(handle);
		manager.DestroyResource(handle);
	}
}

TEST_CASE("gpu-tests-create-commandlist")
{
	Client::Window window("gpu_tests", 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::Handle handle;
	handle = manager.CreateCommandList("gpu-tests-create-commandlist");
	REQUIRE(handle);
	manager.DestroyResource(handle);
}

TEST_CASE("gpu-tests-create-shader")
{
	Client::Window window("gpu_tests", 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::ShaderDesc desc;
	desc.type_ = GPU::ShaderType::COMPUTE;
	desc.dataSize_ = sizeof(g_CShader);
	desc.data_ = g_CShader;

	GPU::Handle handle = manager.CreateShader(desc, "gpu-tests-create-shader");
	REQUIRE(handle);
	manager.DestroyResource(handle);
}


TEST_CASE("gpu-tests-create-compute-pipeline-state")
{
	Client::Window window("gpu_tests", 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::ShaderDesc shaderDesc;
	shaderDesc.type_ = GPU::ShaderType::COMPUTE;
	shaderDesc.dataSize_ = sizeof(g_CShader);
	shaderDesc.data_ = g_CShader;

	GPU::Handle shaderHandle = manager.CreateShader(shaderDesc, "gpu-tests-create-compute-pipeline-state");
	REQUIRE(shaderHandle);

	GPU::ComputePipelineStateDesc pipelineDesc;
	pipelineDesc.shader_ = shaderHandle;
	GPU::Handle pipelineHandle =
	    manager.CreateComputePipelineState(pipelineDesc, "gpu-tests-create-compute-pipeline-state");

	manager.DestroyResource(pipelineHandle);
	manager.DestroyResource(shaderHandle);
}

TEST_CASE("gpu-tests-create-pipeline-binding-set")
{
	Client::Window window("gpu_tests", 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

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
		texHandle = manager.CreateTexture(desc, nullptr, "gpu-tests-create-pipeline-binding-set");
		REQUIRE(texHandle);
	}

	{
		GPU::BufferDesc desc;
		desc.bindFlags_ = GPU::BindFlags::CONSTANT_BUFFER;
		desc.size_ = 4096;
		desc.stride_ = 0;
		cbHandle = manager.CreateBuffer(desc, nullptr, "gpu-tests-create-pipeline-binding-set");
		REQUIRE(cbHandle);
	}

	{
		GPU::SamplerState desc;
		samplerHandle = manager.CreateSamplerState(desc, "gpu-tests-create-pipeline-binding-set");
		REQUIRE(samplerHandle);
	}

	GPU::Handle shaderHandle = manager.CreateShader(shaderDesc, "gpu-tests-create-pipeline-binding-set");
	REQUIRE(shaderHandle);

	GPU::ComputePipelineStateDesc pipelineDesc;
	pipelineDesc.shader_ = shaderHandle;
	GPU::Handle pipelineHandle =
	    manager.CreateComputePipelineState(pipelineDesc, "gpu-tests-create-pipeline-binding-set");

	SECTION("no-views")
	{
		GPU::PipelineBindingSetDesc pipelineBindingSetDesc;
		pipelineBindingSetDesc.pipelineState_ = pipelineHandle;
		GPU::Handle pipelineBindingSetHandle =
		    manager.CreatePipelineBindingSet(pipelineBindingSetDesc, "gpu-tests-create-pipeline-binding-set-no-views");

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
		    manager.CreatePipelineBindingSet(pipelineBindingSetDesc, "gpu-tests-create-pipeline-binding-set-srv");

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
		    manager.CreatePipelineBindingSet(pipelineBindingSetDesc, "gpu-tests-create-pipeline-binding-set-cbv");

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
		    manager.CreatePipelineBindingSet(pipelineBindingSetDesc, "gpu-tests-create-pipeline-binding-set-uav");

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
	Client::Window window("gpu_tests", 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::ShaderDesc shaderDesc;
	shaderDesc.type_ = GPU::ShaderType::VERTEX;
	shaderDesc.dataSize_ = sizeof(g_VShader);
	shaderDesc.data_ = g_VShader;

	GPU::Handle shaderHandle = manager.CreateShader(shaderDesc, "gpu-tests-create-graphics-pipeline-state");
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
	GPU::Handle pipelineHandle =
	    manager.CreateGraphicsPipelineState(pipelineDesc, "gpu-tests-create-graphics-pipeline-state");

	manager.DestroyResource(pipelineHandle);
	manager.DestroyResource(shaderHandle);
}

TEST_CASE("gpu-tests-create-draw-binding-set")
{
	Client::Window window("gpu_tests", 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	float vertices[] = {
	    0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	};

	GPU::BufferDesc vbDesc;
	vbDesc.bindFlags_ = GPU::BindFlags::VERTEX_BUFFER;
	vbDesc.size_ = sizeof(vertices);
	vbDesc.stride_ = 0;
	GPU::Handle vbHandle = manager.CreateBuffer(vbDesc, vertices, "gpu-tests-create-draw-binding-set");
	REQUIRE(vbHandle);

	GPU::DrawBindingSetDesc dbsDesc;
	dbsDesc.vbs_[0].resource_ = vbHandle;
	dbsDesc.vbs_[0].offset_ = 0;
	dbsDesc.vbs_[0].size_ = (i32)vbDesc.size_;

	GPU::Handle dbsHandle = manager.CreateDrawBindingSet(dbsDesc, "gpu-tests-create-draw-binding-set");
	REQUIRE(dbsHandle);

	manager.DestroyResource(dbsHandle);
	manager.DestroyResource(vbHandle);
}

TEST_CASE("gpu-tests-create-frame-binding-set")
{
	Client::Window window("gpu_tests", 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::TextureDesc rtDesc;
	rtDesc.type_ = GPU::TextureType::TEX2D;
	rtDesc.bindFlags_ = GPU::BindFlags::RENDER_TARGET | GPU::BindFlags::SHADER_RESOURCE;
	rtDesc.width_ = 128;
	rtDesc.height_ = 128;
	rtDesc.format_ = GPU::Format::R8G8B8A8_UNORM;
	GPU::Handle rtHandle = manager.CreateTexture(rtDesc, nullptr, "gpu-tests-create-frame-binding-set");
	REQUIRE(rtHandle);

	GPU::TextureDesc dsDesc;
	dsDesc.type_ = GPU::TextureType::TEX2D;
	dsDesc.bindFlags_ = GPU::BindFlags::DEPTH_STENCIL;
	dsDesc.width_ = 128;
	dsDesc.height_ = 128;
	dsDesc.format_ = GPU::Format::D24_UNORM_S8_UINT;
	GPU::Handle dsHandle = manager.CreateTexture(dsDesc, nullptr, "gpu-tests-create-frame-binding-set");
	REQUIRE(dsHandle);

	GPU::FrameBindingSetDesc fbDesc;
	fbDesc.rtvs_[0].resource_ = rtHandle;
	fbDesc.rtvs_[0].format_ = rtDesc.format_;
	fbDesc.rtvs_[0].dimension_ = GPU::ViewDimension::TEX2D;
	fbDesc.dsv_.resource_ = dsHandle;
	fbDesc.dsv_.format_ = dsDesc.format_;
	fbDesc.dsv_.dimension_ = GPU::ViewDimension::TEX2D;

	GPU::Handle fbsHandle = manager.CreateFrameBindingSet(fbDesc, "gpu-tests-create-frame-binding-set");
	REQUIRE(fbsHandle);

	manager.DestroyResource(fbsHandle);
	manager.DestroyResource(dsHandle);
	manager.DestroyResource(rtHandle);
}

TEST_CASE("gpu-tests-clears")
{
	Client::Window window("gpu_tests", 0, 0, 640, 480, false);
	GPU::Manager manager(window.GetPlatformData().handle_);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::TextureDesc rtDesc;
	rtDesc.type_ = GPU::TextureType::TEX2D;
	rtDesc.bindFlags_ = GPU::BindFlags::RENDER_TARGET | GPU::BindFlags::SHADER_RESOURCE;
	rtDesc.width_ = 128;
	rtDesc.height_ = 128;
	rtDesc.format_ = GPU::Format::R8G8B8A8_UNORM;
	GPU::Handle rtHandle = manager.CreateTexture(rtDesc, nullptr, "gpu-tests-compile-clears");
	REQUIRE(rtHandle);

	GPU::TextureDesc dsDesc;
	dsDesc.type_ = GPU::TextureType::TEX2D;
	dsDesc.bindFlags_ = GPU::BindFlags::DEPTH_STENCIL;
	dsDesc.width_ = 128;
	dsDesc.height_ = 128;
	dsDesc.format_ = GPU::Format::D24_UNORM_S8_UINT;
	GPU::Handle dsHandle = manager.CreateTexture(dsDesc, nullptr, "gpu-tests-compile-clears");
	REQUIRE(dsHandle);

	GPU::FrameBindingSetDesc fbDesc;
	fbDesc.rtvs_[0].resource_ = rtHandle;
	fbDesc.rtvs_[0].format_ = rtDesc.format_;
	fbDesc.rtvs_[0].dimension_ = GPU::ViewDimension::TEX2D;
	fbDesc.dsv_.resource_ = dsHandle;
	fbDesc.dsv_.format_ = dsDesc.format_;
	fbDesc.dsv_.dimension_ = GPU::ViewDimension::TEX2D;

	GPU::Handle fbsHandle = manager.CreateFrameBindingSet(fbDesc, "gpu-tests-compile-clears");
	REQUIRE(fbsHandle);


	GPU::Handle cmdHandle = manager.CreateCommandList("gpu-tests-compile-clears");
	GPU::CommandList cmdList(manager.GetHandleAllocator());

	f32 color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	REQUIRE(cmdList.ClearRTV(fbsHandle, 0, color));
	REQUIRE(cmdList.ClearDSV(fbsHandle, 0.0f, 0));
	REQUIRE(manager.CompileCommandList(cmdHandle, cmdList));
	REQUIRE(manager.SubmitCommandList(cmdHandle));

	manager.DestroyResource(fbsHandle);
	manager.DestroyResource(dsHandle);
	manager.DestroyResource(rtHandle);
}
