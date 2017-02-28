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

	template<typename TYPE>
	bool Pot(TYPE T)
	{
		return ((T & (T - 1)) == 0) || (T == 1);
	}

	template<typename TYPE, typename TYPE2>
	TYPE PotRoundUp(TYPE Value, TYPE2 RoundUpTo)
	{
		DBG_ASSERT(Pot(RoundUpTo));
		return (Value + (((TYPE)RoundUpTo) - 1)) & ~((TYPE)RoundUpTo - 1);
	}

	template<typename ENUM>
	inline bool ContainsAllFlags(ENUM Value, ENUM Flags)
	{
		static_assert(sizeof(ENUM) <= sizeof(int), "Enum size too large.");
		return ((int)Value & (int)Flags) == (int)Flags;
	}

	inline bool ContainsAllFlags(int Value, int Flags) { return ((int)Value & (int)Flags) == (int)Flags; }

	template<typename ENUM>
	inline bool ContainsAnyFlags(ENUM Value, ENUM Flags)
	{
		static_assert(sizeof(ENUM) <= sizeof(int), "Enum size too large.");
		return ((int)Value & (int)Flags) != 0;
	}

	inline bool ContainsAnyFlags(int Value, int Flags) { return ((int)Value & (int)Flags) != 0; }
} // namespace Core
