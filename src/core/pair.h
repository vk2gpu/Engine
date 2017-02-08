#pragma once

#include "core/types.h"

//////////////////////////////////////////////////////////////////////////
// Definition
template<typename FIRST_TYPE, typename SECOND_TYPE>
class Pair
{
public:
	Pair() {}
	Pair(const FIRST_TYPE& First, const SECOND_TYPE& Second)
		: First_(First)
		, Second_(Second)
	{}

	FIRST_TYPE First_;
	SECOND_TYPE Second_;
};
