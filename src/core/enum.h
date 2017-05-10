#pragma once

#include "core/dll.h"
#include "core/types.h"

namespace Core
{
	/**
	 * Convert string to enum using a specific conversion function.
	 * This is intended to be internal only.
	 */
	CORE_DLL bool EnumFromString(i32& outVal, const char* str, const char* (*convFn)(i32));

	/**
	 * Convert string to enum.
	 */
	template<typename TYPE>
	inline bool EnumFromString(TYPE& outVal, const char* str)
	{
		auto convFn = [](i32 val) { return EnumToString((TYPE)val); };
		i32 intVal = (i32)outVal;
		if(EnumFromString(intVal, str, convFn))
		{
			outVal = (TYPE)intVal;
			return true;
		}
		return false;
	}

} // namespace Core
