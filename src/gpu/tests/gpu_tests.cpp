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
		REQUIRE(info.blockBits_> 0);
	}
}

TEST_CASE("gpu-tests-manager")
{
	Client::Window window("gpu_tests", 0, 0, 640, 480);
	GPU::Manager manager(window.GetPlatformData().handle_);
}

TEST_CASE("gpu-tests-enumerate")
{
	Client::Window window("gpu_tests", 0, 0, 640, 480);
	GPU::Manager manager(window.GetPlatformData().handle_);
	
	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);
	Core::Vector<GPU::AdapterInfo> adapterInfos;
	adapterInfos.resize(numAdapters);
	manager.EnumerateAdapters(adapterInfos.data(), adapterInfos.size());
}

TEST_CASE("gpu-tests-initialize")
{
	Client::Window window("gpu_tests", 0, 0, 640, 480);
	GPU::Manager manager(window.GetPlatformData().handle_);
	
	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.InitializeAdapter(0) == GPU::ErrorCode::OK);
}
