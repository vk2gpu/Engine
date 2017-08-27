#pragma once
#include "core/debug.h"
#include "core/file.h"
#include "core/random.h"
#include "core/timer.h"
#include "core/vector.h"
#include "core/os.h"
#include "client/window.h"
#include "gpu/manager.h"
#include "gpu/utils.h"
#include "job/manager.h"
#include "plugin/manager.h"
#include "resource/manager.h"

#include "graphics/model.h"
#include "graphics/shader.h"
#include "graphics/texture.h"

namespace
{
	GPU::SetupParams GetDefaultSetupParams()
	{
		GPU::SetupParams setupParams;
		setupParams.debuggerIntegration_ = GPU::DebuggerIntegrationFlags::NONE;
		return setupParams;
	}

	class ScopedEngine
	{
	public:
		Client::Window window;
		Plugin::Manager::Scoped pluginManager;
		GPU::Manager::Scoped gpuManager = GPU::Manager::Scoped(GetDefaultSetupParams());
		Job::Manager::Scoped jobManager = Job::Manager::Scoped(Core::GetNumLogicalCores(), 256, 32 * 1024);
		Resource::Manager::Scoped resourceManager;

		GPU::SwapChainDesc scDesc;
		GPU::Handle scHandle;
		GPU::Handle fbsHandle;

		ScopedEngine(const char* name)
		    : window(name, 100, 100, 1024, 768, true)
		{
			Graphics::Model::RegisterFactory();
			Graphics::Shader::RegisterFactory();
			Graphics::Texture::RegisterFactory();

			// Init device.
			i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
			DBG_ASSERT(numAdapters > 0);
			DBG_ASSERT(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

			// Create swap chain.
			scDesc.width_ = 1024;
			scDesc.height_ = 768;
			scDesc.format_ = GPU::Format::R8G8B8A8_UNORM;
			scDesc.bufferCount_ = 2;
			scDesc.outputWindow_ = window.GetPlatformData().handle_;

			scHandle = GPU::Manager::CreateSwapChain(scDesc, "ScopedEngine");
			DBG_ASSERT(scHandle);

			// Create frame buffer binding set.
			GPU::FrameBindingSetDesc fbDesc;
			fbDesc.rtvs_[0].resource_ = scHandle;
			fbDesc.rtvs_[0].format_ = scDesc.format_;
			fbDesc.rtvs_[0].dimension_ = GPU::ViewDimension::TEX2D;

			fbsHandle = GPU::Manager::CreateFrameBindingSet(fbDesc, "ScopedEngine");
			DBG_ASSERT(fbsHandle);
		}

		~ScopedEngine()
		{
			GPU::Manager::DestroyResource(fbsHandle);
			GPU::Manager::DestroyResource(scHandle);
			Graphics::Model::UnregisterFactory();
			Graphics::Shader::UnregisterFactory();
			Graphics::Texture::UnregisterFactory();
		}
	};
}
