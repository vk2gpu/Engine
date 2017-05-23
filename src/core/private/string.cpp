#include "core/string.h"
#include "core/debug.h"

#if PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include "core/os.h"
#endif

namespace Core
{
	bool StringConvertUTF16toUTF8(const wchar* src, i32 srcLength, char* dst, i32 dstLength)
	{
		DBG_ASSERT(src);
		DBG_ASSERT(dst);
		DBG_ASSERT(srcLength > 0);
		DBG_ASSERT(dstLength > 1);

// TODO: Platform agnostic conversion. Taking a shortcut here for speed.
#if PLATFORM_WINDOWS
		auto retVal = ::WideCharToMultiByte(CP_UTF8, 0, src, srcLength, dst, dstLength, nullptr, nullptr);
		return retVal > 0;
#else
#error "Not implemented for platform."
#endif
	}

	bool StringConvertUTF8toUTF16(const char* src, i32 srcLength, wchar* dst, i32 dstLength)
	{
		DBG_ASSERT(src);
		DBG_ASSERT(dst);
		DBG_ASSERT(srcLength > 0);
		DBG_ASSERT(dstLength > 1);

// TODO: Platform agnostic conversion. Taking a shortcut here for speed.
#if PLATFORM_WINDOWS
		auto retVal = ::MultiByteToWideChar(CP_UTF8, 0, src, srcLength, dst, dstLength);
		return retVal > 0;
#else
#error "Not implemented for platform."
#endif
	}
} // end namespace
