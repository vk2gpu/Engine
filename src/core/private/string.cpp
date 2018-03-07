#include "core/string.h"
#include "core/allocator_tlsf.h"
#include "core/allocator_proxy_thread_safe.h"
#include "core/debug.h"
#include "core/hash.h"
#include "core/misc.h"

#if PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include "core/os.h"
#endif

#include <cstdio>
#include <utility>

namespace Core
{
	static IAllocator& stringAllocator_ = *[]() {
		static AllocatorTLSF alloc(GeneralAllocator(), 1024 * 1024);
		static AllocatorProxyThreadSafe tsProxy(alloc);
		return &Core::CreateAllocationTracker(tsProxy, "General/String");
	}();

	void* StringAllocator::Allocate(i64 size, i64 align) { return stringAllocator_.Allocate(size, align); }

	void StringAllocator::Deallocate(void* mem) { stringAllocator_.Deallocate(mem); }

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

	String& String::Printf(const char* fmt, ...)
	{
		va_list argList;
		va_start(argList, fmt);
		Printfv(fmt, argList);
		va_end(argList);
		return *this;
	}

	String& String::Printfv(const char* fmt, va_list argList)
	{
		Core::Array<char, 4096> buffer;
		vsprintf_s(buffer.data(), buffer.size(), fmt, argList);
		internalSet(buffer.data());
		return *this;
	}

	String& String::Appendf(const char* fmt, ...)
	{
		va_list argList;
		va_start(argList, fmt);
		Appendfv(fmt, argList);
		va_end(argList);
		return *this;
	}

	String& String::Appendfv(const char* fmt, va_list argList)
	{
		Core::Array<char, 4096> buffer = {0};
		vsprintf_s(buffer.data(), buffer.size(), fmt, argList);
		internalAppend(buffer.data());
		return *this;
	}

	String& String::Append(const char* str)
	{
		internalAppend(str);
		return *this;
	}

	String& String::internalRemoveNullTerminator(bool forceRemove)
	{
		if(data_.size() > (forceRemove ? 0 : 1))
			data_.pop_back();
		return *this;
	}

	String& String::internalSet(const char* begin, const char* end)
	{
		if(begin)
		{
			if(!end)
			{
				i32 len = (i32)strlen(begin) + 1;
				data_.reserve(len);
				data_.clear();
				data_.insert(begin, begin + len);
			}
			else
			{
				data_.clear();
				data_.insert(begin, end);
				data_.emplace_back('\0');
			}
		}
		else
		{
			data_.clear();
		}
		return *this;
	}

	String& String::internalAppend(const char* str, index_type subPos, index_type subLen)
	{
		if(str)
		{
			const index_type strLen = (index_type)strlen(str);
			if(subLen == npos)
				subLen = strLen;
			DBG_ASSERT((subPos + subLen) <= strLen);
			internalRemoveNullTerminator(true);
			data_.insert(str + subPos, str + subPos + subLen);
			data_.emplace_back('\0');
		}

		return *this;
	}

	int String::internalCompare(const char* str) const
	{
		if(!str)
			str = "";
		if(data_.size() > 0)
		{
			return strcmp(data_.data(), str);
		}
		return strcmp("", str);
	}

	String::index_type String::find(const char* str, index_type subPos) const
	{
		if(!str || size() == 0)
			return npos;
		auto found = strstr(&data_[subPos], str);
		if(found == nullptr)
			return npos;
		return (index_type)(found - &data_[0]);
	}

	String String::substr(index_type start, index_type len) const
	{
		DBG_ASSERT(start >= 0 && start < size());
		DBG_ASSERT(len == npos || (len > 0 && len < (size() - start)));
		if(len == npos)
			len = size() - start;
		String retVal;
		retVal.resize(len);
		memcpy(retVal.data(), data() + start, len);
		return retVal;
	}

	String String::replace(const char* search, const char* replacement) const
	{
		String outString;
		outString.reserve(size());

		index_type searchLen = (index_type)strlen(search);

		index_type lastPos = 0;
		index_type foundPos = 0;
		while((foundPos = find(search, lastPos)) != npos)
		{
			outString.append(c_str(), lastPos, foundPos - lastPos);
			outString.append(replacement);
			lastPos = foundPos + searchLen;
		}

		outString.append(&data_[lastPos]);

		return outString;
	}

	int StringView::internalCompare(const char* begin, const char* end) const
	{
		if(!begin)
		{
			begin = "";
			end = begin + 1;
		}

		if(size() > 0)
		{
			if(!end)
				end = begin + strlen(begin);
			auto maxSize = Core::Min((index_type)(end - begin), size());
			return strncmp(begin, begin_, maxSize);
		}
		return strncmp("", begin, 1);
	}

	u64 Hash(u64 input, const String& str)
	{
		if(str.size() > 0)
			return HashFNV1a(input, str.c_str(), str.size());
		else
			return HashFNV1a(input, "", 0);
	}

	u64 Hash(u64 input, const StringView& str)
	{
		if(str.size() > 0)
			return HashFNV1a(input, str.begin(), str.size());
		else
			return HashFNV1a(input, "", 0);
	}

} // end namespace
