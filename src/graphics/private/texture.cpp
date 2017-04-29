#include "graphics/texture.h"
#include "graphics/private/texture_impl.h"

#include "core/debug.h"

namespace Graphics
{
	Texture::Texture()
	{ //
	}

	Texture::~Texture()
	{ //
		DBG_ASSERT(impl_);
		delete impl_;
	}

} // namespace Graphics