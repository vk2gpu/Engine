#include "image/codec.h"
#include "image/image.h"
#include "core/misc.h"
#include "gpu/utils.h"

namespace Image
{
	Codec::Codec(Image& image, i32 level, i32 slice)
	    : image_(&image)
	{
		DBG_ASSERT(slice == 0);
		baseAddress_ = image_->GetMipData<u8>(level);
		width_ = Core::Max(1, image_->GetWidth() >> level);
		height_ = Core::Max(1, image_->GetHeight() >> level);
		format_ = image_->GetFormat();

		const auto formatInfo = GPU::GetFormatInfo(format_);
		DBG_ASSERT(formatInfo.blockW_ == 1);
		DBG_ASSERT(formatInfo.blockH_ == 1);
		texelSize_ = formatInfo.blockBits_ / 8;
	}

	Codec::~Codec() {}

	void Codec::GetTexel(f32* out, i32 x, i32 y, i32 c) const
	{

		const i32 index = texelSize_ * x + (y * width_);
		for(i32 i = 0; i < c; + i)
		{
		}
	}

	void Codec::SetTexel(i32 x, i32 y, i32 c, const f32* in) {}
} // namespace Image
