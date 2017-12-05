#include "gpu/enum.h"

namespace Core
{
	const char* EnumToString(GPU::Format val)
	{
#define CASE_STRING(ENUM_VALUE)                                                                                        \
	\
case GPU::Format::##ENUM_VALUE : return #ENUM_VALUE;

		switch(val)
		{
			CASE_STRING(R32G32B32A32_TYPELESS)
			CASE_STRING(R32G32B32A32_FLOAT)
			CASE_STRING(R32G32B32A32_UINT)
			CASE_STRING(R32G32B32A32_SINT)
			CASE_STRING(R32G32B32_TYPELESS)
			CASE_STRING(R32G32B32_FLOAT)
			CASE_STRING(R32G32B32_UINT)
			CASE_STRING(R32G32B32_SINT)
			CASE_STRING(R16G16B16A16_TYPELESS)
			CASE_STRING(R16G16B16A16_FLOAT)
			CASE_STRING(R16G16B16A16_UNORM)
			CASE_STRING(R16G16B16A16_UINT)
			CASE_STRING(R16G16B16A16_SNORM)
			CASE_STRING(R16G16B16A16_SINT)
			CASE_STRING(R32G32_TYPELESS)
			CASE_STRING(R32G32_FLOAT)
			CASE_STRING(R32G32_UINT)
			CASE_STRING(R32G32_SINT)
			CASE_STRING(R32G8X24_TYPELESS)
			CASE_STRING(D32_FLOAT_S8X24_UINT)
			CASE_STRING(R32_FLOAT_X8X24_TYPELESS)
			CASE_STRING(X32_TYPELESS_G8X24_UINT)
			CASE_STRING(R10G10B10A2_TYPELESS)
			CASE_STRING(R10G10B10A2_UNORM)
			CASE_STRING(R10G10B10A2_UINT)
			CASE_STRING(R11G11B10_FLOAT)
			CASE_STRING(R8G8B8A8_TYPELESS)
			CASE_STRING(R8G8B8A8_UNORM)
			CASE_STRING(R8G8B8A8_UNORM_SRGB)
			CASE_STRING(R8G8B8A8_UINT)
			CASE_STRING(R8G8B8A8_SNORM)
			CASE_STRING(R8G8B8A8_SINT)
			CASE_STRING(R16G16_TYPELESS)
			CASE_STRING(R16G16_FLOAT)
			CASE_STRING(R16G16_UNORM)
			CASE_STRING(R16G16_UINT)
			CASE_STRING(R16G16_SNORM)
			CASE_STRING(R16G16_SINT)
			CASE_STRING(R32_TYPELESS)
			CASE_STRING(D32_FLOAT)
			CASE_STRING(R32_FLOAT)
			CASE_STRING(R32_UINT)
			CASE_STRING(R32_SINT)
			CASE_STRING(R24G8_TYPELESS)
			CASE_STRING(D24_UNORM_S8_UINT)
			CASE_STRING(R24_UNORM_X8_TYPELESS)
			CASE_STRING(X24_TYPELESS_G8_UINT)
			CASE_STRING(R8G8_TYPELESS)
			CASE_STRING(R8G8_UNORM)
			CASE_STRING(R8G8_UINT)
			CASE_STRING(R8G8_SNORM)
			CASE_STRING(R8G8_SINT)
			CASE_STRING(R16_TYPELESS)
			CASE_STRING(R16_FLOAT)
			CASE_STRING(D16_UNORM)
			CASE_STRING(R16_UNORM)
			CASE_STRING(R16_UINT)
			CASE_STRING(R16_SNORM)
			CASE_STRING(R16_SINT)
			CASE_STRING(R8_TYPELESS)
			CASE_STRING(R8_UNORM)
			CASE_STRING(R8_UINT)
			CASE_STRING(R8_SNORM)
			CASE_STRING(R8_SINT)
			CASE_STRING(A8_UNORM)
			CASE_STRING(R1_UNORM)
			CASE_STRING(R9G9B9E5_SHAREDEXP)
			CASE_STRING(R8G8_B8G8_UNORM)
			CASE_STRING(G8R8_G8B8_UNORM)
			CASE_STRING(BC1_TYPELESS)
			CASE_STRING(BC1_UNORM)
			CASE_STRING(BC1_UNORM_SRGB)
			CASE_STRING(BC2_TYPELESS)
			CASE_STRING(BC2_UNORM)
			CASE_STRING(BC2_UNORM_SRGB)
			CASE_STRING(BC3_TYPELESS)
			CASE_STRING(BC3_UNORM)
			CASE_STRING(BC3_UNORM_SRGB)
			CASE_STRING(BC4_TYPELESS)
			CASE_STRING(BC4_UNORM)
			CASE_STRING(BC4_SNORM)
			CASE_STRING(BC5_TYPELESS)
			CASE_STRING(BC5_UNORM)
			CASE_STRING(BC5_SNORM)
			CASE_STRING(B5G6R5_UNORM)
			CASE_STRING(B5G5R5A1_UNORM)
			CASE_STRING(B8G8R8A8_UNORM)
			CASE_STRING(B8G8R8X8_UNORM)
			CASE_STRING(R10G10B10_XR_BIAS_A2_UNORM)
			CASE_STRING(B8G8R8A8_TYPELESS)
			CASE_STRING(B8G8R8A8_UNORM_SRGB)
			CASE_STRING(B8G8R8X8_TYPELESS)
			CASE_STRING(B8G8R8X8_UNORM_SRGB)
			CASE_STRING(BC6H_TYPELESS)
			CASE_STRING(BC6H_UF16)
			CASE_STRING(BC6H_SF16)
			CASE_STRING(BC7_TYPELESS)
			CASE_STRING(BC7_UNORM)
			CASE_STRING(BC7_UNORM_SRGB)
			CASE_STRING(ETC1_UNORM)
			CASE_STRING(ETC2_UNORM)
			CASE_STRING(ETC2A_UNORM)
			CASE_STRING(ETC2A1_UNORM)
		}
#undef CASE_STRING
		return nullptr;
	}

	const char* EnumToString(GPU::TextureType val)
	{
#define CASE_STRING(ENUM_VALUE)                                                                                        \
	\
case GPU::TextureType::##ENUM_VALUE : return #ENUM_VALUE;

		switch(val)
		{
			CASE_STRING(INVALID)
			CASE_STRING(TEX1D)
			CASE_STRING(TEX2D)
			CASE_STRING(TEX3D)
			CASE_STRING(TEXCUBE)
		}

#undef CASE_STRING
		return nullptr;
	}

	const char* EnumToString(GPU::AddressingMode val)
	{
#define CASE_STRING(ENUM_VALUE)                                                                                        \
	\
case GPU::AddressingMode::##ENUM_VALUE : return #ENUM_VALUE;

		switch(val)
		{
			CASE_STRING(WRAP)
			CASE_STRING(MIRROR)
			CASE_STRING(CLAMP)
			CASE_STRING(BORDER)
		}

#undef CASE_STRING
		return nullptr;
	}

	const char* EnumToString(GPU::FilteringMode val)
	{
#define CASE_STRING(ENUM_VALUE)                                                                                        \
	\
case GPU::FilteringMode::##ENUM_VALUE : return #ENUM_VALUE;

		switch(val)
		{
			CASE_STRING(NEAREST)
			CASE_STRING(LINEAR)
			CASE_STRING(NEAREST_MIPMAP_NEAREST)
			CASE_STRING(LINEAR_MIPMAP_NEAREST)
			CASE_STRING(NEAREST_MIPMAP_LINEAR)
			CASE_STRING(LINEAR_MIPMAP_LINEAR)
		}

#undef CASE_STRING
		return nullptr;
	}

	const char* EnumToString(GPU::BorderColor val)
	{
#define CASE_STRING(ENUM_VALUE)                                                                                        \
	\
case GPU::BorderColor::##ENUM_VALUE : return #ENUM_VALUE;

		switch(val)
		{
			CASE_STRING(TRANSPARENT)
			CASE_STRING(BLACK)
			CASE_STRING(WHITE)
		}

#undef CASE_STRING
		return nullptr;
	}

	const char* EnumToString(GPU::FillMode val)
	{
#define CASE_STRING(ENUM_VALUE)                                                                                        \
	\
case GPU::FillMode::##ENUM_VALUE : return #ENUM_VALUE;

		switch(val)
		{
			CASE_STRING(INVALID)
			CASE_STRING(SOLID)
			CASE_STRING(WIREFRAME)
		}

#undef CASE_STRING
		return nullptr;
	}

	const char* EnumToString(GPU::CullMode val)
	{
#define CASE_STRING(ENUM_VALUE)                                                                                        \
	\
case GPU::CullMode::##ENUM_VALUE : return #ENUM_VALUE;

		switch(val)
		{
			CASE_STRING(INVALID)
			CASE_STRING(NONE)
			CASE_STRING(CCW)
			CASE_STRING(CW)
		}

#undef CASE_STRING
		return nullptr;
	}

	const char* EnumToString(GPU::BlendType val)
	{
#define CASE_STRING(ENUM_VALUE)                                                                                        \
	\
case GPU::BlendType::##ENUM_VALUE : return #ENUM_VALUE;

		switch(val)
		{
			CASE_STRING(INVALID)
			CASE_STRING(ZERO)
			CASE_STRING(ONE)
			CASE_STRING(SRC_COLOR)
			CASE_STRING(INV_SRC_COLOR)
			CASE_STRING(SRC_ALPHA)
			CASE_STRING(INV_SRC_ALPHA)
			CASE_STRING(DEST_COLOR)
			CASE_STRING(INV_DEST_COLOR)
			CASE_STRING(DEST_ALPHA)
			CASE_STRING(INV_DEST_ALPHA)
		}

#undef CASE_STRING
		return nullptr;
	}

	const char* EnumToString(GPU::BlendFunc val)
	{
#define CASE_STRING(ENUM_VALUE)                                                                                        \
	\
case GPU::BlendFunc::##ENUM_VALUE : return #ENUM_VALUE;

		switch(val)
		{
			CASE_STRING(INVALID)
			CASE_STRING(ADD)
			CASE_STRING(SUBTRACT)
			CASE_STRING(REV_SUBTRACT)
			CASE_STRING(MINIMUM)
			CASE_STRING(MAXIMUM)
		}

#undef CASE_STRING
		return nullptr;
	}

	const char* EnumToString(GPU::CompareMode val)
	{
#define CASE_STRING(ENUM_VALUE)                                                                                        \
	\
case GPU::CompareMode::##ENUM_VALUE : return #ENUM_VALUE;

		switch(val)
		{
			CASE_STRING(INVALID)
			CASE_STRING(NEVER)
			CASE_STRING(LESS)
			CASE_STRING(EQUAL)
			CASE_STRING(LESS_EQUAL)
			CASE_STRING(GREATER)
			CASE_STRING(NOT_EQUAL)
			CASE_STRING(GREATER_EQUAL)
			CASE_STRING(ALWAYS)
		}

#undef CASE_STRING
		return nullptr;
	}

	const char* EnumToString(GPU::StencilFunc val)
	{
#define CASE_STRING(ENUM_VALUE)                                                                                        \
	\
case GPU::StencilFunc::##ENUM_VALUE : return #ENUM_VALUE;

		switch(val)
		{
			CASE_STRING(INVALID)
			CASE_STRING(KEEP)
			CASE_STRING(ZERO)
			CASE_STRING(REPLACE)
			CASE_STRING(INCR)
			CASE_STRING(INCR_WRAP)
			CASE_STRING(DECR)
			CASE_STRING(DECR_WRAP)
			CASE_STRING(INVERT)
		}

#undef CASE_STRING
		return nullptr;
	}

	const char* EnumToString(GPU::ShaderType val)
	{
#define CASE_STRING(ENUM_VALUE)                                                                                        \
	\
case GPU::ShaderType::##ENUM_VALUE : return #ENUM_VALUE;

		switch(val)
		{
			CASE_STRING(INVALID)
			CASE_STRING(VS)
			CASE_STRING(GS)
			CASE_STRING(HS)
			CASE_STRING(DS)
			CASE_STRING(PS)
			CASE_STRING(CS)
		}

#undef CASE_STRING
		return nullptr;
	}

} // namespace Core
