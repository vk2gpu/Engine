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

#include "graphics/factory.h"
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
}


namespace
{
	class ScopedFactory
	{
	public:
		ScopedFactory()
		{
			factory_ = new Graphics::Factory();
			Resource::Manager::RegisterFactory<Graphics::Shader>(factory_);
			Resource::Manager::RegisterFactory<Graphics::Texture>(factory_);
		}

		~ScopedFactory()
		{
			Resource::Manager::UnregisterFactory(factory_);
			delete factory_;
		}

	private:
		Resource::IFactory* factory_ = nullptr;
	};
}
