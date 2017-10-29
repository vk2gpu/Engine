#include "gpu/utils.h"
#include "core/debug.h"
#include "core/misc.h"

namespace GPU
{
	FormatInfo GetFormatInfo(Format format)
	{
		FormatInfo info;

		// Setup format bits.
		switch(format)
		{
		case Format::R32G32B32A32_TYPELESS:
		case Format::R32G32B32A32_FLOAT:
		case Format::R32G32B32A32_UINT:
		case Format::R32G32B32A32_SINT:
			info.rBits_ = 32;
			info.gBits_ = 32;
			info.bBits_ = 32;
			info.aBits_ = 32;
			break;
		case Format::R32G32B32_TYPELESS:
		case Format::R32G32B32_FLOAT:
		case Format::R32G32B32_UINT:
		case Format::R32G32B32_SINT:
			info.rBits_ = 32;
			info.gBits_ = 32;
			info.bBits_ = 32;
			break;
		case Format::R16G16B16A16_TYPELESS:
		case Format::R16G16B16A16_FLOAT:
		case Format::R16G16B16A16_UNORM:
		case Format::R16G16B16A16_UINT:
		case Format::R16G16B16A16_SNORM:
		case Format::R16G16B16A16_SINT:
			info.rBits_ = 16;
			info.gBits_ = 16;
			info.bBits_ = 16;
			info.aBits_ = 16;
			break;
		case Format::R32G32_TYPELESS:
		case Format::R32G32_FLOAT:
		case Format::R32G32_UINT:
		case Format::R32G32_SINT:
			info.rBits_ = 32;
			info.gBits_ = 32;
			break;
		case Format::R32G8X24_TYPELESS:
			info.rBits_ = 32;
			info.gBits_ = 8;
			info.bBits_ = 0;
			info.aBits_ = 0;
			info.xBits_ = 24;
			break;
		case Format::D32_FLOAT_S8X24_UINT:
			info.dBits_ = 32;
			info.sBits_ = 8;
			info.xBits_ = 24;
			break;
		case Format::R32_FLOAT_X8X24_TYPELESS:
			info.rBits_ = 32;
			info.xBits_ = 32;
			break;
		case Format::X32_TYPELESS_G8X24_UINT:
			info.gBits_ = 32;
			info.xBits_ = 56;
			break;
		case Format::R10G10B10A2_TYPELESS:
		case Format::R10G10B10A2_UNORM:
		case Format::R10G10B10A2_UINT:
			info.rBits_ = 10;
			info.gBits_ = 10;
			info.bBits_ = 10;
			info.aBits_ = 2;
			break;
		case Format::R11G11B10_FLOAT:
			info.rBits_ = 11;
			info.gBits_ = 11;
			info.bBits_ = 10;
			break;
		case Format::R8G8B8A8_TYPELESS:
		case Format::R8G8B8A8_UNORM:
		case Format::R8G8B8A8_UNORM_SRGB:
		case Format::R8G8B8A8_UINT:
		case Format::R8G8B8A8_SNORM:
		case Format::R8G8B8A8_SINT:
			info.rBits_ = 8;
			info.gBits_ = 8;
			info.bBits_ = 8;
			info.aBits_ = 8;
			break;
		case Format::R16G16_TYPELESS:
		case Format::R16G16_FLOAT:
		case Format::R16G16_UNORM:
		case Format::R16G16_UINT:
		case Format::R16G16_SNORM:
		case Format::R16G16_SINT:
			info.rBits_ = 16;
			info.gBits_ = 16;
			break;
		case Format::D32_FLOAT:
			info.dBits_ = 32;
			break;
		case Format::R32_TYPELESS:
		case Format::R32_FLOAT:
		case Format::R32_UINT:
		case Format::R32_SINT:
			info.rBits_ = 32;
			break;
		case Format::R24G8_TYPELESS:
			info.rBits_ = 24;
			info.gBits_ = 32;
			break;
		case Format::D24_UNORM_S8_UINT:
			info.dBits_ = 24;
			info.sBits_ = 8;
			break;
		case Format::R24_UNORM_X8_TYPELESS:
			info.rBits_ = 24;
			info.xBits_ = 8;
			break;
		case Format::X24_TYPELESS_G8_UINT:
			info.xBits_ = 24;
			info.gBits_ = 8;
			break;
		case Format::R8G8_TYPELESS:
		case Format::R8G8_UNORM:
		case Format::R8G8_UINT:
		case Format::R8G8_SNORM:
		case Format::R8G8_SINT:
			info.rBits_ = 8;
			info.gBits_ = 8;
			break;
		case Format::D16_UNORM:
			info.dBits_ = 16;
			break;
		case Format::R16_TYPELESS:
		case Format::R16_FLOAT:
		case Format::R16_UNORM:
		case Format::R16_UINT:
		case Format::R16_SNORM:
		case Format::R16_SINT:
			info.rBits_ = 16;
			break;
		case Format::R8_TYPELESS:
		case Format::R8_UNORM:
		case Format::R8_UINT:
		case Format::R8_SNORM:
		case Format::R8_SINT:
			info.rBits_ = 8;
			break;
		case Format::A8_UNORM:
			info.aBits_ = 8;
			break;
		case Format::R1_UNORM:
			info.rBits_ = 1;
			break;
		case Format::R9G9B9E5_SHAREDEXP:
			info.rBits_ = 9;
			info.gBits_ = 9;
			info.bBits_ = 9;
			info.eBits_ = 5;
			break;
		case Format::R8G8_B8G8_UNORM:
		case Format::G8R8_G8B8_UNORM:
			info.rBits_ = 8;
			info.gBits_ = 16;
			info.bBits_ = 8;
			break;
		case Format::B5G6R5_UNORM:
			info.rBits_ = 5;
			info.gBits_ = 6;
			info.bBits_ = 5;
			break;
		case Format::B5G5R5A1_UNORM:
			info.rBits_ = 5;
			info.gBits_ = 6;
			info.bBits_ = 5;
			info.aBits_ = 1;
			break;
		case Format::R10G10B10_XR_BIAS_A2_UNORM:
			info.rBits_ = 10;
			info.gBits_ = 10;
			info.bBits_ = 10;
			info.xBits_ = 2;
			break;
		case Format::B8G8R8A8_TYPELESS:
		case Format::B8G8R8A8_UNORM_SRGB:
		case Format::B8G8R8A8_UNORM:
			info.rBits_ = 8;
			info.gBits_ = 8;
			info.bBits_ = 8;
			info.aBits_ = 8;
			break;
		case Format::B8G8R8X8_TYPELESS:
		case Format::B8G8R8X8_UNORM_SRGB:
		case Format::B8G8R8X8_UNORM:
			info.rBits_ = 8;
			info.gBits_ = 8;
			info.bBits_ = 8;
			info.xBits_ = 8;
			break;

		case Format::BC1_TYPELESS:
		case Format::BC1_UNORM:
		case Format::BC1_UNORM_SRGB:
		case Format::BC2_TYPELESS:
		case Format::BC2_UNORM:
		case Format::BC2_UNORM_SRGB:
		case Format::BC3_TYPELESS:
		case Format::BC3_UNORM:
		case Format::BC3_UNORM_SRGB:
		case Format::BC4_TYPELESS:
		case Format::BC4_UNORM:
		case Format::BC4_SNORM:
		case Format::BC5_TYPELESS:
		case Format::BC5_UNORM:
		case Format::BC5_SNORM:
		case Format::BC6H_TYPELESS:
		case Format::BC6H_UF16:
		case Format::BC6H_SF16:
		case Format::BC7_TYPELESS:
		case Format::BC7_UNORM:
		case Format::BC7_UNORM_SRGB:
		case Format::ETC1_UNORM:
		case Format::ETC2_UNORM:
		case Format::ETC2A_UNORM:
		case Format::ETC2A1_UNORM:
			// Ignore compressed formats.
			break;

		default:
			DBG_BREAK; // Format not defined.
			break;
		}

		// Setup RGBA format types where trivial.
		switch(format)
		{
		case Format::R32G32B32A32_TYPELESS:
		case Format::R32G32B32_TYPELESS:
		case Format::R16G16B16A16_TYPELESS:
		case Format::R32G32_TYPELESS:
		case Format::R10G10B10A2_TYPELESS:
		case Format::R8G8B8A8_TYPELESS:
		case Format::R16G16_TYPELESS:
		case Format::R32_TYPELESS:
		case Format::R24G8_TYPELESS:
		case Format::R16_TYPELESS:
		case Format::R8G8_TYPELESS:
		case Format::R8_TYPELESS:
			info.rgbaFormat_ = FormatType::TYPELESS;
			break;

		case Format::R32G32B32A32_FLOAT:
		case Format::R32G32B32_FLOAT:
		case Format::R16G16B16A16_FLOAT:
		case Format::R32G32_FLOAT:
		case Format::R11G11B10_FLOAT:
		case Format::R16G16_FLOAT:
		case Format::R32_FLOAT:
		case Format::R16_FLOAT:
			info.rgbaFormat_ = FormatType::FLOAT;
			break;

		case Format::R32G32B32A32_UINT:
		case Format::R32G32B32_UINT:
		case Format::R16G16B16A16_UINT:
		case Format::R32G32_UINT:
		case Format::R10G10B10A2_UINT:
		case Format::R8G8B8A8_UINT:
		case Format::R16G16_UINT:
		case Format::R32_UINT:
		case Format::R8G8_UINT:
		case Format::R16_UINT:
		case Format::R8_UINT:
			info.rgbaFormat_ = FormatType::UINT;
			break;

		case Format::R32G32B32A32_SINT:
		case Format::R32G32B32_SINT:
		case Format::R16G16B16A16_SINT:
		case Format::R32G32_SINT:
		case Format::R8G8B8A8_SINT:
		case Format::R16G16_SINT:
		case Format::R32_SINT:
		case Format::R8G8_SINT:
		case Format::R16_SINT:
		case Format::R8_SINT:
			info.rgbaFormat_ = FormatType::SINT;
			break;

		case Format::R16G16B16A16_UNORM:
		case Format::R10G10B10A2_UNORM:
		case Format::R8G8B8A8_UNORM:
		case Format::R8G8B8A8_UNORM_SRGB:
		case Format::R16G16_UNORM:
		case Format::R8G8_UNORM:
		case Format::R16_UNORM:
		case Format::R8_UNORM:
		case Format::A8_UNORM:
			info.rgbaFormat_ = FormatType::UNORM;
			break;

		case Format::R16G16B16A16_SNORM:
		case Format::R8G8B8A8_SNORM:
		case Format::R16G16_SNORM:
		case Format::R8G8_SNORM:
		case Format::R16_SNORM:
		case Format::R8_SNORM:
			info.rgbaFormat_ = FormatType::SNORM;
			break;

		default:
			break;
		}

		// Calculate block size based on total number of bits.
		info.blockBits_ += info.rBits_;
		info.blockBits_ += info.gBits_;
		info.blockBits_ += info.bBits_;
		info.blockBits_ += info.aBits_;
		info.blockBits_ += info.dBits_;
		info.blockBits_ += info.sBits_;
		info.blockBits_ += info.xBits_;
		info.blockBits_ += info.eBits_;

		// If no block size, must be a compressed format.
		if(info.blockBits_ == 0)
		{
			switch(format)
			{
			case Format::BC1_TYPELESS:
			case Format::BC1_UNORM:
			case Format::BC1_UNORM_SRGB:
			case Format::BC4_TYPELESS:
			case Format::BC4_UNORM:
			case Format::BC4_SNORM:
				info.blockBits_ = 64;
				info.blockW_ = 4;
				info.blockH_ = 4;
				break;
			case Format::BC2_TYPELESS:
			case Format::BC2_UNORM:
			case Format::BC2_UNORM_SRGB:
			case Format::BC3_TYPELESS:
			case Format::BC3_UNORM:
			case Format::BC3_UNORM_SRGB:
			case Format::BC5_TYPELESS:
			case Format::BC5_UNORM:
			case Format::BC5_SNORM:
			case Format::BC6H_TYPELESS:
			case Format::BC6H_UF16:
			case Format::BC6H_SF16:
			case Format::BC7_TYPELESS:
			case Format::BC7_UNORM:
			case Format::BC7_UNORM_SRGB:
				info.blockBits_ = 128;
				info.blockW_ = 4;
				info.blockH_ = 4;
				break;
			case Format::ETC1_UNORM:
				info.blockBits_ = 64;
				info.blockW_ = 4;
				info.blockH_ = 4;
				break;
			case Format::ETC2_UNORM:
				info.blockBits_ = 64;
				info.blockW_ = 4;
				info.blockH_ = 4;
				break;
			case Format::ETC2A_UNORM:
				info.blockBits_ = 128;
				info.blockW_ = 4;
				info.blockH_ = 4;
				break;
			case Format::ETC2A1_UNORM:
				info.blockBits_ = 64;
				info.blockW_ = 4;
				info.blockH_ = 4;
				break;
			}
		}
		else
		{
			info.blockW_ = 1;
			info.blockH_ = 1;
		}

		// Just handle this one separately.
		if(format == Format::R1_UNORM)
		{
			info.blockW_ = 8;
			info.blockBits_ = 8;
		}

		return info;
	}

	GPU_DLL TextureLayoutInfo GetTextureLayoutInfo(Format format, i32 width, i32 height)
	{
		TextureLayoutInfo texLayoutInfo;

		FormatInfo formatInfo = GetFormatInfo(format);
		i32 widthByBlock = Core::Max(1, width / formatInfo.blockW_);
		i32 heightByBlock = Core::Max(1, height / formatInfo.blockH_);
		texLayoutInfo.pitch_ = (widthByBlock * formatInfo.blockBits_) / 8;
		texLayoutInfo.slicePitch_ = (widthByBlock * heightByBlock * formatInfo.blockBits_) / 8;

		return texLayoutInfo;
	}

	GPU_DLL i64 GetTextureSize(Format format, i32 width, i32 height, i32 depth, i32 levels, i32 elements)
	{
		i64 size = 0;
		FormatInfo formatInfo = GetFormatInfo(format);
		for(i32 i = 0; i < levels; ++i)
		{
			i32 blocksW = Core::PotRoundUp(width, formatInfo.blockW_) / formatInfo.blockW_;
			i32 blocksH = Core::PotRoundUp(height, formatInfo.blockH_) / formatInfo.blockH_;
			i32 blocksD = depth;

			size += (formatInfo.blockBits_ * blocksW * blocksH * blocksD) / 8;
			width = Core::Max(width / 2, 1);
			height = Core::Max(height / 2, 1);
			depth = Core::Max(depth / 2, 1);
		}
		size *= elements;
		return size;
	}

	GPU_DLL ViewDimension GetViewDimension(TextureType type)
	{
		switch(type)
		{
		case TextureType::TEX1D:
			return ViewDimension::TEX1D;
		case TextureType::TEX2D:
			return ViewDimension::TEX2D;
		case TextureType::TEX3D:
			return ViewDimension::TEX3D;
		case TextureType::TEXCUBE:
			return ViewDimension::TEXCUBE;
		}
		return ViewDimension::INVALID;
	}

	GPU_DLL Format GetDSVFormat(Format format)
	{
		switch(format)
		{
		case Format::R16_TYPELESS:
			return Format::D16_UNORM;
		case Format::R24G8_TYPELESS:
			return Format::D24_UNORM_S8_UINT;
		case Format::R32_FLOAT:
		case Format::R32_UINT:
		case Format::R32_SINT:
		case Format::R32_TYPELESS:
			return Format::D32_FLOAT;
		case Format::R32G8X24_TYPELESS:
		case Format::X32_TYPELESS_G8X24_UINT:
		case Format::R32_FLOAT_X8X24_TYPELESS:
			return Format::D32_FLOAT_S8X24_UINT;
		}
		return Format::INVALID;
	}

	GPU_DLL Format GetSRVFormatDepth(Format format)
	{
		switch(format)
		{
		case Format::R16_TYPELESS:
		case Format::D16_UNORM:
			return Format::R16_UNORM;
		case Format::R24G8_TYPELESS:
		case Format::D24_UNORM_S8_UINT:
			return Format::R24_UNORM_X8_TYPELESS;
		case Format::R32_TYPELESS:
		case Format::D32_FLOAT:
			return Format::R32_FLOAT;
		case Format::R32G8X24_TYPELESS:
		case Format::D32_FLOAT_S8X24_UINT:
			return Format::R32_FLOAT_X8X24_TYPELESS;
		}
		return GPU::Format::INVALID;
	}

	GPU_DLL Format GetSRVFormatStencil(Format format)
	{
		switch(format)
		{
		case Format::R24G8_TYPELESS:
		case Format::D24_UNORM_S8_UINT:
			return Format::X24_TYPELESS_G8_UINT;
		case Format::R32G8X24_TYPELESS:
		case Format::D32_FLOAT_S8X24_UINT:
			return Format::X32_TYPELESS_G8X24_UINT;
		}
		return GPU::Format::INVALID;
	}

	GPU_DLL i32 GetStride(const VertexElement* elements, i32 numElements, i32 streamIdx)
	{
		u32 stride = 0;
		for(i32 elementIdx = 0; elementIdx < numElements; ++elementIdx)
		{
			const auto& element(elements[elementIdx]);
			if(element.streamIdx_ == streamIdx)
			{
				u32 size = GPU::GetFormatInfo(element.format_).blockBits_ / 8;
				stride = Core::Max(stride, element.offset_ + size);
			}
		}
		return stride;
	}

} // namespace GPU
