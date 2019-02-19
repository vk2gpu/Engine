#pragma once

#if PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include "core/os.h"
#endif

namespace Core
{
	CORE_DLL_INLINE i32 CountLeadingZeros(u32 mask)
	{
#if COMPILER_MSVC
		unsigned long index;
		auto ret = _BitScanReverse(&index, mask);
		return ret ? 31 - index : 32;
#elif COMPILER_GCC || COMPILER_CLANG
		return __builtin_clz(mask);
#else
#error "No BSR implementation."
#endif
	}

	CORE_DLL_INLINE i32 CountLeadingZeros(u64 mask)
	{
#if COMPILER_MSVC
		unsigned long index;
		auto ret = _BitScanReverse64(&index, mask);
		return ret ? 63 - index : 64;
#elif COMPILER_GCC || COMPILER_CLANG
		return __builtin_clzll(mask);
#else
#error "No BSR implementation."
#endif
	}
}
