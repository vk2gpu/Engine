#pragma once

#include "core/types.h"

namespace Math
{
	template<typename TYPE>
	FORCEINLINE TYPE Lerp(const TYPE A, const TYPE B, f32 T)
	{
		return A + ((B - A) * T);
	}
} // namespace Math
