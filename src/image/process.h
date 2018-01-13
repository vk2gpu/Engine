#pragma once

#include "image/types.h"

namespace Image
{
	class Image;

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

} // namespace Image
