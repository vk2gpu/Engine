#include "graphics/render_resources.h"

namespace Graphics
{
	RenderGraphTextureDesc::RenderGraphTextureDesc(
	    GPU::TextureType type, GPU::Format format, i32 width, i32 height, i16 depth, i16 levels, i16 elements)
	{
		type_ = type;
		format_ = format;
		width_ = width;
		height_ = height;
		depth_ = depth;
		levels_ = levels;
		elements_ = elements;
	}

	RenderGraphBufferDesc::RenderGraphBufferDesc(i32 size) { size_ = size; }
};
