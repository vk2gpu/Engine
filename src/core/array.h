#pragma once

#include "core/types.h"
#include "core/debug.h"

//////////////////////////////////////////////////////////////////////////
// Definition
template<typename TYPE, size_t SIZE>
class Array
{
public:
	typedef TYPE value_type;
    typedef value_type* iterator;
    typedef const value_type* const_iterator;

	Array();
	Array(const Array& Other);
	Array(Array&& Other);
	~Array();
	Array& operator = (const Array& Other);
	Array& operator = (Array&& Other);

	TYPE& operator [](size_t Idx);
	const TYPE& operator [](size_t Idx) const;
	void fill(const TYPE& Val);

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
	size_t size() const{ return SIZE; }

private:
	TYPE Data_[SIZE];
};

//////////////////////////////////////////////////////////////////////////
// Implementation
template<typename TYPE, size_t SIZE>
Array<TYPE, SIZE>::Array()
{
	for(size_t Idx = 0; Idx < SIZE; ++Idx)
		Data_[Idx] = TYPE();
}

template<typename TYPE, size_t SIZE>
Array<TYPE, SIZE>::Array(const Array& Other)
{
	for(size_t Idx = 0; Idx < SIZE; ++Idx)
		Data_[Idx] = Other.Data_[Idx];
}

template<typename TYPE, size_t SIZE>
Array<TYPE, SIZE>::Array(Array&& Other)
	: Array()
{
	for(size_t Idx = 0; Idx < SIZE; ++Idx)
		std::swap(Data_[Idx], Other.Data_[Idx]);
}

template<typename TYPE, size_t SIZE>
Array<TYPE, SIZE>::~Array()
{
}

template<typename TYPE, size_t SIZE>
Array<TYPE, SIZE>& Array<TYPE, SIZE>::operator = (const Array& Other)
{
	for(size_t Idx = 0; Idx < SIZE; ++Idx)
		Data_[Idx] = Other.Data_[Idx];
	return *this;
}

template<typename TYPE, size_t SIZE>
Array<TYPE, SIZE>& Array<TYPE, SIZE>::operator = (Array&& Other)
{
	for(size_t Idx = 0; Idx < SIZE; ++Idx)
		std::swap(Data_[Idx], Other.Data_[Idx]);
	return *this;
}

template<typename TYPE, size_t SIZE>
TYPE& Array<TYPE, SIZE>::operator [](size_t Idx)
{
	DBG_ASSERT_MSG(Idx < SIZE, "Index out of bounds. (index %u, size %u)", Idx, SIZE);
	return Data_[Idx];
}

template<typename TYPE, size_t SIZE>
const TYPE& Array<TYPE, SIZE>::operator [](size_t Idx) const
{
	DBG_ASSERT_MSG(Idx < SIZE, "Index out of bounds. (%u, size %u)", Idx, SIZE);
	return Data_[Idx];
}

template<typename TYPE, size_t SIZE>
void Array<TYPE, SIZE>::fill(const TYPE& Val)
{
	for(size_t Idx = 0; Idx < SIZE; ++Idx)
		Data_[Idx] = Val;
}
