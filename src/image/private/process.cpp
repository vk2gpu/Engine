#pragma once

#include "image/process.h"
#include "image/image.h"
#include "image/ispc/image_processing_ispc.h"
#include "image/ispc/ispc_texcomp_kernel_ispc.h"

#include "core/misc.h"
#include "core/type_conversion.h"
#include "core/vector.h"
#include "gpu/utils.h"
#include "math/utils.h"

#include <squish.h>

#include <cmath>
#include <utility>

namespace Image
{
	namespace
	{
		bool SetupOutput(Image& output, const Image& input, ImageFormat outputFormat)
		{
			if(!output)
			{
				Image newOutput(input.GetType(), outputFormat, input.GetWidth(), input.GetHeight(), input.GetDepth(),
				    input.GetLevels(), nullptr, nullptr);
				std::swap(output, newOutput);
			}

			return output.GetWidth() == input.GetWidth() && output.GetHeight() == input.GetHeight() &&
			       output.GetDepth() == input.GetDepth() && output.GetFormat() == outputFormat;
		}
	}


	bool Convert(Image& output, const Image& input, ImageFormat outFormat)
	{
		if(!SetupOutput(output, input, outFormat))
			return false;

		// Fast paths for conversion.
		if(input.GetFormat() == ImageFormat::R8G8B8A8_UNORM && output.GetFormat() == ImageFormat::R32G32B32A32_FLOAT)
		{
			const i32 numLevels = Core::Min(input.GetLevels(), output.GetLevels());
			i32 levelW = input.GetWidth();
			i32 levelH = input.GetHeight();
			for(i32 level = 0; level < numLevels; ++level)
			{
				const i32 numTexels = levelW * levelH;

				ispc::ImageProc_Unpack_R8G8B8A8(
				    numTexels, input.GetMipData<ispc::Color_R8G8B8A8>(level), output.GetMipData<ispc::Color>(level));

				levelW = Core::Max(levelW >> 1, 1);
				levelH = Core::Max(levelH >> 1, 1);
			}

			return true;
		}
		else if(input.GetFormat() == ImageFormat::R32G32B32A32_FLOAT &&
		        output.GetFormat() == ImageFormat::R8G8B8A8_UNORM)
		{
			const i32 numLevels = Core::Min(input.GetLevels(), output.GetLevels());
			i32 levelW = input.GetWidth();
			i32 levelH = input.GetHeight();
			for(i32 level = 0; level < numLevels; ++level)
			{
				const i32 numTexels = levelW * levelH;
				ispc::ImageProc_Pack_R8G8B8A8(
				    numTexels, input.GetMipData<ispc::Color>(level), output.GetMipData<ispc::Color_R8G8B8A8>(level));
				levelW = Core::Max(levelW >> 1, 1);
				levelH = Core::Max(levelH >> 1, 1);
			}

			return true;
		}

		auto PadData = [](
		    Core::Vector<u32>& paddedData, const u32* levelData, i32 levelW, i32 levelH, i32 paddedW, i32 paddedH) {
			// Always going largest to smallest, so only need to allocate once and reuse the memory
			// for subsequent levels.
			if(paddedData.empty())
				paddedData.resize(paddedW * paddedH);

			// Copy rows.
			const u32* srcData = levelData;
			u32* dstData = paddedData.data();
			for(i32 y = 0; y < levelH; ++y)
			{
				DBG_ASSERT(paddedData.belongs(dstData, levelW));
				memcpy(dstData, srcData, levelW * sizeof(u32));
				dstData += paddedW;
			}

			// Pad out.
			srcData = paddedData.data() + (levelW - 1);
			dstData = paddedData.data() + levelW;
			for(i32 y = 0; y < paddedH; ++y)
			{
				const u32 src = *srcData;
				for(i32 x = levelW; x < paddedW; ++x)
				{
					DBG_ASSERT(paddedData.belongs(dstData, 1));
					*dstData++ = src;
				}
				srcData += paddedW;
				dstData += levelW;
			}

			srcData = paddedData.data() + ((levelH - 1) * paddedW);
			dstData = paddedData.data() + (levelH * paddedW);
			for(i32 y = levelH; y < paddedH; ++y)
			{
				DBG_ASSERT(paddedData.belongs(dstData, levelW));
				memcpy(dstData, srcData, levelW * sizeof(u32));

				srcData += levelW;
				dstData += paddedW;
			}
		};

		// Block compressed support.
		if(input.GetFormat() == ImageFormat::R8G8B8A8_UNORM)
		{
			using CompressFn = void(ispc::rgba_surface*, uint8_t*);
			CompressFn* compressFn = nullptr;

			switch(output.GetFormat())
			{
			case ImageFormat::BC1_UNORM:
			case ImageFormat::BC1_UNORM_SRGB:
				compressFn = ispc::CompressBlocksBC1_ispc;
				break;
			case ImageFormat::BC3_UNORM:
			case ImageFormat::BC3_UNORM_SRGB:
				compressFn = ispc::CompressBlocksBC3_ispc;
				break;
			case ImageFormat::BC4_UNORM:
				//case ImageFormat::BC4_SNORM:
				compressFn = ispc::CompressBlocksBC4_ispc;
				break;
			case ImageFormat::BC5_UNORM:
				//case ImageFormat::BC5_SNORM:
				compressFn = ispc::CompressBlocksBC5_ispc;
				break;
			}

			if(compressFn)
			{
				const i32 numLevels = Core::Min(input.GetLevels(), output.GetLevels());
				i32 levelW = input.GetWidth();
				i32 levelH = input.GetHeight();

				Core::Vector<u32> tempPadding;

				for(i32 level = 0; level < output.GetLevels(); ++level)
				{
					const i32 paddedW = Core::PotRoundUp(levelW, 4);
					const i32 paddedH = Core::PotRoundUp(levelH, 4);

					const u32* levelData = input.GetMipData<u32>(level);

					// If the level data is not the correct size then we'll need to pad it out.
					if((levelW & 3) != 0 || (levelH & 3) != 0)
					{
						PadData(tempPadding, levelData, levelW, levelH, paddedW, paddedH);
						levelData = tempPadding.data();
					}

					ispc::rgba_surface surface;
					surface.ptr = (u8*)(levelData);
					surface.width = paddedW;
					surface.height = paddedH;
					surface.stride = paddedW * sizeof(u32);
					compressFn(&surface, output.GetMipData<u8>(level));

					levelW = Core::Max(levelW >> 1, 1);
					levelH = Core::Max(levelH >> 1, 1);
				}

				return true;
			}
		}

		// Compressed input, R8G8B8A8_UNORM output.
		if(output.GetFormat() == ImageFormat::R8G8B8A8_UNORM)
		{
			u32 squishFlags = 0;
			switch(input.GetFormat())
			{
			case ImageFormat::BC1_UNORM:
			case ImageFormat::BC1_UNORM_SRGB:
				squishFlags = squish::kBc1;
				break;
			case ImageFormat::BC3_UNORM:
			case ImageFormat::BC3_UNORM_SRGB:
				squishFlags = squish::kBc3;
				break;
			case ImageFormat::BC4_UNORM:
				//case ImageFormat::BC4_SNORM:
				squishFlags = squish::kBc4;
				break;
			case ImageFormat::BC5_UNORM:
				//case ImageFormat::BC5_SNORM:
				squishFlags = squish::kBc5;
				break;
			}

			if(squishFlags)
			{
				i32 levelW = output.GetWidth();
				i32 levelH = output.GetHeight();
				for(i32 level = 0; level < output.GetLevels(); ++level)
				{
					squish::DecompressImage(
					    output.GetMipData<u8>(level), levelW, levelH, input.GetMipData<void>(level), squishFlags);

					levelW = Core::Max(levelW >> 1, 1);
					levelH = Core::Max(levelH >> 1, 1);
				}
				return true;
			}
		}

		// Fallback to generic conversion.
		auto inFormatInfo = GPU::GetFormatInfo(input.GetFormat());
		auto outFormatInfo = GPU::GetFormatInfo(output.GetFormat());

		Core::StreamDesc inStreamDesc;
		inStreamDesc.dataType_ = inFormatInfo.rgbaFormat_;
		inStreamDesc.numBits_ = inFormatInfo.rBits_;
		inStreamDesc.stride_ = inFormatInfo.blockBits_ >> 3;

		Core::StreamDesc outStreamDesc;
		outStreamDesc.dataType_ = outFormatInfo.rgbaFormat_;
		outStreamDesc.numBits_ = outFormatInfo.rBits_;
		outStreamDesc.stride_ = outFormatInfo.blockBits_ >> 3;

		if(inFormatInfo.blockW_ != 1 || inFormatInfo.blockH_ != 1 || outFormatInfo.blockW_ != 1 ||
		    outFormatInfo.blockH_ != 1)
			return false;

		i32 inChannels = 0;
		inChannels += inFormatInfo.rBits_ > 0 ? 1 : 0;
		inChannels += inFormatInfo.gBits_ > 0 ? 1 : 0;
		inChannels += inFormatInfo.bBits_ > 0 ? 1 : 0;
		inChannels += inFormatInfo.aBits_ > 0 ? 1 : 0;

		i32 outChannels = 0;
		outChannels += outFormatInfo.rBits_ > 0 ? 1 : 0;
		outChannels += outFormatInfo.gBits_ > 0 ? 1 : 0;
		outChannels += outFormatInfo.bBits_ > 0 ? 1 : 0;
		outChannels += outFormatInfo.aBits_ > 0 ? 1 : 0;

		const i32 numLevels = Core::Min(input.GetLevels(), output.GetLevels());
		i32 levelW = input.GetWidth();
		i32 levelH = input.GetHeight();
		for(i32 level = 0; level < numLevels; ++level)
		{
			const i32 numTexels = levelW * levelH;

			inStreamDesc.data_ = input.GetMipBaseAddr(level);
			outStreamDesc.data_ = output.GetMipBaseAddr(level);

			if(!Core::Convert(outStreamDesc, inStreamDesc, numTexels, Core::Min(inChannels, outChannels)))
				return false;

			levelW = Core::Max(levelW >> 1, 1);
			levelH = Core::Max(levelH >> 1, 1);
		}

		return true;
	}


	bool GammaToLinear(Image& output, const Image& input)
	{
		if(!SetupOutput(output, input, ImageFormat::R32G32B32A32_FLOAT))
			return false;

		i32 numLevels = Core::Min(input.GetLevels(), output.GetLevels());
		i32 levelW = input.GetWidth();
		i32 levelH = input.GetHeight();
		for(i32 level = 0; level < numLevels; ++level)
		{
			ispc::ImageProc_GammaToLinear(
			    levelW, levelH, input.GetMipData<ispc::Color>(level), output.GetMipData<ispc::Color>(level));
			levelW = Core::Max(levelW / 2, 1);
			levelH = Core::Max(levelH / 2, 1);
		}
		return true;
	}


	bool LinearToGamma(Image& output, const Image& input)
	{
		if(!SetupOutput(output, input, ImageFormat::R32G32B32A32_FLOAT))
			return false;

		i32 numLevels = Core::Min(input.GetLevels(), output.GetLevels());
		i32 levelW = input.GetWidth();
		i32 levelH = input.GetHeight();
		for(i32 level = 0; level < numLevels; ++level)
		{
			ispc::ImageProc_LinearToGamma(
			    levelW, levelH, input.GetMipData<ispc::Color>(level), output.GetMipData<ispc::Color>(level));
			levelW = Core::Max(levelW >> 1, 1);
			levelH = Core::Max(levelH >> 1, 1);
		}
		return true;
	}


	bool GenerateMips(Image& output, const Image& input)
	{
		if(!SetupOutput(output, input, ImageFormat::R32G32B32A32_FLOAT))
			return false;

		i32 levelW = input.GetWidth();
		i32 levelH = input.GetHeight();

		if(output.GetLevels() > 1)
		{
			// Generate first mip.
			ispc::ImageProc_Downsample2x(
			    levelW, levelH, input.GetMipData<ispc::Color>(0), output.GetMipData<ispc::Color>(1));

			levelW = Core::Max(levelW >> 1, 1);
			levelH = Core::Max(levelH >> 1, 1);

			// Now the rest in the chain.
			for(i32 level = 1; level < output.GetLevels() - 1; ++level)
			{
				ispc::ImageProc_Downsample2x(
				    levelW, levelH, output.GetMipData<ispc::Color>(level), output.GetMipData<ispc::Color>(level + 1));
				levelW = Core::Max(levelW >> 1, 1);
				levelH = Core::Max(levelH >> 1, 1);
			}
		}

		return true;
	}


	RGBAColor CalculatePSNR(const Image& base, const Image& compare)
	{
		RGBAColor psnr = INFINITE_PSNR_RGBA;

		if(!base || !compare)
			return RGBAColor(0.0f, 0.0f, 0.0f, 0.0f);
		if(base.GetWidth() != compare.GetWidth())
			return RGBAColor(0.0f, 0.0f, 0.0f, 0.0f);
		if(base.GetHeight() != compare.GetHeight())
			return RGBAColor(0.0f, 0.0f, 0.0f, 0.0f);
		if(base.GetFormat() != compare.GetFormat())
			return RGBAColor(0.0f, 0.0f, 0.0f, 0.0f);

		if(base.GetFormat() == ImageFormat::R8G8B8A8_UNORM)
		{
			ispc::Error mse;
			ispc::ImageProc_MSE_R8G8B8A8(&mse, base.GetWidth(), base.GetHeight(),
			    base.GetMipData<ispc::Color_R8G8B8A8>(0), compare.GetMipData<ispc::Color_R8G8B8A8>(0));

			if(mse.r > 0.0f)
				psnr.r = Math::AmplitudeRatioToDecibels((f32)((255.0) / std::sqrt(mse.r)));
			if(mse.g > 0.0f)
				psnr.g = Math::AmplitudeRatioToDecibels((f32)((255.0) / std::sqrt(mse.g)));
			if(mse.b > 0.0f)
				psnr.b = Math::AmplitudeRatioToDecibels((f32)((255.0) / std::sqrt(mse.b)));
			if(mse.a > 0.0f)
				psnr.a = Math::AmplitudeRatioToDecibels((f32)((255.0) / std::sqrt(mse.a)));
		}
		else if(base.GetFormat() == ImageFormat::R32G32B32A32_FLOAT)
		{
			ispc::Error mse;
			ispc::ImageProc_MSE(&mse, base.GetWidth(), base.GetHeight(), base.GetMipData<ispc::Color>(0),
			    compare.GetMipData<ispc::Color>(0));

			if(mse.r > 0.0f)
				psnr.r = Math::AmplitudeRatioToDecibels((f32)(1.0 / std::sqrt(mse.r)));
			if(mse.g > 0.0f)
				psnr.g = Math::AmplitudeRatioToDecibels((f32)(1.0 / std::sqrt(mse.g)));
			if(mse.b > 0.0f)
				psnr.b = Math::AmplitudeRatioToDecibels((f32)(1.0 / std::sqrt(mse.b)));
			if(mse.a > 0.0f)
				psnr.a = Math::AmplitudeRatioToDecibels((f32)(1.0 / std::sqrt(mse.a)));
		}

		return psnr;
	}


} // namespace Image
