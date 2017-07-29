#pragma once

#include "core/dll.h"
#include "core/types.h"
#include "core/debug.h"

namespace Core
{
	template<typename TYPE, typename TYPE2>
	TYPE Min(TYPE a, TYPE2 b)
	{
		return a < (TYPE)b ? a : (TYPE)b;
	}

	template<typename TYPE, typename TYPE2>
	TYPE Max(TYPE a, TYPE2 b)
	{
		return a > (TYPE)b ? a : (TYPE)b;
	}

	template<typename TYPE, typename TYPE2, typename TYPE3>
	TYPE Clamp(TYPE v, TYPE2 min, TYPE3 max)
	{
		return Max(min, Min(v, max));
	}

	template<typename TYPE>
	bool Pot(TYPE T)
	{
		return ((T & (T - 1)) == 0) || (T == 1);
	}

	template<typename TYPE, typename TYPE2>
	TYPE PotRoundUp(TYPE value, TYPE2 RoundUpTo)
	{
		DBG_ASSERT(Pot(RoundUpTo));
		return (value + (((TYPE)RoundUpTo) - 1)) & ~((TYPE)RoundUpTo - 1);
	}

	inline i32 BitsSet(u32 value)
	{
		value = (value & 0x55555555U) + ((value & 0xAAAAAAAAU) >> 1);
		value = (value & 0x33333333U) + ((value & 0xCCCCCCCCU) >> 2);
		value = (value & 0x0F0F0F0FU) + ((value & 0xF0F0F0F0U) >> 4);
		value = (value & 0x00FF00FFU) + ((value & 0xFF00FF00U) >> 8);
		return (i32)(value & 0x0000FFFFU) + ((value & 0xFFFF0000U) >> 16);
	}

	template<typename ENUM>
	inline bool ContainsAllFlags(ENUM value, ENUM Flags)
	{
		static_assert(sizeof(ENUM) <= sizeof(int), "Enum size too large.");
		return ((int)value & (int)Flags) == (int)Flags;
	}

	inline bool ContainsAllFlags(int value, int Flags) { return ((int)value & (int)Flags) == (int)Flags; }

	template<typename ENUM>
	inline bool ContainsAnyFlags(ENUM value, ENUM Flags)
	{
		static_assert(sizeof(ENUM) <= sizeof(int), "Enum size too large.");
		return ((int)value & (int)Flags) != 0;
	}

	inline bool ContainsAnyFlags(int value, int Flags) { return ((int)value & (int)Flags) != 0; }
} // namespace Core
