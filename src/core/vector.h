#pragma once

#include "core/types.h"
#include "core/debug.h"

namespace Core
{
	/**
	 * Vector of elements.
	 */
	template<typename TYPE>
	class Vector
	{
	public:
		typedef i32 index_type;
		typedef TYPE value_type;
		typedef value_type* iterator;
		typedef const value_type* const_iterator;

		Vector() {}

		Vector(const Vector& Other)
		{
			internalResize(Other.Size_);
			Size_ = Other.Size_;
			for(index_type Idx = 0; Idx < Other.Size_; ++Idx)
				Data_[Idx] = Other.Data_[Idx];
		}

		Vector(Vector&& Other) { swap(Other); }

		~Vector() {}

		Vector& operator=(const Vector& Other)
		{
			internalResize(Other.Size_);
			Size_ = Other.Size_;
			for(index_type Idx = 0; Idx < Other.Size_; ++Idx)
				Data_[Idx] = Other.Data_[Idx];
			return *this;
		}

		Vector& operator=(Vector&& Other)
		{
			swap(Other);
			return *this;
		}

		void swap(Vector& Other)
		{
			std::swap(Data_, Other.Data_);
			std::swap(Size_, Other.Size_);
			std::swap(Capacity_, Other.Capacity_);
		}

		TYPE& operator[](index_type Idx)
		{
			DBG_ASSERT_MSG(Idx >= 0 && Idx < Size_, "Index out of bounds. (index %u, size %u)", Idx, Size_);
			return Data_[Idx];
		}

		const TYPE& operator[](index_type Idx) const
		{
			DBG_ASSERT_MSG(Idx >= 0 && Idx < Size_, "Index out of bounds. (%u, size %u)", Idx, Size_);
			return Data_[Idx];
		}

		void clear() { internalResize(0); }

		void fill(const TYPE& Val)
		{
			for(index_type Idx = 0; Idx < Size_; ++Idx)
				Data_[Idx] = Val;
		}

		iterator erase(iterator It)
		{
			DBG_ASSERT_MSG(It >= begin() && It < end(), "Invalid iterator.");
			for(iterator DstIt = It, SrcIt = It + 1; SrcIt != end(); ++DstIt, ++SrcIt)
				*DstIt = std::move(*SrcIt);
			--Size_;
			return It;
		}

		void push_back(const TYPE& Value)
		{
			if(Capacity_ < (Size_ + 1))
				internalResize(getGrowCapacity(Capacity_));
			Data_[Size_++] = Value;
		}

		void emplace_back(TYPE&& value)
		{
			if(Capacity_ < (Size_ + 1))
				internalResize(getGrowCapacity(Capacity_));
			Data_[Size_++] = std::forward<TYPE>(value);
		}

		void pop_back()
		{
			DBG_ASSERT_MSG(Size_ > 0, "No elements in vector.");
			--Size_;
		}

		void reserve(index_type Capacity)
		{
			if(Capacity_ != Capacity)
				internalResize(Capacity);
		}

		void resize(index_type Size)
		{
			if(Size_ != Size)
				internalResize(Size);
			Size_ = Size;
		}

		void resize(index_type Size, const TYPE& Default)
		{
			if(Size_ != Size)
				internalResize(Size);
			Size_ = Size;
			fill(Default);
		}

		void shrink_to_fit()
		{
			if(Capacity_ > Size_)
				internalResize(Size_);
		}

		TYPE& front()
		{
			DBG_ASSERT_MSG(Size_ > 0, "No elements in vector.");
			return Data_[0];
		}

		const TYPE& front() const
		{
			DBG_ASSERT_MSG(Size_ > 0, "No elements in vector.");
			return Data_[0];
		}

		TYPE& back()
		{
			DBG_ASSERT_MSG(Size_ > 0, "No elements in vector.");
			return Data_[Size_ - 1];
		}

		const TYPE& back() const
		{
			DBG_ASSERT_MSG(Size_ > 0, "No elements in vector.");
			return Data_[Size_ - 1];
		}

		iterator begin() { return Data_; }
		const_iterator begin() const { return Data_; }
		iterator end() { return Data_ + Size_; }
		const_iterator end() const { return Data_ + Size_; }

		TYPE* data() { return &Data_[0]; }
		const TYPE* data() const { return &Data_[0]; }
		index_type size() const { return Size_; }
		index_type capacity() const { return Capacity_; }
		bool empty() const { return Size_ == 0; }

	private:
		static TYPE* alloc(index_type Size)
		{
			TYPE* RetVal = new TYPE[Size];
			DBG_ASSERT_MSG(RetVal != nullptr, "Out of memory allocating %u elements.", Size);
			return RetVal;
		}
		static void dealloc(TYPE* Data) { delete[] Data; }
		static index_type getGrowCapacity(index_type CurrCapacity)
		{
			return CurrCapacity ? (CurrCapacity + CurrCapacity / 2) : 16;
		}
		void internalResize(index_type NewCapacity)
		{
			index_type CopySize = NewCapacity < Size_ ? NewCapacity : Size_;
			TYPE* NewData = nullptr;
			if(NewCapacity > 0)
			{
				NewData = alloc(NewCapacity);
				for(index_type Idx = 0; Idx < CopySize; ++Idx)
					NewData[Idx] = std::move(Data_[Idx]);
			}
			dealloc(Data_);
			Data_ = NewData;
			Size_ = CopySize;
			Capacity_ = NewCapacity;
		}


	private:
		TYPE* Data_ = nullptr;
		index_type Size_ = 0;
		index_type Capacity_ = 0;
	};
} // namespace Core
