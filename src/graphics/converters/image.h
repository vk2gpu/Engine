#pragma once

#include "core/types.h"
#include "gpu/types.h"

namespace Graphics
{
	class Image
	{
	public:
		typedef void (*FreeDataFn)(u8*);

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
		Image(GPU::TextureType type, GPU::Format format, i32 width, i32 height, i32 depth, i32 levels, u8* data,
		    FreeDataFn freeDataFn);

		/// Move
		Image(Image&& other);
		Image& operator=(Image&& other);

		/// No copy.
		Image(const Image&) = delete;
		Image& operator=(const Image&) = delete;

		~Image();
		void swap(Image& other);

		operator bool() const { return data_ != nullptr; }

		template<typename TYPE>
		TYPE* GetData() { return reinterpret_cast<TYPE*>(data_); }

		template<typename TYPE>
		const TYPE* GetData() const { return reinterpret_cast<TYPE*>(data_); }

		GPU::TextureType type_ = GPU::TextureType::TEX2D;
		GPU::Format format_ = GPU::Format::INVALID;
		i32 width_ = 0;
		i32 height_ = 0;
		i32 depth_ = 0;
		i32 levels_ = 0;
		u8* data_ = nullptr;

		/// Function to call to free image data (may be allocated by external sources)
		FreeDataFn freeDataFn_ = nullptr;
	};
} // namespace Graphics
