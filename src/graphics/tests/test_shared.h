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

	class ScopedFactory
	{
	public:
		ScopedFactory()
		{
			Graphics::Shader::RegisterFactory();
			Graphics::Texture::RegisterFactory();
		}

		~ScopedFactory()
		{
			Graphics::Shader::UnregisterFactory();
			Graphics::Texture::UnregisterFactory();
		}
	};
}
