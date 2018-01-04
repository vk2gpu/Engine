#pragma once

#include "core/debug.h"

namespace Core
{
	template<typename TYPE>
	class ArrayView
	{
	public:
		using index_type = i32; // TODO: Template arg?
		using value_type = TYPE;
		using iterator = value_type*;
		using const_iterator = const value_type*;

		ArrayView() = default;

		ArrayView(const ArrayView& other)
		    : begin_(other.begin_)
		    , end_(other.end_)
		{
		}

		template<typename OTHER>
		ArrayView(const ArrayView<OTHER>& other)
		    : begin_(other.begin_)
		    , end_(other.end_)
		{
		}

		ArrayView(value_type& value)
		    : begin_(&value)
		    , end_(begin_ + 1)
		{
			DBG_ASSERT(begin_ == nullptr && end_ == nullptr || (begin_ != nullptr && end_ != nullptr));
		}

		ArrayView(value_type* begin, value_type* end)
		    : begin_(begin)
		    , end_(end)
		{
			DBG_ASSERT(begin_ == nullptr && end_ == nullptr || (begin_ != nullptr && end_ != nullptr));
		}

		ArrayView(value_type* data, index_type size)
		    : begin_(data)
		    , end_(data + size)
		{
		}

		template<i32 SIZE>
		ArrayView(value_type (&arr)[SIZE])
		    : begin_(&arr[0])
		    , end_(&arr[0] + SIZE)

		{
		}

		~ArrayView() = default;

		iterator begin() { return begin_; }
		iterator end() { return end_; }
		const_iterator begin() const { return begin_; }
		const_iterator end() const { return end_; }

		index_type size() const { return (index_type)(end_ - begin_); }
		value_type* data() const { return begin_; }

		explicit operator bool() const { return !!begin_; }

		value_type& operator[](index_type idx)
		{
			DBG_ASSERT_MSG(idx >= 0 && idx < size(), "Index out of bounds. (%u, size %u)", idx, size());
			return begin_[idx];
		}

		const value_type& operator[](index_type idx) const
		{
			DBG_ASSERT_MSG(idx >= 0 && idx < size(), "Index out of bounds. (%u, size %u)", idx, size());
			return begin_[idx];
		}

	private:
		value_type* begin_ = nullptr;
		value_type* end_ = nullptr;
	};
} // namespace Core
