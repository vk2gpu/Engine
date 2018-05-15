#pragma once

#include "gpu/dll.h"
#include "gpu/types.h"

#include "core/type_conversion.h"

namespace GPU
{
	/**
	 * Format info.
	 */
	struct GPU_DLL FormatInfo
	{
		/// Block width.
		i32 blockW_ = 0;
		/// Block height.
		i32 blockH_ = 0;
		/// Number of bits in block.
		i32 blockBits_ = 0;
		/// Number of bits for red channel.
		i32 rBits_ = 0;
		/// Number of bits for green channel.
		i32 gBits_ = 0;
		/// Number of bits for blue channel.
		i32 bBits_ = 0;
		/// Number of bits for alpha channel.
		i32 aBits_ = 0;
		/// RGBA format.
		Core::DataType rgbaFormat_ = Core::DataType::INVALID;
		/// Number of bits for depth.
		i32 dBits_ = 0;
		/// Number of bits for stencil.
		i32 sBits_ = 0;
		/// Number of padding bits.
		i32 xBits_ = 0;
		/// Number of exponent bits.
		i32 eBits_ = 0;
		/// Number of channels.
		i32 channels_ = 0;
	};

	/**
	 * Get format info.
	 * @pre format != INVALID.
	 */
	GPU_DLL FormatInfo GetFormatInfo(Format format);

	/**
	 * Get texture footprint.
	 * @pre format != INVALID.
	 * @pre @a width >= 1.
	 * @pre @a height >= 1.
	 * @pre @a depth >= 1.
	 * @pre @a rowPitch is 0, or at least required size.
	 * @pre @a slicePitch is 0, or at least required size.
	 */
	GPU_DLL Footprint GetTextureFootprint(
	    Format format, i32 width, i32 height = 1, i32 depth = 1, i32 rowPitch = 0, i32 slicePitch = 0);

	/**
	 * Get texture size.
	 * @pre @a width >= 1.
	 * @pre @a height >= 1.
	 * @pre @a depth >= 1.
	 * @pre @a levels >= 1.
	 * @pre @a elements >= 1.
	 */
	GPU_DLL i64 GetTextureSize(Format format, i32 width, i32 height, i32 depth, i32 levels, i32 elements);

	/**
	 * Copy texture data.
	 */
	GPU_DLL void CopyTextureData(void* dstData, const Footprint& dstFootprint, const void* srcData,
	    const Footprint& srcFootprint, i32 rows, i32 slices);

	/*
	GPU_DLL void CopyTextureData(GPU::Format format, void* dstData, const TextureLayoutInfo& dstLayout, const GPU::Point dstPoint, 
		const void* srcData, const TextureLayoutInfo& srcLayout, const GPU::Box srcBox);
	*/

	/**
	 * Get view dimension from texture type.
	 */
	GPU_DLL ViewDimension GetViewDimension(TextureType type);

	/**
	 * Get valid DSV format from @a format.
	 */
	GPU_DLL Format GetDSVFormat(Format format);

	/**
	 * Get valid SRV format to read depth from @a format.
	 */
	GPU_DLL Format GetSRVFormatDepth(Format format);

	/**
	 * Get valid SRV format to read stencil from @a format.
	 */
	GPU_DLL Format GetSRVFormatStencil(Format format);

	/**
	 * Get stride for vertex stream.
	 */
	GPU_DLL i32 GetStride(const VertexElement* elements, i32 numElements, i32 streamIdx);

} // namespace GPU
