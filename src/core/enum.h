#pragma once

#include "core/dll.h"
#include "core/types.h"

namespace Core
{
	/**
	 * This is to allow enum<->string conversion in a reasonably central & standard way
	 * without having a bulky reflection system.
	 * To use it, just specialize for the enums you want, and any systems using this
	 * should work.
	 * Enum->String conversion has no requirements to work correctly.
	 * String->Enum requires enums to have no gaps, all sequential values must
	 * be valid as the last invalid one is used to determine when to stop searching for a value.
	 */
	template<typename TYPE>
	struct Enum
	{
		static const char* ToString(i32 val)
		{
			static_assert(false, "Type doesn't have a specialised conversion implemented.");
			return nullptr;
		}
	};

	/**
	 * Convert enum to string.
	 */
	template<typename TYPE>
	const char* EnumToString(TYPE val)
	{
		return Enum<TYPE>::ToString((i32)val);
	}

	/**
	 * Convert string to enum using a specific conversion function.
	 * This is intended to be internal only.
	 */
	bool CORE_DLL EnumFromString(i32& outVal, const char* str, const char* (*convFn)(i32));

	/**
	 * Convert string to enum.
	 */
	template<typename TYPE>
	bool EnumFromString(TYPE& outVal, const char* str)
	{
		i32 intVal = (i32)outVal;
		if(EnumFromString(intVal, str, Enum<TYPE>::ToString))
		{
			outVal = (TYPE)intVal;
			return true;
		}
		return false;
	}

} // namespace Core
