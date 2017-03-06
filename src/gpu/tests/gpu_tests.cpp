#include "gpu/manager.h"
#include "gpu/utils.h"
#include "client/window.h"
#include "core/vector.h"

#include "catch.hpp"


namespace
{
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

			//manager.DestroyResource(handle);
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

	SECTION("1d")
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

	SECTION("2d")
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

	SECTION("3d")
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

	SECTION("cube")
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
