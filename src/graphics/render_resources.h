#pragma once

#include "graphics/dll.h"
#include "core/debug.h"
#include "gpu/resources.h"

namespace Graphics
{
	struct RenderGraphTextureDesc : public GPU::TextureDesc
	{
	};

	struct RenderGraphBufferDesc : public GPU::BufferDesc
	{
	};

	struct RenderGraphResource
	{
		RenderGraphResource() = default;
		RenderGraphResource(i32 idx, i32 version)
		    : idx_((i16)idx)
		    , version_((i16)version)
		{
			DBG_ASSERT(idx >= 0 && idx < 0x8000);
			DBG_ASSERT(version >= 0 && version < 0x8000);
		}

		RenderGraphResource(i32 id)
		{
			idx_ = (id >> 16) & 0x7fff;
			version_ = (id)&0x7fff;
		}

		bool operator==(const RenderGraphResource& other) const
		{
			return idx_ == other.idx_ && version_ == other.version_;
		}

		i32 GetID() const { return (i32)idx_ << 16 | (i32)version_; }

		i16 idx_ = -1;
		i16 version_ = -1;
	};

} // namespace Graphics
