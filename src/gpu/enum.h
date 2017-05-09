#pragma once

#include "gpu/dll.h"
#include "gpu/types.h"
#include "core/enum.h"

namespace Core
{
	template<>
	struct GPU_DLL Enum<GPU::Format>
	{
		static const char* ToString(i32 val);
	};

	template<>
	struct GPU_DLL Enum<GPU::TextureType>
	{
		static const char* ToString(i32 val);
	};

	template<>
	struct GPU_DLL Enum<GPU::AddressingMode>
	{
		static const char* ToString(i32 val);
	};

	template<>
	struct GPU_DLL Enum<GPU::FilteringMode>
	{
		static const char* ToString(i32 val);
	};

	template<>
	struct GPU_DLL Enum<GPU::FillMode>
	{
		static const char* ToString(i32 val);
	};

	template<>
	struct GPU_DLL Enum<GPU::CullMode>
	{
		static const char* ToString(i32 val);
	};

	template<>
	struct GPU_DLL Enum<GPU::BlendType>
	{
		static const char* ToString(i32 val);
	};

	template<>
	struct GPU_DLL Enum<GPU::BlendFunc>
	{
		static const char* ToString(i32 val);
	};

	template<>
	struct GPU_DLL Enum<GPU::CompareMode>
	{
		static const char* ToString(i32 val);
	};

	template<>
	struct GPU_DLL Enum<GPU::StencilFunc>
	{
		static const char* ToString(i32 val);
	};
}