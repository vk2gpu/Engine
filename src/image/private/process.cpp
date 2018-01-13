#pragma once

#include "image/process.h"
#include "image/image.h"
#include "image/ispc/image_processing_ispc.h"

#include "core/misc.h"
#include "core/type_conversion.h"
#include "gpu/utils.h"
#include "math/utils.h"

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


	f32 CalculatePSNR(const Image& base, const Image& compare)
	{
		if(!base || !compare)
			return 0.0f;
		if(base.GetWidth() != compare.GetWidth())
			return 0.0f;
		if(base.GetHeight() != compare.GetHeight())
			return 0.0f;
		if(base.GetFormat() != compare.GetFormat())
			return 0.0f;

		f32 psnr = INFINITE_PSNR;
		if(base.GetFormat() == ImageFormat::R8G8B8A8_UNORM)
		{
			f32 mse = ispc::ImageProc_MSE_R8G8B8A8(base.GetWidth(), base.GetDepth(),
			    base.GetMipData<ispc::Color_R8G8B8A8>(0), compare.GetMipData<ispc::Color_R8G8B8A8>(0));
			if(mse > 0.0f)
				psnr = Math::AmplitudeRatioToDecibels(255.0f / std::sqrt(mse));
		}
		else if(base.GetFormat() == ImageFormat::R32G32B32A32_FLOAT)
		{
			f32 mse = ispc::ImageProc_MSE(
			    base.GetWidth(), base.GetDepth(), base.GetMipData<ispc::Color>(0), compare.GetMipData<ispc::Color>(0));
			if(mse > 0.0f)
				psnr = Math::AmplitudeRatioToDecibels(1.0f / std::sqrt(mse));
		}

		return psnr;
	}


} // namespace Image
