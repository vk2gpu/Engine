#include "graphics/shader.h"
#include "graphics/private/shader_impl.h"

#include "core/debug.h"
#include "gpu/manager.h"

namespace Graphics
{
	Shader::Shader()
	{ //
	}

	Shader::~Shader()
	{ //
		DBG_ASSERT(impl_);

		if(GPU::Manager::IsInitialized())
		{
			//GPU::Manager::DestroyResource(impl_->handle_);
		}
		delete impl_;
	}

} // namespace Graphics
