#pragma once

#include "core/dll.h"
#include "core/types.h"
#include "core/array.h"
#include "core/vector.h"

#include <utility>

namespace Core
{
	/**
	 * Convert from UTF-16 to UTF-8.
	 * @param src Source string.
	 * @param srcLength Source string length.
	 * @param dst Destination string.
	 * @param dstLength Destination length including null terminator.
	 * @return Success.
	 * @pre src != nullptr.
	 * @pre dst != nullptr.
	 * @pre srcLength > 0.
	 * @pre dstLength > 1.
	 */
	CORE_DLL bool StringConvertUTF16toUTF8(const wchar* src, i32 srcLength, char* dst, i32 dstLength);

	/**
	 * Convert from UTF-8 to UTF-16.
	 * @param src Source string.
	 * @param srcLength Source string length.
	 * @param dst Destination string.
	 * @param dstLength Destination length including null terminator.
	 * @return Success.
	 * @pre src != nullptr.
	 * @pre dst != nullptr.
	 * @pre srcLength > 0.
	 * @pre dstLength > 1.
	 */
	CORE_DLL bool StringConvertUTF8toUTF16(const char* src, i32 srcLength, wchar* dst, i32 dstLength);

	/**
	 * String class.
	 */
	class String
	{
	public:
		String() {}
		String(const char* str) { internalSet(str); }
		String(const String& str) { internalSet(str.c_str()); }
		String(String&& str) { swap(str); }
		~String() {}

		// Custom interfaces.
		CORE_DLL void Printf(const char* fmt, ...);
		CORE_DLL void Printfv(const char* fmt, va_list argList);
		CORE_DLL void Appendf(const char* fmt, ...);
		CORE_DLL void Appendfv(const char* fmt, va_list argList);

		// STL compatible interfaces.
		void clear() { data_.clear(); }
		const char* c_str() const { return data_.data(); }
		i32 size() const { return data_.size() > 0 ? data_.size() - 1 : 0; }

		void reserve(i32 capacity) { data_.reserve(capacity); }
		void swap(String& other) { data_.swap(other.data_); }

		// cstring versions.
		void append(const char* str) { internalAppend(str); }
		int compare(const char* str) const { return internalCompare(str); }

		bool operator==(const char* str) const { return internalCompare(str) == 0; }
		bool operator!=(const char* str) const { return internalCompare(str) != 0; }
		bool operator<(const char* str) const { return internalCompare(str) < 0; }
		bool operator>(const char* str) const { return internalCompare(str) > 0; }
		bool operator<=(const char* str) const { return internalCompare(str) <= 0; }
		bool operator>=(const char* str) const { return internalCompare(str) >= 0; }

		// String versions.
		void append(const String& str) { internalAppend(str.c_str()); }
		int compare(const String& str) const { return internalCompare(str.c_str()); }

		String& operator=(const char* str) { return internalSet(str); }
		String& operator=(const String& str) { return internalSet(str.c_str()); }
		String& operator=(String&& str)
		{
			swap(str);
			return (*this);
		}

		String& operator+=(const char* str) { return internalAppend(str); }
		String& operator+=(const String& str) { return internalAppend(str.c_str()); }

		bool operator==(const String& str) const { return internalCompare(str.c_str()) == 0; }
		bool operator!=(const String& str) const { return internalCompare(str.c_str()) != 0; }
		bool operator<(const String& str) const { return internalCompare(str.c_str()) < 0; }
		bool operator>(const String& str) const { return internalCompare(str.c_str()) > 0; }
		bool operator<=(const String& str) const { return internalCompare(str.c_str()) <= 0; }
		bool operator>=(const String& str) const { return internalCompare(str.c_str()) >= 0; }

	private:
		CORE_DLL String& internalRemoveNullTerminator();
		CORE_DLL String& internalSet(const char* str);
		CORE_DLL String& internalAppend(const char* str);
		CORE_DLL int internalCompare(const char* str) const;

		Core::Vector<char> data_;
	};

	CORE_DLL u32 Hash(u32 input, const String& string);

} // end namespace Core
