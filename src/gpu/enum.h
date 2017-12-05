#pragma once

#include "gpu/dll.h"
#include "gpu/types.h"
#include "core/enum.h"

namespace Core
{
	GPU_DLL const char* EnumToString(GPU::Format);
	GPU_DLL const char* EnumToString(GPU::TextureType val);
	GPU_DLL const char* EnumToString(GPU::AddressingMode val);
	GPU_DLL const char* EnumToString(GPU::FilteringMode val);
	GPU_DLL const char* EnumToString(GPU::BorderColor val);
	GPU_DLL const char* EnumToString(GPU::FillMode val);
	GPU_DLL const char* EnumToString(GPU::CullMode val);
	GPU_DLL const char* EnumToString(GPU::BlendType val);
	GPU_DLL const char* EnumToString(GPU::BlendFunc val);
	GPU_DLL const char* EnumToString(GPU::CompareMode val);
	GPU_DLL const char* EnumToString(GPU::StencilFunc val);
	GPU_DLL const char* EnumToString(GPU::ShaderType val);
}