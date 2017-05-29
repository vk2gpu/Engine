#include "core/string.h"
#include "core/debug.h"
#include "core/hash.h"

#if PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include "core/os.h"
#endif

#include <cstdio>
#include <utility>

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

	void String::Printf(const char* fmt, ...)
	{
		va_list argList;
		va_start(argList, fmt);
		Printfv(fmt, argList);
		va_end(argList);
	}

	void String::Printfv(const char* fmt, va_list argList)
	{
		Core::Array<char, 4096> buffer;
		vsprintf_s(buffer.data(), buffer.size(), fmt, argList);
		internalSet(buffer.data());
	}

	void String::Appendf(const char* fmt, ...)
	{
		va_list argList;
		va_start(argList, fmt);
		Appendfv(fmt, argList);
		va_end(argList);
	}

	void String::Appendfv(const char* fmt, va_list argList)
	{
		Core::Array<char, 4096> buffer;
		vsprintf_s(buffer.data(), buffer.size(), fmt, argList);
		internalAppend(buffer.data());
	}

	String& String::internalRemoveNullTerminator()
	{
		if(data_.size() > 1)
			data_.pop_back();
		return *this;
	}

	String& String::internalSet(const char* str)
	{
		DBG_ASSERT(str);
		i32 len = (i32)strlen(str) + 1;
		data_.reserve(len);
		data_.clear();
		data_.insert(str, str + len);
		return *this;
	}

	String& String::internalAppend(const char* str)
	{
		DBG_ASSERT(str);
		i32 len = (i32)strlen(str) + 1;
		internalRemoveNullTerminator();
		data_.insert(str, str + len);
		return *this;
	}

	int String::internalCompare(const char* str) const
	{
		DBG_ASSERT(str);
		if(data_.size() > 0)
		{
			return strcmp(data_.data(), str);
		}
		return strcmp("", str);
	}

	u32 Hash(u32 input, const String& str)
	{
		if(str.size() > 0)
			return Hash(input, str.c_str());
		else
			return Hash(input, "\0");
	}

} // end namespace
