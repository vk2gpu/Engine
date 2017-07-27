#pragma once
#include "gpu/resources.h"

namespace Graphics
{
	struct TextureImpl
	{
		GPU::Handle handle_;
		GPU::TextureDesc desc_;

		~TextureImpl();
	};

} // namespace Graphics
