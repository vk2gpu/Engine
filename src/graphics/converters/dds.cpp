#include "graphics/converters/dds.h"
#include "graphics/converters/image.h"
#include "core/file.h"
#include "core/misc.h"
#include "gpu/types.h"
#include "gpu/utils.h"
#include "resource/converter.h"

#include <cstdio>

namespace Graphics
{
	namespace DDS
	{
#define MAKEFOURCC(A, B, C, D) ((u32(D) << 24) | (u32(C) << 16) | (u32(B) << 8) | (u32(A)))

		struct DDS_PIXELFORMAT
		{
			u32 dwSize;
			u32 dwFlags;
			u32 dwFourCC;
			u32 dwRGBBitCount;
			u32 dwRBitMask;
			u32 dwGBitMask;
			u32 dwBBitMask;
			u32 dwABitMask;
		};

		struct DDS_HEADER
		{
			u32 dwSize;
			u32 dwFlags;
			u32 dwHeight;
			u32 dwWidth;
			u32 dwPitchOrLinearSize;
			u32 dwDepth;
			u32 dwMipMapCount;
			u32 dwReserved1[11];
			DDS_PIXELFORMAT ddspf;
			u32 dwCaps;
			u32 dwCaps2;
			u32 dwCaps3;
			u32 dwCaps4;
			u32 dwReserved2;
		};

		enum DDSD_FLAGS
		{
			DDSD_CAPS = 0x1,
			DDSD_HEIGHT = 0x2,
			DDSD_WIDTH = 0x4,
			DDSD_PITCH = 0x8,
			DDSD_PIXELFORMAT = 0x1000,
			DDSD_MIPMAPCOUNT = 0x20000,
			DDSD_LINEARSIZE = 0x80000,
			DDSD_DEPTH = 0x800000,
		};

		enum DDPF_FLAGS
		{
			DDPF_ALPHAPIXELS = 0x1,
			DDPF_ALPHA = 0x2,
			DDPF_FOURCC = 0x4,
			DDPF_RGB = 0x40,
			DDPF_YUV = 0x200,
			DDPF_LUMINANCE = 0x20000,
		};

		enum DDSCAPS2
		{
			DDSCAPS2_CUBEMAP = 0x200,
			DDSCAPS2_CUBEMAP_POSITIVEX = 0x400,
			DDSCAPS2_CUBEMAP_NEGATIVEX = 0x800,
			DDSCAPS2_CUBEMAP_POSITIVEY = 0x1000,
			DDSCAPS2_CUBEMAP_NEGATIVEY = 0x2000,
			DDSCAPS2_CUBEMAP_POSITIVEZ = 0x4000,
			DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x8000,
			DDSCAPS2_VOLUME = 0x200000
		};

		enum D3D10_RESOURCE_DIMENSION
		{
			D3D10_RESOURCE_DIMENSION_TEXTURE1D = 2,
			D3D10_RESOURCE_DIMENSION_TEXTURE2D = 3,
			D3D10_RESOURCE_DIMENSION_TEXTURE3D = 4
		};

		enum D3D10_RESOURCE_MISC_FLAG
		{
			D3D10_RESOURCE_MISC_GENERATE_MIPS = 0x1L,
			D3D10_RESOURCE_MISC_SHARED = 0x2L,
			D3D10_RESOURCE_MISC_TEXTURECUBE = 0x4L,
			D3D10_RESOURCE_MISC_SHARED_KEYEDMUTEX = 0x10L,
			D3D10_RESOURCE_MISC_GDI_COMPATIBLE = 0x20L
		};

		enum DDS_ALPHA_MODE
		{
			DDS_ALPHA_MODE_UNKNOWN = 0x0,
			DDS_ALPHA_MODE_STRAIGHT = 0x1,
			DDS_ALPHA_MODE_PREMULTIPLIED = 0x2,
			DDS_ALPHA_MODE_OPAQUE = 0x3,
			DDS_ALPHA_MODE_CUSTOM = 0x4,
		};

		enum DXGI_FORMAT
		{
			DXGI_FORMAT_UNKNOWN = 0,
			DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
			DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
			DXGI_FORMAT_R32G32B32A32_UINT = 3,
			DXGI_FORMAT_R32G32B32A32_SINT = 4,
			DXGI_FORMAT_R32G32B32_TYPELESS = 5,
			DXGI_FORMAT_R32G32B32_FLOAT = 6,
			DXGI_FORMAT_R32G32B32_UINT = 7,
			DXGI_FORMAT_R32G32B32_SINT = 8,
			DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
			DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
			DXGI_FORMAT_R16G16B16A16_UNORM = 11,
			DXGI_FORMAT_R16G16B16A16_UINT = 12,
			DXGI_FORMAT_R16G16B16A16_SNORM = 13,
			DXGI_FORMAT_R16G16B16A16_SINT = 14,
			DXGI_FORMAT_R32G32_TYPELESS = 15,
			DXGI_FORMAT_R32G32_FLOAT = 16,
			DXGI_FORMAT_R32G32_UINT = 17,
			DXGI_FORMAT_R32G32_SINT = 18,
			DXGI_FORMAT_R32G8X24_TYPELESS = 19,
			DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
			DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
			DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
			DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
			DXGI_FORMAT_R10G10B10A2_UNORM = 24,
			DXGI_FORMAT_R10G10B10A2_UINT = 25,
			DXGI_FORMAT_R11G11B10_FLOAT = 26,
			DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
			DXGI_FORMAT_R8G8B8A8_UNORM = 28,
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
			DXGI_FORMAT_R8G8B8A8_UINT = 30,
			DXGI_FORMAT_R8G8B8A8_SNORM = 31,
			DXGI_FORMAT_R8G8B8A8_SINT = 32,
			DXGI_FORMAT_R16G16_TYPELESS = 33,
			DXGI_FORMAT_R16G16_FLOAT = 34,
			DXGI_FORMAT_R16G16_UNORM = 35,
			DXGI_FORMAT_R16G16_UINT = 36,
			DXGI_FORMAT_R16G16_SNORM = 37,
			DXGI_FORMAT_R16G16_SINT = 38,
			DXGI_FORMAT_R32_TYPELESS = 39,
			DXGI_FORMAT_D32_FLOAT = 40,
			DXGI_FORMAT_R32_FLOAT = 41,
			DXGI_FORMAT_R32_UINT = 42,
			DXGI_FORMAT_R32_SINT = 43,
			DXGI_FORMAT_R24G8_TYPELESS = 44,
			DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
			DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
			DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
			DXGI_FORMAT_R8G8_TYPELESS = 48,
			DXGI_FORMAT_R8G8_UNORM = 49,
			DXGI_FORMAT_R8G8_UINT = 50,
			DXGI_FORMAT_R8G8_SNORM = 51,
			DXGI_FORMAT_R8G8_SINT = 52,
			DXGI_FORMAT_R16_TYPELESS = 53,
			DXGI_FORMAT_R16_FLOAT = 54,
			DXGI_FORMAT_D16_UNORM = 55,
			DXGI_FORMAT_R16_UNORM = 56,
			DXGI_FORMAT_R16_UINT = 57,
			DXGI_FORMAT_R16_SNORM = 58,
			DXGI_FORMAT_R16_SINT = 59,
			DXGI_FORMAT_R8_TYPELESS = 60,
			DXGI_FORMAT_R8_UNORM = 61,
			DXGI_FORMAT_R8_UINT = 62,
			DXGI_FORMAT_R8_SNORM = 63,
			DXGI_FORMAT_R8_SINT = 64,
			DXGI_FORMAT_A8_UNORM = 65,
			DXGI_FORMAT_R1_UNORM = 66,
			DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
			DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
			DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
			DXGI_FORMAT_BC1_TYPELESS = 70,
			DXGI_FORMAT_BC1_UNORM = 71,
			DXGI_FORMAT_BC1_UNORM_SRGB = 72,
			DXGI_FORMAT_BC2_TYPELESS = 73,
			DXGI_FORMAT_BC2_UNORM = 74,
			DXGI_FORMAT_BC2_UNORM_SRGB = 75,
			DXGI_FORMAT_BC3_TYPELESS = 76,
			DXGI_FORMAT_BC3_UNORM = 77,
			DXGI_FORMAT_BC3_UNORM_SRGB = 78,
			DXGI_FORMAT_BC4_TYPELESS = 79,
			DXGI_FORMAT_BC4_UNORM = 80,
			DXGI_FORMAT_BC4_SNORM = 81,
			DXGI_FORMAT_BC5_TYPELESS = 82,
			DXGI_FORMAT_BC5_UNORM = 83,
			DXGI_FORMAT_BC5_SNORM = 84,
			DXGI_FORMAT_B5G6R5_UNORM = 85,
			DXGI_FORMAT_B5G5R5A1_UNORM = 86,
			DXGI_FORMAT_B8G8R8A8_UNORM = 87,
			DXGI_FORMAT_B8G8R8X8_UNORM = 88,
			DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
			DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
			DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
			DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
			DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
			DXGI_FORMAT_BC6H_TYPELESS = 94,
			DXGI_FORMAT_BC6H_UF16 = 95,
			DXGI_FORMAT_BC6H_SF16 = 96,
			DXGI_FORMAT_BC7_TYPELESS = 97,
			DXGI_FORMAT_BC7_UNORM = 98,
			DXGI_FORMAT_BC7_UNORM_SRGB = 99,
			DXGI_FORMAT_AYUV = 100,
			DXGI_FORMAT_Y410 = 101,
			DXGI_FORMAT_Y416 = 102,
			DXGI_FORMAT_NV12 = 103,
			DXGI_FORMAT_P010 = 104,
			DXGI_FORMAT_P016 = 105,
			DXGI_FORMAT_420_OPAQUE = 106,
			DXGI_FORMAT_YUY2 = 107,
			DXGI_FORMAT_Y210 = 108,
			DXGI_FORMAT_Y216 = 109,
			DXGI_FORMAT_NV11 = 110,
			DXGI_FORMAT_AI44 = 111,
			DXGI_FORMAT_IA44 = 112,
			DXGI_FORMAT_P8 = 113,
			DXGI_FORMAT_A8P8 = 114,
			DXGI_FORMAT_B4G4R4A4_UNORM = 115,
			DXGI_FORMAT_FORCE_UINT = 0xffffffff
		};

		enum D3DFORMAT
		{
			D3DFMT_UNKNOWN = 0,

			D3DFMT_R8G8B8 = 20,
			D3DFMT_A8R8G8B8 = 21,
			D3DFMT_X8R8G8B8 = 22,
			D3DFMT_R5G6B5 = 23,
			D3DFMT_X1R5G5B5 = 24,
			D3DFMT_A1R5G5B5 = 25,
			D3DFMT_A4R4G4B4 = 26,
			D3DFMT_R3G3B2 = 27,
			D3DFMT_A8 = 28,
			D3DFMT_A8R3G3B2 = 29,
			D3DFMT_X4R4G4B4 = 30,
			D3DFMT_A2B10G10R10 = 31,
			D3DFMT_A8B8G8R8 = 32,
			D3DFMT_X8B8G8R8 = 33,
			D3DFMT_G16R16 = 34,
			D3DFMT_A2R10G10B10 = 35,
			D3DFMT_A16B16G16R16 = 36,

			D3DFMT_A8P8 = 40,
			D3DFMT_P8 = 41,

			D3DFMT_L8 = 50,
			D3DFMT_A8L8 = 51,
			D3DFMT_A4L4 = 52,

			D3DFMT_V8U8 = 60,
			D3DFMT_L6V5U5 = 61,
			D3DFMT_X8L8V8U8 = 62,
			D3DFMT_Q8W8V8U8 = 63,
			D3DFMT_V16U16 = 64,
			D3DFMT_A2W10V10U10 = 67,

			D3DFMT_UYVY = MAKEFOURCC('U', 'Y', 'V', 'Y'),
			D3DFMT_R8G8_B8G8 = MAKEFOURCC('R', 'G', 'B', 'G'),
			D3DFMT_YUY2 = MAKEFOURCC('Y', 'U', 'Y', '2'),
			D3DFMT_G8R8_G8B8 = MAKEFOURCC('G', 'R', 'G', 'B'),
			D3DFMT_DXT1 = MAKEFOURCC('D', 'X', 'T', '1'),
			D3DFMT_DXT2 = MAKEFOURCC('D', 'X', 'T', '2'),
			D3DFMT_DXT3 = MAKEFOURCC('D', 'X', 'T', '3'),
			D3DFMT_DXT4 = MAKEFOURCC('D', 'X', 'T', '4'),
			D3DFMT_DXT5 = MAKEFOURCC('D', 'X', 'T', '5'),

			D3DFMT_ATI1 = MAKEFOURCC('A', 'T', 'I', '1'),
			D3DFMT_ATI2 = MAKEFOURCC('A', 'T', 'I', '2'),

			D3DFMT_BC4U = MAKEFOURCC('B', 'C', '4', 'U'),
			D3DFMT_BC4S = MAKEFOURCC('B', 'C', '4', 'S'),

			D3DFMT_BC5U = MAKEFOURCC('B', 'C', '5', 'U'),
			D3DFMT_BC5S = MAKEFOURCC('B', 'C', '5', 'S'),

			D3DFMT_D16_LOCKABLE = 70,
			D3DFMT_D32 = 71,
			D3DFMT_D15S1 = 73,
			D3DFMT_D24S8 = 75,
			D3DFMT_D24X8 = 77,
			D3DFMT_D24X4S4 = 79,
			D3DFMT_D16 = 80,

			D3DFMT_D32F_LOCKABLE = 82,
			D3DFMT_D24FS8 = 83,

			/* Z-Stencil formats valid for CPU access */
			D3DFMT_D32_LOCKABLE = 84,
			D3DFMT_S8_LOCKABLE = 85,

			D3DFMT_L16 = 81,

			D3DFMT_VERTEXDATA = 100,
			D3DFMT_INDEX16 = 101,
			D3DFMT_INDEX32 = 102,

			D3DFMT_Q16W16V16U16 = 110,

			D3DFMT_MULTI2_ARGB8 = MAKEFOURCC('M', 'E', 'T', '1'),

			// Floating point surface formats

			// s10e5 formats (16-bits per channel)
			D3DFMT_R16F = 111,
			D3DFMT_G16R16F = 112,
			D3DFMT_A16B16G16R16F = 113,

			// IEEE s23e8 formats (32-bits per channel)
			D3DFMT_R32F = 114,
			D3DFMT_G32R32F = 115,
			D3DFMT_A32B32G32R32F = 116,

			D3DFMT_CxV8U8 = 117,

			// Monochrome 1 bit per pixel format
			D3DFMT_A1 = 118,

			// 2.8 biased fixed point
			D3DFMT_A2B10G10R10_XR_BIAS = 119,

			// Binary format indicating that the data has no inherent type
			D3DFMT_BINARYBUFFER = 199,

			D3DFMT_FORCE_DWORD = 0x7fffffff
		};

		struct DDS_HEADER_DXT10
		{
			DXGI_FORMAT dxgiFormat;
			D3D10_RESOURCE_DIMENSION resourceDimension;
			u32 miscFlag;
			u32 arraySize;
			u32 miscFlags2;
		};

		GPU::Format GetFormat(DXGI_FORMAT format)
		{
			switch(format)
			{
			case DXGI_FORMAT_BC1_UNORM:
				return GPU::Format::BC1_UNORM;
			case DXGI_FORMAT_BC1_UNORM_SRGB:
				return GPU::Format::BC1_UNORM_SRGB;

			case DXGI_FORMAT_BC2_UNORM:
				return GPU::Format::BC2_UNORM;
			case DXGI_FORMAT_BC2_UNORM_SRGB:
				return GPU::Format::BC2_UNORM_SRGB;

			case DXGI_FORMAT_BC3_UNORM:
				return GPU::Format::BC3_UNORM;
			case DXGI_FORMAT_BC3_UNORM_SRGB:
				return GPU::Format::BC3_UNORM_SRGB;

			case DXGI_FORMAT_BC4_UNORM:
				return GPU::Format::BC4_UNORM;
			case DXGI_FORMAT_BC4_SNORM:
				return GPU::Format::BC4_SNORM;

			case DXGI_FORMAT_BC5_UNORM:
				return GPU::Format::BC5_UNORM;
			case DXGI_FORMAT_BC5_SNORM:
				return GPU::Format::BC5_SNORM;

			case DXGI_FORMAT_BC6H_UF16:
				return GPU::Format::BC6H_UF16;
			case DXGI_FORMAT_BC6H_SF16:
				return GPU::Format::BC6H_SF16;

			case DXGI_FORMAT_BC7_UNORM:
				return GPU::Format::BC7_UNORM;
			case DXGI_FORMAT_BC7_UNORM_SRGB:
				return GPU::Format::BC7_UNORM_SRGB;
			}
			return GPU::Format::INVALID;
		}

		GPU::Format GetResourceFormat(D3DFORMAT Format)
		{
			switch(Format)
			{
			case D3DFMT_DXT1:
				return GPU::Format::BC1_UNORM;

			case D3DFMT_DXT2:
			case D3DFMT_DXT3:
				return GPU::Format::BC2_UNORM;

			case D3DFMT_DXT4:
			case D3DFMT_DXT5:
				return GPU::Format::BC3_UNORM;

			case D3DFMT_ATI1:
				return GPU::Format::BC4_UNORM;

			case D3DFMT_ATI2:
				return GPU::Format::BC5_UNORM;

			case D3DFMT_BC4U:
				return GPU::Format::BC4_UNORM;
			case D3DFMT_BC4S:
				return GPU::Format::BC4_SNORM;

			case D3DFMT_BC5U:
				return GPU::Format::BC5_UNORM;
			case D3DFMT_BC5S:
				return GPU::Format::BC5_SNORM;
			}

			return GPU::Format::INVALID;
		}

		Graphics::Image LoadImage(Resource::IConverterContext& context, const char* sourceFile)
		{
			Core::File imageFile(sourceFile, Core::FileFlags::READ, context.GetPathResolver());

			u32 magic = 0;
			DDS_HEADER ddsHeader = {};
			DDS_HEADER_DXT10 ddsHeaderDXT10 = {};

			// Read magic.
			imageFile.Read(&magic, sizeof(magic));
			if(magic != 0x20534444)
			{
				return Graphics::Image();
			}

			// Read header.
			imageFile.Read(&ddsHeader, sizeof(ddsHeader));

			const u32 Flags1D = DDSD_WIDTH;
			const u32 Flags2D = DDSD_WIDTH | DDSD_HEIGHT;
			const u32 Flags3D = DDSD_WIDTH | DDSD_HEIGHT | DDSD_DEPTH;
			const u32 FlagsCube = DDSCAPS2_CUBEMAP;

			GPU::TextureType type = GPU::TextureType::INVALID;

			if(Core::ContainsAllFlags(ddsHeader.dwFlags, Flags1D))
			{
				type = GPU::TextureType::TEX1D;
			}
			if(Core::ContainsAllFlags(ddsHeader.dwFlags, Flags2D))
			{
				type = GPU::TextureType::TEX2D;
			}
			if(Core::ContainsAllFlags(ddsHeader.dwFlags, Flags3D))
			{
				type = GPU::TextureType::TEX3D;
			}
			if(Core::ContainsAllFlags(ddsHeader.dwCaps2, FlagsCube))
			{
				type = GPU::TextureType::TEXCUBE;
			}

			GPU::Format format = DDS::GetResourceFormat(DDS::D3DFORMAT(ddsHeader.ddspf.dwFourCC));

			// Check for DX10 format.
			if(Core::ContainsAllFlags(ddsHeader.ddspf.dwFlags, u32(DDPF_FOURCC)) &&
			    ddsHeader.ddspf.dwFourCC == MAKEFOURCC('D', 'X', '1', '0'))
			{
				imageFile.Read(&ddsHeaderDXT10, sizeof(ddsHeaderDXT10));
				format = DDS::GetFormat(ddsHeaderDXT10.dxgiFormat);
			}

			// No format determined, log error and fail.
			if(format == GPU::Format::INVALID)
			{
				char error[4096] = {0};
				sprintf_s(error, sizeof(error), "Unable to load texture \"%s\", unsupported format.", sourceFile);
				context.AddError(__FILE__, __LINE__, error);

				return Graphics::Image();
			}

			// Calculate size.
			auto formatSize = GPU::GetTextureSize(
			    format, ddsHeader.dwWidth, ddsHeader.dwHeight, ddsHeader.dwDepth, ddsHeader.dwMipMapCount, 1);
			u8* data = new u8[formatSize];
			imageFile.Read(data, formatSize);

			// Return setup image.
			return Graphics::Image(type, format, ddsHeader.dwWidth, ddsHeader.dwHeight, ddsHeader.dwDepth,
			    ddsHeader.dwMipMapCount, data, [](u8* data) { delete[] data; });
		}
	} // namespace DDS
} // namespace Graphics
