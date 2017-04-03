#include "client/client.h"
#include "client/input_provider.h"
#include "client/window.h"
#include "core/concurrency.h"
#include "gpu/manager.h"
#include "imgui/imgui.h"
#include "plugin/manager.h"

#include "catch.hpp"

namespace
{
	GPU::SetupParams GetDefaultSetupParams()
	{
		GPU::SetupParams setupParams;
		setupParams.debuggerIntegration_ = GPU::DebuggerIntegrationFlags::NONE;
		return setupParams;
	}
}

TEST_CASE("imgui-tests-run")
{
	auto testName = Catch::getResultCapture().getCurrentTestName();
	Plugin::Manager pluginManager;
	Client::Window window("imgui-tests", 100, 100, 1024, 768, true);
	GPU::Manager manager(pluginManager, GetDefaultSetupParams());

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

	const Client::IInputProvider& input = window.GetInputProvider();

	while(Client::Update() && (Core::IsDebuggerAttached() || testRunCounter-- > 0))
	{
		// Reset command list to reuse.
		cmdList.Reset();

		// Clear swapchain.
		cmdList.ClearRTV(fbsHandle, 0, color);

		ImGui::BeginFrame(input, scDesc.width_, scDesc.height_);

		ImGui::ShowTestWindow();

		ImGui::EndFrame(fbsHandle, cmdList);

		// Compile and submit.
		manager.CompileCommandList(cmdHandle, cmdList);
		manager.SubmitCommandList(cmdHandle);

		// Present.
		manager.PresentSwapChain(scHandle);

		// Next frame.
		manager.NextFrame();

		// Force a sleep.
		Core::Sleep(1.0 / 60.0);
	}

	ImGui::Finalize();

	manager.DestroyResource(cmdHandle);
	manager.DestroyResource(fbsHandle);
	manager.DestroyResource(scHandle);
}
