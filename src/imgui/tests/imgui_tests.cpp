#include "client/client.h"
#include "client/window.h"
#include "core/concurrency.h"
#include "gpu/manager.h"
#include "imgui/imgui.h"

#include "catch.hpp"

#if PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

using namespace Core;

namespace
{
	GPU::DebuggerIntegrationFlags debuggerIntegrationFlags = GPU::DebuggerIntegrationFlags::NONE;
}

TEST_CASE("imgui-tests-run")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Client::Window window("imgui-tests", 100, 100, 1024, 768, true);
	GPU::Manager manager(window.GetPlatformData().handle_, debuggerIntegrationFlags);

	i32 numAdapters = manager.EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(manager.Initialize(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(manager, testName.c_str());

	GPU::SwapChainDesc scDesc;
	scDesc.width_ = 1024;
	scDesc.height_ = 768;
	scDesc.format_ = GPU::Format::R8G8B8A8_UNORM;
	scDesc.bufferCount_ = 2;
	scDesc.outputWindow_ = window.GetPlatformData().handle_;

	GPU::Handle scHandle;
	scHandle = manager.CreateSwapChain(scDesc, testName.c_str());
	REQUIRE(scHandle);

	GPU::FrameBindingSetDesc fbDesc;
	fbDesc.rtvs_[0].resource_ = scHandle;
	fbDesc.rtvs_[0].format_ = scDesc.format_;
	fbDesc.rtvs_[0].dimension_ = GPU::ViewDimension::TEX2D;

	GPU::Handle fbsHandle = manager.CreateFrameBindingSet(fbDesc, testName.c_str());
	REQUIRE(fbsHandle);

	GPU::Handle cmdHandle = manager.CreateCommandList(testName.c_str());
	REQUIRE(cmdHandle);
	GPU::CommandList cmdList(manager.GetHandleAllocator());

	ImGui::Initialize(manager);

	f32 color[] = {0.1f, 0.1f, 0.2f, 1.0f};

	i32 testRunCounter = GPU::MAX_GPU_FRAMES * 10;

	while(Client::PumpMessages() && (::IsDebuggerAttached() || testRunCounter-- > 0))
	{
		// Reset command list to reuse.
		cmdList.Reset();

		// Clear swapchain.
		cmdList.ClearRTV(fbsHandle, 0, color);

		ImGui::BeginFrame(scDesc.width_, scDesc.height_);

		ImGui::ShowTestWindow();

		ImGui::EndFrame(fbsHandle, cmdList);

		// Compile and submit.
		manager.CompileCommandList(cmdHandle, cmdList);
		manager.SubmitCommandList(cmdHandle);

		// Present.
		manager.PresentSwapChain(scHandle);

		// Next frame.
		manager.NextFrame();
	}

	ImGui::Finalize();

	manager.DestroyResource(cmdHandle);
	manager.DestroyResource(fbsHandle);
	manager.DestroyResource(scHandle);
}
