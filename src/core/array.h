#pragma once

#include "core/types.h"
#include "core/array_view.h"
#include "core/debug.h"

namespace Core
{
	/**
	 * Fixed size array.
	 */
	template<typename TYPE, i32 SIZE>
	class Array
	{
	public:
		typedef decltype(SIZE) index_type;
		typedef TYPE value_type;
		typedef value_type* iterator;
		typedef const value_type* const_iterator;

		operator ArrayView<TYPE>() { return ArrayView<TYPE>(data_, SIZE); }
		operator ArrayView<const TYPE>() const { return ArrayView<const TYPE>(data_, SIZE); }

		TYPE& operator[](index_type Idx)
		{
			DBG_ASSERT_MSG(Idx >= 0 && Idx < SIZE, "Index out of bounds. (index %u, size %u)", Idx, SIZE);
			return data_[Idx];
		}

		const TYPE& operator[](index_type Idx) const
		{
			DBG_ASSERT_MSG(Idx >= 0 && Idx < SIZE, "Index out of bounds. (%u, size %u)", Idx, SIZE);
			return data_[Idx];
		}

		void fill(const TYPE& Val)
		{
			for(index_type Idx = 0; Idx < SIZE; ++Idx)
				data_[Idx] = Val;
		}

		TYPE& front() { return data_[0]; }
		const TYPE& front() const { return data_[0]; }
		TYPE& back() { return data_[SIZE - 1]; }
		const TYPE& back() const { return data_[SIZE - 1]; }

		iterator begin() { return data_; }
		const_iterator begin() const { return data_; }
		iterator end() { return data_ + SIZE; }
		const_iterator end() const { return data_ + SIZE; }

		TYPE* data() { return &data_[0]; }
		const TYPE* data() const { return &data_[0]; }
		index_type size() const { return SIZE; }

		TYPE data_[SIZE];
	};
} // namespace Core
