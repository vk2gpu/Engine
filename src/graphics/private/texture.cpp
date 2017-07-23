#include "graphics/texture.h"
#include "graphics/private/texture_impl.h"

#include "core/debug.h"
#include "gpu/manager.h"

namespace Graphics
{
	Texture::Texture()
	{ //
	}

	Texture::~Texture()
	{ //
		DBG_ASSERT(impl_);

		if(GPU::Manager::IsInitialized())
		{
			GPU::Manager::DestroyResource(impl_->handle_);
		}
		delete impl_;
	}

	const GPU::TextureDesc& Texture::GetDesc() const
	{
		DBG_ASSERT(impl_);
		return impl_->desc_;
	}

	GPU::Handle Texture::GetHandle() const
	{
		DBG_ASSERT(impl_);
		return impl_->handle_;
	}

} // namespace Graphics
