#pragma once
#include "core/command_line.h"
#include "core/debug.h"
#include "core/file.h"
#include "core/random.h"
#include "core/string.h"
#include "core/timer.h"
#include "core/vector.h"
#include "core/os.h"
#include "client/window.h"
#include "gpu/manager.h"
#include "gpu/utils.h"
#include "job/manager.h"
#include "plugin/manager.h"
#include "resource/manager.h"

#include "graphics/material.h"
#include "graphics/model.h"
#include "graphics/shader.h"
#include "graphics/texture.h"

#include "Remotery.h"

namespace
{
	GPU::SetupParams GetDefaultSetupParams(const Core::CommandLine& cmdLine)
	{
		GPU::SetupParams setupParams;
		if(cmdLine.HasArg(0, "renderdoc"))
			setupParams.debugFlags_ |= GPU::DebugFlags::RENDERDOC;
		if(cmdLine.HasArg(0, "gpubasedvalidation"))
			setupParams.debugFlags_ |= GPU::DebugFlags::GPU_BASED_VALIDATION;
		return setupParams;
	}

	i32 GetNumJobWorkers(const Core::CommandLine& cmdLine)
	{
		i32 numWorkers = Core::GetNumLogicalCores();
		Core::String val;
		if(cmdLine.GetArg(0, "jobworkers", val))
			numWorkers = atoi(val.c_str());
		return numWorkers;
	}

	class ScopedEngine
	{
	public:
		struct ScopedRemotery
		{
			ScopedRemotery()
			{
				auto* settings = rmt_Settings();
				settings->messageQueueSizeInBytes = 1024 * 1024;
				settings->maxNbMessagesPerUpdate = 100;
				settings->msSleepBetweenServerUpdates = 1;

				rmt_CreateGlobalInstance(&rmt_);
				rmt_SetCurrentThreadName("Main Thread");
			}

			~ScopedRemotery() { rmt_DestroyGlobalInstance(rmt_); }
			Remotery* rmt_ = nullptr;
		};
		ScopedRemotery remotery;

		Client::Window window;
		Plugin::Manager::Scoped pluginManager;
		GPU::Manager::Scoped gpuManager;
		Job::Manager::Scoped jobManager;
		Resource::Manager::Scoped resourceManager;

		GPU::SwapChainDesc scDesc;
		GPU::Handle scHandle;
		GPU::Handle fbsHandle;


		ScopedEngine(const char* name, const Core::CommandLine& cmdLine)
		    : window(name, 100, 100, 1280, 720, true, true)
		    , gpuManager(GetDefaultSetupParams(cmdLine))
		    , jobManager(GetNumJobWorkers(cmdLine), 256, 64 * 1024)
		{
			Graphics::Material::RegisterFactory();
			Graphics::Model::RegisterFactory();
			Graphics::Shader::RegisterFactory();
			Graphics::Texture::RegisterFactory();

			// Init device.
			i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
			DBG_ASSERT(numAdapters > 0);
			DBG_ASSERT(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);

			i32 w, h;
			window.GetSize(w, h);

			// Create swap chain.
			scDesc.width_ = w;
			scDesc.height_ = h;
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
			Graphics::Material::UnregisterFactory();
			Graphics::Model::UnregisterFactory();
			Graphics::Shader::UnregisterFactory();
			Graphics::Texture::UnregisterFactory();
		}
	};
}
