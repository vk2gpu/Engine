#pragma once
#include "core/debug.h"
#include "core/file.h"
#include "core/random.h"
#include "core/timer.h"
#include "core/vector.h"
#include "core/os.h"
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
		Plugin::Manager::Scoped pluginManager;
		GPU::Manager::Scoped gpuManager = GPU::Manager::Scoped(GetDefaultSetupParams());
		Job::Manager::Scoped jobManager = Job::Manager::Scoped(2, 256, 32 * 1024);
		Resource::Manager::Scoped resourceManager;

		ScopedEngine()
		{
			Graphics::Model::RegisterFactory();
			Graphics::Shader::RegisterFactory();
			Graphics::Texture::RegisterFactory();

			// Init device.
			i32 numAdapters = GPU::Manager::EnumerateAdapters(nullptr, 0);
			REQUIRE(numAdapters > 0);

			REQUIRE(GPU::Manager::CreateAdapter(0) == GPU::ErrorCode::OK);
		}

		~ScopedEngine()
		{
			Graphics::Model::UnregisterFactory();
			Graphics::Shader::UnregisterFactory();
			Graphics::Texture::UnregisterFactory();
		}
	};
}
