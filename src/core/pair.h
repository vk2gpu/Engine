#pragma once

#include "core/types.h"

namespace Core
{
	/**
	 * Pair of values.
	 */
	template<typename FIRST_TYPE, typename SECOND_TYPE>
	class Pair
	{
	public:
		Pair() {}
		Pair(const FIRST_TYPE& _first, const SECOND_TYPE& _second)
		    : first(_first)
		    , second(_second)
		{
		}

		FIRST_TYPE first;
		SECOND_TYPE second;
	};
} // namespace Core
