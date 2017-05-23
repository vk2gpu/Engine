#pragma once

#include "graphics/dll.h"
#include "core/types.h"
#include "gpu/fwd_decls.h"

namespace Graphics
{
	namespace TextureFileData
	{
		struct Header
		{
			GPU::TextureType type_;
			GPU::Format format_;
			i32 width_;
			i32 height_;
			i16 depth_;
			i16 levels_;
			i16 elements_;
		};
	};
} // namespace Graphics
