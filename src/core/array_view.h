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

		ArrayView(const value_type& value)
		    : begin_(&value)
		    , end_(begin_ + 1)
		{
			DBG_ASSERT(begin_ == nullptr && end_ == nullptr || (begin_ != nullptr && end_ != nullptr));
		}

		ArrayView(const value_type* begin, const value_type* end)
		    : begin_(begin)
		    , end_(end)
		{
			DBG_ASSERT(begin_ == nullptr && end_ == nullptr || (begin_ != nullptr && end_ != nullptr));
		}

		ArrayView(const value_type* data, index_type size)
		    : begin_(data)
		    , end_(data + size)
		{
		}

		~ArrayView() = default;

		const_iterator begin() const { return begin_; }
		const_iterator end() const { return end_; }
		index_type size() const { return (index_type)(end_ - begin_); }
		const value_type* data() const { return begin_; }

		const value_type& operator[](index_type idx) const
		{
			DBG_ASSERT_MSG(idx >= 0 && idx < size(), "Index out of bounds. (%u, size %u)", idx, size());
			return begin_[idx];
		}

	private:
		const value_type* begin_ = nullptr;
		const value_type* end_ = nullptr;
	};
} // namespace Core
