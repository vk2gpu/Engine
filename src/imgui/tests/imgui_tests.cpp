#include "client/manager.h"
#include "client/input_provider.h"
#include "client/window.h"
#include "core/concurrency.h"
#include "gpu/manager.h"
#include "imgui/manager.h"
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
	Client::Window window("imgui-tests", 100, 100, 1024, 768, true);

	Plugin::Manager::Scoped pluginManager;
	GPU::Manager::Scoped gpuManager(GetDefaultSetupParams());

	i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
	REQUIRE(numAdapters > 0);

	REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

	GPU::Manager::ScopedDebugCapture capture(testName.c_str());

	ImGui::Manager::Scoped imguiManager;

	GPU::SwapChainDesc scDesc;
	scDesc.width_ = 1024;
	scDesc.height_ = 768;
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
	REQUIRE(cmdHandle);
	GPU::CommandList cmdList(GPU::Manager::GetHandleAllocator());

	f32 color[] = {0.1f, 0.1f, 0.2f, 1.0f};

	i32 testRunCounter = GPU::MAX_GPU_FRAMES * 10;

	const Client::IInputProvider& input = window.GetInputProvider();

	while(Client::Manager::Update() && (Core::IsDebuggerAttached() || testRunCounter-- > 0))
	{
		// Reset command list to reuse.
		cmdList.Reset();

		// Clear swapchain.
		cmdList.ClearRTV(fbsHandle, 0, color);

		ImGui::Manager::BeginFrame(input, scDesc.width_, scDesc.height_);

		ImGui::ShowTestWindow();

		ImGui::Manager::EndFrame(fbsHandle, cmdList);

		// Compile and submit.
		GPU::Manager::CompileCommandList(cmdHandle, cmdList);
		GPU::Manager::SubmitCommandList(cmdHandle);

		// Present.
		GPU::Manager::PresentSwapChain(scHandle);

		// Next frame.
		GPU::Manager::NextFrame();

		// Force a sleep.
		Core::Sleep(1.0 / 60.0);
	}

	GPU::Manager::DestroyResource(cmdHandle);
	GPU::Manager::DestroyResource(fbsHandle);
	GPU::Manager::DestroyResource(scHandle);
}
