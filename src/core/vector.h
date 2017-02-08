#pragma once

#include "core/types.h"
#include "core/debug.h"

//////////////////////////////////////////////////////////////////////////
// Definition
template<typename TYPE>
class Vector
{
public:
	typedef TYPE value_type;
    typedef value_type* iterator;
    typedef const value_type* const_iterator;

	Vector();
	Vector(const Vector& Other);
	Vector(Vector&& Other);
	~Vector();
	Vector& operator = (const Vector& Other);
	Vector& operator = (Vector&& Other);
	void swap(Vector& Other);

	TYPE& operator [](size_t Idx);
	const TYPE& operator [](size_t Idx) const;
	void fill(const TYPE& Val);

	void clear();
	void push_back(const TYPE& Value);
	void pop_back();
	iterator erase(iterator It)
	{
		DBG_ASSERT_MSG(It >= begin() && It < end(), "Invalid iterator.");
		for(iterator DstIt = It, SrcIt = It + 1; SrcIt != end(); ++DstIt, ++SrcIt)
			*DstIt = std::move(*SrcIt);
		--Size_;
		return It;
	}

	void reserve(size_t Capacity);
	void resize(size_t Size);
	void resize(size_t Size, const TYPE& Default);
	void shrink_to_fit();

	TYPE& front();
	const TYPE& front() const;
	TYPE& back();
	const TYPE& back() const;

	iterator begin() { return Data_; } 
    const_iterator begin() const { return Data_; }
    iterator end() { return Data_ + Size_; }
	const_iterator end() const { return Data_ + Size_; }

	TYPE* data() { return &Data_[0]; }
	const TYPE* data() const { return &Data_[0]; }
	size_t size() const{ return Size_; }
    size_t capacity() const{ return Capacity_; }
	bool empty() const{ return Size_ == 0; }

private:
	static TYPE* alloc(size_t Size)
	{
		TYPE* RetVal = new TYPE[Size];
		DBG_ASSERT_MSG(RetVal != nullptr, "Out of memory allocating %u elements.", Size);
		return RetVal;
	}
	static void dealloc(TYPE* Data)
	{
		delete [] Data;
	}
	static size_t getGrowCapacity(size_t CurrCapacity)
	{
		return CurrCapacity ? (CurrCapacity + CurrCapacity / 2) : 16;
	}
	void internalResize(size_t NewCapacity)
	{
		size_t CopySize = NewCapacity < Size_ ? NewCapacity : Size_; 
		TYPE* NewData = nullptr;
		if(NewCapacity > 0)
		{
			NewData = alloc(NewCapacity);
			for(size_t Idx = 0; Idx < CopySize; ++Idx)
				NewData[Idx] = std::move(Data_[Idx]);
		}
		dealloc(Data_);
		Data_ = NewData;
		Size_ = CopySize;
		Capacity_ = NewCapacity;
	}


private:
	TYPE* Data_ = nullptr;
	size_t Size_ = 0;
	size_t Capacity_ = 0;
};

//////////////////////////////////////////////////////////////////////////
// Implementation
template<typename TYPE>
Vector<TYPE>::Vector()
{
}

template<typename TYPE>
Vector<TYPE>::Vector(const Vector& Other)
{
	internalResize(Other.Size_);
	Size_ = Other.Size_;
	for(size_t Idx = 0; Idx < Other.Size_; ++Idx)
		Data_[Idx] = Other.Data_[Idx];
}

template<typename TYPE>
Vector<TYPE>::Vector(Vector&& Other)
	: Vector()
{
	swap(Other);
}

template<typename TYPE>
Vector<TYPE>::~Vector()
{
}

template<typename TYPE>
Vector<TYPE>& Vector<TYPE>::operator = (const Vector& Other)
{
	internalResize(Other.Size_);
	Size_ = Other.Size_;
	for(size_t Idx = 0; Idx < Other.Size_; ++Idx)
		Data_[Idx] = Other.Data_[Idx];
	return *this;
}

template<typename TYPE>
Vector<TYPE>& Vector<TYPE>::operator = (Vector&& Other)
{
	swap(Other);
	return *this;
}

template<typename TYPE>
void Vector<TYPE>::swap(Vector& Other)
{
	std::swap(Data_, Other.Data_);
	std::swap(Size_, Other.Size_);
	std::swap(Capacity_, Other.Capacity_);	
}

template<typename TYPE>
TYPE& Vector<TYPE>::operator [](size_t Idx)
{
	DBG_ASSERT_MSG(Idx < Size_, "Index out of bounds. (index %u, size %u)", Idx, Size_);
	return Data_[Idx];
}

template<typename TYPE>
const TYPE& Vector<TYPE>::operator [](size_t Idx) const
{
	DBG_ASSERT_MSG(Idx < Size_, "Index out of bounds. (%u, size %u)", Idx, Size_);
	return Data_[Idx];
}

template<typename TYPE>
void Vector<TYPE>::clear()
{
	internalResize(0);
}

template<typename TYPE>
void Vector<TYPE>::fill(const TYPE& Val)
{
	for(size_t Idx = 0; Idx < Size_; ++Idx)
		Data_[Idx] = Val;
}

template<typename TYPE>
void Vector<TYPE>::push_back(const TYPE& Value)
{
	if(Capacity_ < (Size_ + 1))
		internalResize(getGrowCapacity(Capacity_));
	Data_[Size_++] = Value;
}

template<typename TYPE>
void Vector<TYPE>::pop_back()
{
	DBG_ASSERT_MSG(Size_ > 0, "No elements in vector.");
	--Size_;
}

template<typename TYPE>
void Vector<TYPE>::reserve(size_t Capacity)
{
	if(Capacity_ != Capacity)
		internalResize(Capacity);
}

template<typename TYPE>
void Vector<TYPE>::resize(size_t Size)
{
	if(Size_ != Size)
		internalResize(Size);
	Size_ = Size;
}

template<typename TYPE>
void Vector<TYPE>::resize(size_t Size, const TYPE& Default)
{
	if(Size_ != Size)
		internalResize(Size);
	Size_ = Size;
	fill(Default);
}

template<typename TYPE>
void Vector<TYPE>::shrink_to_fit()
{
	if(Capacity_ > Size_)
		internalResize(Size_);
}

template<typename TYPE>
TYPE& Vector<TYPE>::front()
{
	DBG_ASSERT_MSG(Size_ > 0, "No elements in vector.");
	return Data_[0];
}

template<typename TYPE>
const TYPE& Vector<TYPE>::front() const
{
	DBG_ASSERT_MSG(Size_ > 0, "No elements in vector.");
	return Data_[0];
}

template<typename TYPE>
TYPE& Vector<TYPE>::back()
{
	DBG_ASSERT_MSG(Size_ > 0, "No elements in vector.");
	return Data_[Size_ - 1];
}

template<typename TYPE>
const TYPE& Vector<TYPE>::back() const
{
	DBG_ASSERT_MSG(Size_ > 0, "No elements in vector.");
	return Data_[Size_ - 1];
}

