#include "image/image.h"

#include "core/misc.h"
#include "gpu/utils.h"

#include <utility>

namespace Image
{
	Image::Image(ImageType type, ImageFormat format, i32 width, i32 height, i32 depth, i32 levels, u8* data,
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
		if(!data_)
		{
			i64 bytesReq = GPU::GetTextureSize(format, width, height, depth, levels, 1);
			if(type_ == ImageType::TEXCUBE)
			{
				bytesReq *= 6;
			}

			data_ = new u8[bytesReq];
			memset(data_, 0, bytesReq);

			freeDataFn_ = [](u8* data) { delete[] data; };
		}
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

	void* Image::GetMipBaseAddr(i32 level) const
	{
		if(level >= levels_)
			return nullptr;

		auto formatInfo = GPU::GetFormatInfo(format_);

		i32 blockW = (width_ + formatInfo.blockW_ - 1) / formatInfo.blockW_;
		i32 blockH = (height_ + formatInfo.blockH_ - 1) / formatInfo.blockH_;

		u8* data = data_;
		for(i32 i = 0; i < levels_; ++i)
		{
			if(i == level)
				return data;

			const i64 numBlocks = blockW * blockH;
			data += (numBlocks * formatInfo.blockBits_) >> 3;

			blockW = Core::Max(blockW >> 1, 1);
			blockH = Core::Max(blockH >> 1, 1);
		}

		return nullptr;
	}


} // namespace Graphics
