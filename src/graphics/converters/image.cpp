#include "graphics/converters/image.h"

#include <utility>

namespace Graphics
{
	Image::Image(GPU::TextureType type, GPU::Format format, i32 width, i32 height, i32 depth, i32 levels, u8* data,
	    FreeDataFn freeDataFn)
	    : type_(type)
	    , format_(format)
	    , width_(width)
	    , height_(height)
	    , depth_(depth)
	    , levels_(levels)
	    , data_(data)
	    , freeDataFn_(freeDataFn)

	{
	}

	Image::Image(Image&& other)
	{ //
		swap(other);
	}

	Image& Image::operator=(Image&& other)
	{
		swap(other);
		return *this;
	}

	Image::~Image()
	{
		if(freeDataFn_)
		{
			freeDataFn_(data_);
			data_ = nullptr;
		}
	}

	void Image::swap(Image& other)
	{
		std::swap(type_, other.type_);
		std::swap(width_, other.width_);
		std::swap(height_, other.height_);
		std::swap(depth_, other.depth_);
		std::swap(levels_, other.levels_);
		std::swap(format_, other.format_);
		std::swap(data_, other.data_);
		std::swap(freeDataFn_, other.freeDataFn_);
	}

} // namespace Graphics
