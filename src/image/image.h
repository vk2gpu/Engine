#pragma once

#include "image/dll.h"
#include "image/types.h"

namespace Image
{
	class IMAGE_DLL Image
	{
	public:
		using FreeDataFn = void (*)(u8*);

		Image() = default;

		/**
		 * Construct image.
		 * @param type Type of image (1D, 2D, etc...)
		 * @param format
		 * @param width
		 * @param height
		 * @param depth
		 * @param levels Number of mip levels.
		 * @param data Pointer to data.
		 * @param freeDataFn Function to call to free data, can be nullptr.
		 */
		Image(ImageType type, ImageFormat format, i32 width, i32 height, i32 depth, i32 levels, u8* data,
		    FreeDataFn freeDataFn);

		Image(Image&& other);
		Image& operator=(Image&& other);
		~Image();

		/**
		 * @return true if a valid image.
		 */
		explicit operator bool() const { return data_ != nullptr; }

		template<typename TYPE>
		TYPE* GetMipData(i32 level)
		{
			return reinterpret_cast<TYPE*>(GetMipBaseAddr(level));
		}

		template<typename TYPE>
		const TYPE* GetMipData(i32 level) const
		{
			return reinterpret_cast<TYPE*>(GetMipBaseAddr(level));
		}

		void* GetMipBaseAddr(i32 level) const;

		ImageType GetType() const { return type_; }
		ImageFormat GetFormat() const { return format_; }
		i32 GetWidth() const { return width_; }
		i32 GetHeight() const { return height_; }
		i32 GetDepth() const { return depth_; }
		i32 GetLevels() const { return levels_; }


		/// Function to call to free image data (may be allocated by external sources)
		FreeDataFn freeDataFn_ = nullptr;

	private:
		/// No copy.
		Image(const Image&) = delete;
		Image& operator=(const Image&) = delete;

		void swap(Image& other);

		ImageType type_ = ImageType::TEX2D;
		ImageFormat format_ = ImageFormat::INVALID;
		i32 width_ = 0;
		i32 height_ = 0;
		i32 depth_ = 0;
		i32 levels_ = 0;
		u8* data_ = nullptr;
	};
} // namespace Image
