#pragma once

#include "image/types.h"

namespace Image
{
	class Image;

	static const f32 INFINITE_PSNR = 9999.0f;


	/**
	 * Convert @a input image to @a outFormat.
	 * @return success.
	 */
	IMAGE_DLL bool Convert(Image& output, const Image& input, ImageFormat outFormat);

	/**
	 * Gamma to Linear conversion.
	 * @param output Output image.
	 * @param input Input image.
	 * @pre @a input format is R32G32B32A32_FLOAT.
	 * @return success.
	 */
	IMAGE_DLL bool GammaToLinear(Image& output, const Image& input);

	/**
	 * Linear to Gamma conversion.
	 * @param output Output image.
	 * @param input Input image.
	 * @pre @a input format is R32G32B32A32_FLOAT.
	 * @return success.
	 */
	IMAGE_DLL bool LinearToGamma(Image& output, const Image& input);

	/**
	 * Generate mips.
	 * @param output Output image.
	 * @param input Input image.
	 * @pre @a input format is R32G32B32A32_FLOAT.
	 * @return success.
	 */
	IMAGE_DLL bool GenerateMips(Image& output, const Image& input);

	/**
	 * Calculate PSNR
	 * @param base Base image.
	 * @param compare Image to compare.
	 * @return PSNR in dB
	 */
	IMAGE_DLL f32 CalculatePSNR(const Image& base, const Image& compare);

} // namespace Image
