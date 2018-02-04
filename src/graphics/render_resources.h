#pragma once

#include "graphics/dll.h"
#include "core/debug.h"
#include "gpu/resources.h"

namespace Graphics
{
	struct GRAPHICS_DLL RenderGraphTextureDesc : public GPU::TextureDesc
	{
		RenderGraphTextureDesc() = default;
		RenderGraphTextureDesc(GPU::TextureType type, GPU::Format format, i32 width, i32 height = 1, i16 depth = 1,
		    i16 levels = 1, i16 elements = 1);
	};

	struct GRAPHICS_DLL RenderGraphBufferDesc : public GPU::BufferDesc
	{
		RenderGraphBufferDesc() = default;
		RenderGraphBufferDesc(i32 size);
	};

	struct GRAPHICS_DLL RenderGraphResource
	{
		RenderGraphResource() = default;
		RenderGraphResource(i32 idx, i32 version)
		    : idx_((i16)idx)
		    , version_((i16)version)
		{
			DBG_ASSERT(idx >= 0 && idx < 0x8000);
			DBG_ASSERT(version >= 0 && version < 0x8000);
		}

		bool operator==(const RenderGraphResource& other) const
		{
			return idx_ == other.idx_ && version_ == other.version_;
		}

		explicit operator bool() const { return idx_ != -1 && version_ != -1; }

		i16 idx_ = -1;
		i16 version_ = -1;
	};

} // namespace Graphics
