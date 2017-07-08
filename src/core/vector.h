#pragma once

#include "core/types.h"
#include "core/allocator.h"
#include "core/debug.h"

#include <utility>

namespace Core
{
	/**
	 * Vector of elements.
	 */
	template<typename TYPE, typename ALLOCATOR = Allocator>
	class Vector
	{
	public:
		using index_type = i32; // TODO: Template arg?
		using value_type = TYPE;
		using iterator = value_type*;
		using const_iterator = const value_type*;

		Vector() = default;

		Vector(const Vector& other)
		{
			internalResize(other.size_);
			size_ = other.size_;
			for(index_type idx = 0; idx < other.size_; ++idx)
				new(data_ + idx) TYPE(other.data_[idx]);
		}

		Vector(Vector&& other) { swap(other); }
		Vector(index_type size) { resize(size); }

		~Vector() { internalResize(0); }

		Vector& operator=(const Vector& other)
		{
			if(other.size_ != size_)
				internalResize(other.size_);

			// destruct
			destruct(data_, data_ + size_);

			// reconstruct
			size_ = other.size_;
			for(index_type idx = 0; idx < other.size_; ++idx)
				new(data_ + idx) TYPE(other.data_[idx]);
			return *this;
		}

		Vector& operator=(Vector&& other)
		{
			swap(other);
			return *this;
		}

		void swap(Vector& other)
		{
			std::swap(data_, other.data_);
			std::swap(size_, other.size_);
			std::swap(capacity_, other.capacity_);
		}

		TYPE& operator[](index_type idx)
		{
			DBG_ASSERT_MSG(idx >= 0 && idx < size_, "Index out of bounds. (index %u, size %u)", idx, size_);
			return data_[idx];
		}

		const TYPE& operator[](index_type idx) const
		{
			DBG_ASSERT_MSG(idx >= 0 && idx < size_, "Index out of bounds. (%u, size %u)", idx, size_);
			return data_[idx];
		}

		void clear()
		{
			destruct(data_, data_ + size_);
			size_ = 0;
		}

		void fill(const TYPE& value)
		{
			destruct(data_, data_ + size_);
			for(index_type idx = 0; idx < size_; ++idx)
				new(data_ + idx) TYPE(value);
		}

		iterator erase(iterator it)
		{
			DBG_ASSERT_MSG(it >= begin() && it < end(), "Invalid iterator.");
			for(iterator dstIt = it, srcIt = it + 1; srcIt != end(); ++dstIt, ++srcIt)
				*dstIt = std::move(*srcIt);
			--size_;
			destruct(data_ + size_);
			return it;
		}

		iterator push_back(const TYPE& value)
		{
			if(capacity_ < (size_ + 1))
				internalResize(getGrowCapacity(capacity_));
			new(data_ + size_) TYPE(value);
			return (data_ + size_++);
		}

		iterator push_back(TYPE&& value)
		{
			if(capacity_ < (size_ + 1))
				internalResize(getGrowCapacity(capacity_));
			new(data_ + size_) TYPE(std::forward<TYPE>(value));
			return (data_ + size_++);
		}

		template<class... VAL_TYPE>
		iterator emplace_back(VAL_TYPE&&... value)
		{
			if(capacity_ < (size_ + 1))
				internalResize(getGrowCapacity(capacity_));
			new(data_ + size_) TYPE(std::forward<VAL_TYPE>(value)...);
			return (data_ + size_++);
		}

		iterator insert(const_iterator begin, const_iterator end)
		{
			i32 numValues = (i32)(end - begin);
			if(capacity_ < (size_ + numValues))
				internalResize(size_ + numValues);
			for(const_iterator it = begin; it != end; ++it)
			{
				new(data_ + size_) TYPE(*it);
				++size_;
			}
			return (data_ + size_ - 1);
		}

		void pop_back()
		{
			DBG_ASSERT_MSG(size_ > 0, "No elements in vector.");
			--size_;
			destruct(data_ + size_);
		}

		void reserve(index_type capacity)
		{
			if(capacity_ < capacity)
				internalResize(capacity);
		}

		void resize(index_type size)
		{
			if(size_ != size)
				internalResize(size);
			for(index_type idx = size_; idx < size; ++idx)
				new(data_ + idx) TYPE();
			size_ = size;
		}

		void resize(index_type size, const TYPE& default)
		{
			if(size_ != size)
				internalResize(size);
			for(index_type idx = size_; idx < size; ++idx)
				new(data_ + idx) TYPE(default);
			size_ = size;
		}

		void shrink_to_fit()
		{
			if(capacity_ > size_)
				internalResize(size_);
		}

		TYPE& front()
		{
			DBG_ASSERT_MSG(size_ > 0, "No elements in vector.");
			return data_[0];
		}

		const TYPE& front() const
		{
			DBG_ASSERT_MSG(size_ > 0, "No elements in vector.");
			return data_[0];
		}

		TYPE& back()
		{
			DBG_ASSERT_MSG(size_ > 0, "No elements in vector.");
			return data_[size_ - 1];
		}

		const TYPE& back() const
		{
			DBG_ASSERT_MSG(size_ > 0, "No elements in vector.");
			return data_[size_ - 1];
		}

		iterator begin() noexcept { return data_; }
		const_iterator begin() const noexcept { return data_; }
		iterator end() noexcept { return data_ + size_; }
		const_iterator end() const noexcept { return data_ + size_; }

		TYPE* data() noexcept { return &data_[0]; }
		const TYPE* data() const noexcept { return &data_[0]; }
		index_type size() const noexcept { return size_; }
		index_type capacity() const noexcept { return capacity_; }
		bool empty() const noexcept { return size_ == 0; }

	private:
		static index_type getGrowCapacity(index_type CurrCapacity)
		{
			return CurrCapacity ? (CurrCapacity + CurrCapacity / 2) : 16;
		}

		static iterator uninitialized_move(iterator first, iterator last, iterator dest)
		{
			for(; first != last; ++dest, ++first)
				new(dest) TYPE(std::move(*first));
			return dest;
		}

		static void destruct(iterator first) { first->~TYPE(); }

		static void destruct(iterator first, iterator last)
		{
			for(; first != last; ++first)
				destruct(first);
		}

		void internalResize(index_type newCapacity)
		{
			index_type copySize = newCapacity < size_ ? newCapacity : size_;
			TYPE* newData = nullptr;
			if(newCapacity > 0)
			{
				newData = static_cast<TYPE*>(allocator_.allocate(newCapacity, sizeof(TYPE)));
				DBG_ASSERT_MSG(newData, "Unable to allocate for resize.");
				uninitialized_move(data_, data_ + copySize, newData);
				destruct(data_, data_ + copySize);
			}

			// destruct trailing elements.
			if(copySize < size_)
				destruct(data_ + copySize, data_ + size());

			allocator_.deallocate(data_, capacity_, sizeof(TYPE));

			data_ = newData;
			size_ = copySize;
			capacity_ = newCapacity;
		}


	private:
		TYPE* data_ = nullptr;
		index_type size_ = 0;
		index_type capacity_ = 0;

		ALLOCATOR allocator_;
	};
} // namespace Core
