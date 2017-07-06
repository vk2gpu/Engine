#pragma once

#include "core/types.h"
#include "core/debug.h"

#include <utility>

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
			if(Other.size_ != size_)
				internalResize(Other.size_);
			size_ = Other.size_;
			for(index_type Idx = 0; Idx < Other.size_; ++Idx)
				Data_[Idx] = Other.Data_[Idx];
		}

		Vector(Vector&& Other) { swap(Other); }

		~Vector() { internalResize(0); }

		Vector& operator=(const Vector& Other)
		{
			if(Other.size_ != size_)
				internalResize(Other.size_);
			size_ = Other.size_;
			for(index_type Idx = 0; Idx < Other.size_; ++Idx)
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
			std::swap(size_, Other.size_);
			std::swap(capacity_, Other.capacity_);
		}

		TYPE& operator[](index_type Idx)
		{
			DBG_ASSERT_MSG(Idx >= 0 && Idx < size_, "Index out of bounds. (index %u, size %u)", Idx, size_);
			return Data_[Idx];
		}

		const TYPE& operator[](index_type Idx) const
		{
			DBG_ASSERT_MSG(Idx >= 0 && Idx < size_, "Index out of bounds. (%u, size %u)", Idx, size_);
			return Data_[Idx];
		}

		void clear() { size_ = 0; }

		void fill(const TYPE& Val)
		{
			for(index_type Idx = 0; Idx < size_; ++Idx)
				Data_[Idx] = Val;
		}

		iterator erase(iterator It)
		{
			DBG_ASSERT_MSG(It >= begin() && It < end(), "Invalid iterator.");
			for(iterator DstIt = It, SrcIt = It + 1; SrcIt != end(); ++DstIt, ++SrcIt)
				*DstIt = std::move(*SrcIt);
			--size_;
			return It;
		}

		void push_back(const TYPE& Value)
		{
			if(capacity_ < (size_ + 1))
				internalResize(getGrowCapacity(capacity_));
			Data_[size_] = Value;
			++size_;
		}

		template<class... VAL_TYPE>
		void emplace_back(VAL_TYPE&&... value)
		{
			if(capacity_ < (size_ + 1))
				internalResize(getGrowCapacity(capacity_));
			auto& ref = Data_[size_];
			new(&ref) TYPE(std::forward<VAL_TYPE>(value)...);
			++size_;
		}

		void insert(const_iterator Begin, const_iterator End)
		{
			i32 NumValues = (i32)(End - Begin);
			if(capacity_ < (size_ + NumValues))
				internalResize(size_ + NumValues);
			for(const_iterator It = Begin; It != End; ++It)
			{
				Data_[size_++] = *It;
			}
		}

		void pop_back()
		{
			DBG_ASSERT_MSG(size_ > 0, "No elements in vector.");
			--size_;
		}

		void reserve(index_type Capacity)
		{
			if(capacity_ < Capacity)
				internalResize(Capacity);
		}

		void resize(index_type Size)
		{
			if(size_ != Size)
				internalResize(Size);
			size_ = Size;
		}

		void resize(index_type Size, const TYPE& Default)
		{
			i32 OldSize = size_;
			if(size_ != Size)
				internalResize(Size);
			size_ = Size;
			for(index_type Idx = OldSize; Idx < size_; ++Idx)
				Data_[Idx] = Default;
		}

		void shrink_to_fit()
		{
			if(capacity_ > size_)
				internalResize(size_);
		}

		TYPE& front()
		{
			DBG_ASSERT_MSG(size_ > 0, "No elements in vector.");
			return Data_[0];
		}

		const TYPE& front() const
		{
			DBG_ASSERT_MSG(size_ > 0, "No elements in vector.");
			return Data_[0];
		}

		TYPE& back()
		{
			DBG_ASSERT_MSG(size_ > 0, "No elements in vector.");
			return Data_[size_ - 1];
		}

		const TYPE& back() const
		{
			DBG_ASSERT_MSG(size_ > 0, "No elements in vector.");
			return Data_[size_ - 1];
		}

		iterator begin() { return Data_; }
		const_iterator begin() const { return Data_; }
		iterator end() { return Data_ + size_; }
		const_iterator end() const { return Data_ + size_; }

		TYPE* data() { return &Data_[0]; }
		const TYPE* data() const { return &Data_[0]; }
		index_type size() const { return size_; }
		index_type capacity() const { return capacity_; }
		bool empty() const { return size_ == 0; }

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
			index_type CopySize = NewCapacity < size_ ? NewCapacity : size_;
			TYPE* NewData = nullptr;
			if(NewCapacity > 0)
			{
				NewData = alloc(NewCapacity);
				for(index_type Idx = 0; Idx < CopySize; ++Idx)
					NewData[Idx] = std::move(Data_[Idx]);
			}
			dealloc(Data_);
			Data_ = NewData;
			size_ = CopySize;
			capacity_ = NewCapacity;
		}


	private:
		TYPE* Data_ = nullptr;
		index_type size_ = 0;
		index_type capacity_ = 0;
	};
} // namespace Core
