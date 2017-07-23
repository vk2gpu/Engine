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
	class CORE_DLL String
	{
	public:
		using storage_type = Core::Vector<char>;
		using index_type = storage_type::index_type;
		using iterator = storage_type::iterator;
		using const_iterator = storage_type::const_iterator;
		static const index_type npos = -1;

		String() {}
		String(const char* str) { internalSet(str); }
		String(const char* begin, const char* end) { internalSet(begin, end); }
		String(const String& str) { internalSet(str.c_str()); }
		String(String&& str) { swap(str); }

		~String() {}

		// Custom interfaces.
		String& Printf(const char* fmt, ...);
		String& Printfv(const char* fmt, va_list argList);
		String& Appendf(const char* fmt, ...);
		String& Appendfv(const char* fmt, va_list argList);

		// STL compatible interfaces.
		void clear() { data_.clear(); }
		const char* c_str() const { return data_.data(); }
		i32 size() const { return data_.size() > 0 ? data_.size() - 1 : 0; }
		char* data() { return data_.data(); }
		const char* data() const { return data_.data(); }

		void reserve(index_type capacity) { data_.reserve(capacity); }
		void swap(String& other) { data_.swap(other.data_); }
		void resize(index_type size)
		{
			data_.resize(size + 1);
			data_.back() = '\0';
		}

		void shrink_to_fit() { data_.shrink_to_fit(); }

		index_type find(const char* str, index_type subPos = 0) const;
		index_type find(const String& str, index_type subPos = 0) const { return find(str.c_str(), subPos); }

		String replace(const char* search, const char* replacement) const;

		iterator begin() { return data_.begin(); }
		const_iterator begin() const { return data_.begin(); }

		iterator end() { return data_.end() - 1; }
		const_iterator end() const { return data_.end() - 1; }

		// cstring versions.
		void append(const char* str) { internalAppend(str); }
		void append(const char* str, index_type subPos, index_type subLen) { internalAppend(str, subPos, subLen); }
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
		String& internalRemoveNullTerminator(bool forceRemove = true);
		String& internalSet(const char* begin, const char* end = nullptr);
		String& internalAppend(const char* str, index_type subPos = 0, index_type subLen = npos);
		int internalCompare(const char* str) const;

		storage_type data_;
	};

	class CORE_DLL StringView
	{
	public:
		using const_iterator = String::const_iterator;
		using index_type = String::index_type;

		StringView(const char* begin = nullptr, const char* end = nullptr)
		    : begin_(begin)
		    , end_(end)
		{
			if(begin_ && !end_)
				end_ = begin_ + strlen(begin_);
		}

		StringView(const String& str)
		    : begin_(str.begin())
		    , end_(str.end())
		{
		}

		~StringView() = default;

		String ToString() const { return String(begin_, end_); }

		const_iterator begin() const { return begin_; }
		const_iterator end() const { return end_; }
		index_type size() const { return (index_type)(end_ - begin_); }

		int compare(const char* str) const { return internalCompare(str, nullptr); }
		int compare(const String& str) const { return internalCompare(str.begin(), str.end()); }
		int compare(const StringView& str) const { return internalCompare(str.begin(), str.end()); }

		bool operator==(const char* str) const { return internalCompare(str, nullptr) == 0; }
		bool operator!=(const char* str) const { return internalCompare(str, nullptr) != 0; }
		bool operator<(const char* str) const { return internalCompare(str, nullptr) < 0; }
		bool operator>(const char* str) const { return internalCompare(str, nullptr) > 0; }
		bool operator<=(const char* str) const { return internalCompare(str, nullptr) <= 0; }
		bool operator>=(const char* str) const { return internalCompare(str, nullptr) >= 0; }

		bool operator==(const String& str) const { return internalCompare(str.begin(), str.end()) == 0; }
		bool operator!=(const String& str) const { return internalCompare(str.begin(), str.end()) != 0; }
		bool operator<(const String& str) const { return internalCompare(str.begin(), str.end()) < 0; }
		bool operator>(const String& str) const { return internalCompare(str.begin(), str.end()) > 0; }
		bool operator<=(const String& str) const { return internalCompare(str.begin(), str.end()) <= 0; }
		bool operator>=(const String& str) const { return internalCompare(str.begin(), str.end()) >= 0; }

		bool operator==(const StringView& str) const { return internalCompare(str.begin(), str.end()) == 0; }
		bool operator!=(const StringView& str) const { return internalCompare(str.begin(), str.end()) != 0; }
		bool operator<(const StringView& str) const { return internalCompare(str.begin(), str.end()) < 0; }
		bool operator>(const StringView& str) const { return internalCompare(str.begin(), str.end()) > 0; }
		bool operator<=(const StringView& str) const { return internalCompare(str.begin(), str.end()) <= 0; }
		bool operator>=(const StringView& str) const { return internalCompare(str.begin(), str.end()) >= 0; }

	private:
		int internalCompare(const char* begin, const char* end) const;

		const char* begin_ = nullptr;
		const char* end_ = nullptr;
	};

	inline bool operator==(const String& str, const StringView& view) { return view == str; }
	inline bool operator!=(const String& str, const StringView& view) { return view != str; }
	inline bool operator<(const String& str, const StringView& view) { return view >= str; }
	inline bool operator>(const String& str, const StringView& view) { return view <= str; }
	inline bool operator<=(const String& str, const StringView& view) { return view > str; }
	inline bool operator>=(const String& str, const StringView& view) { return view < str; }

	CORE_DLL u32 Hash(u32 input, const String& string);
	CORE_DLL u32 Hash(u32 input, const StringView& string);

} // end namespace Core
