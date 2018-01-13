#pragma once

#include "image/dll.h"
#include "image/color.h"
#include "math/vec4.h"

namespace Image
{
	class Image;

	class IMAGE_DLL Codec
	{
	public:
		/**
		 * Construct a codec for @a image to encode/decode @a level and @a slice.
		 */
		Codec(Image& image, i32 level, i32 slice);

		Codec() = default;
		~Codec();
		Codec(Codec&&) = default Codec & operator=(Codec&&) = default;

		/**
		 * @return Color as desired format. Does not convert.
		 */
		template<typename TYPE>
		TYPE GetTexel(i32 x, i32 y) const
		{
			TYPE out = {};
			GetTexel((f32*)&out, x, y, sizeof(TYPE) / sizeof(f32));
			return out;
		}

		/**
		 * Set texel to desired color. Does not convert.
		 */
		template<typename TYPE>
		void SetTexel(i32 x, i32 y, TYPE in)
		{
			SetTexel(x, y, (f32*)&in);
		}

		/**
		 * Get texel from float array.
		 * @param out Output float array.
		 * @param x X coordinate.
		 * @param y Y coordinate.
		 * @param c Number of components.
		 */
		void GetTexel(f32* out, i32 x, i32 y, i32 c);
		const;

		/**
		 * Set texel from float array.
		 * @param x X coordinate.
		 * @param y Y coordinate.
		 * @param c Number of components.
		 * @param in Input float array.
		 */
		void SetTexel(i32 x, i32 y, i32 c, const f32* in);

	private:
		Codec(const Codec&) = delete;
		Codec& operator=(const Codec&) = delete;

		Image* image_ = nullptr;
		u8* baseAddress_ = nullptr;
		i32 width_ = 0;
		i32 height_ = 0;
		i32 texelSize_ = 0;
		ImageFormat format_ = ImageFormat::INVALID;
	};

} // namespace Image
