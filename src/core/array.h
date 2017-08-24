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

		operator ArrayView<TYPE>() const { return ArrayView<TYPE>(data_, size_); }

		TYPE& operator[](index_type Idx)
		{
			DBG_ASSERT_MSG(Idx >= 0 && Idx < SIZE, "Index out of bounds. (index %u, size %u)", Idx, SIZE);
			return Data_[Idx];
		}

		const TYPE& operator[](index_type Idx) const
		{
			DBG_ASSERT_MSG(Idx >= 0 && Idx < SIZE, "Index out of bounds. (%u, size %u)", Idx, SIZE);
			return Data_[Idx];
		}

		void fill(const TYPE& Val)
		{
			for(index_type Idx = 0; Idx < SIZE; ++Idx)
				Data_[Idx] = Val;
		}

		TYPE& front() { return Data_[0]; }
		const TYPE& front() const { return Data_[0]; }
		TYPE& back() { return Data_[SIZE - 1]; }
		const TYPE& back() const { return Data_[SIZE - 1]; }

		iterator begin() { return Data_; }
		const_iterator begin() const { return Data_; }
		iterator end() { return Data_ + SIZE; }
		const_iterator end() const { return Data_ + SIZE; }

		TYPE* data() { return &Data_[0]; }
		const TYPE* data() const { return &Data_[0]; }
		index_type size() const { return SIZE; }

		TYPE Data_[SIZE];
	};
} // namespace Core
