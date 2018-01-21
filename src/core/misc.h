#pragma once

#include "core/dll.h"
#include "core/types.h"
#include "core/debug.h"

namespace Core
{
	template<typename TYPE, typename TYPE2>
	constexpr TYPE Min(TYPE a, TYPE2 b)
	{
		return a < (TYPE)b ? a : (TYPE)b;
	}

	template<typename TYPE, typename TYPE2>
	constexpr TYPE Max(TYPE a, TYPE2 b)
	{
		return a > (TYPE)b ? a : (TYPE)b;
	}

	template<typename TYPE, typename TYPE2, typename TYPE3>
	constexpr TYPE Clamp(TYPE v, TYPE2 min, TYPE3 max)
	{
		return (TYPE)Max(min, Min(v, max));
	}

	template<typename TYPE>
	constexpr bool Pot(TYPE T)
	{
		return ((T & (T - 1)) == 0) || (T == 1);
	}

	template<typename TYPE, typename TYPE2>
	TYPE PotRoundUp(TYPE value, TYPE2 roundUpTo)
	{
		DBG_ASSERT(Pot(roundUpTo));
		return (value + (((TYPE)roundUpTo) - 1)) & ~((TYPE)roundUpTo - 1);
	}

	template<typename TYPE, typename TYPE2>
	TYPE PotRoundDown(TYPE value, TYPE2 roundDownTo)
	{
		DBG_ASSERT(Pot(roundDownTo));
		return value & ~((TYPE)roundDownTo - 1);
	}

	constexpr inline i32 BitsSet(u32 value)
	{
		value = (value & 0x55555555U) + ((value & 0xAAAAAAAAU) >> 1);
		value = (value & 0x33333333U) + ((value & 0xCCCCCCCCU) >> 2);
		value = (value & 0x0F0F0F0FU) + ((value & 0xF0F0F0F0U) >> 4);
		value = (value & 0x00FF00FFU) + ((value & 0xFF00FF00U) >> 8);
		return (i32)(value & 0x0000FFFFU) + ((value & 0xFFFF0000U) >> 16);
	}

	template<typename ENUM>
	constexpr inline bool ContainsAllFlags(ENUM value, ENUM Flags)
	{
		static_assert(sizeof(ENUM) <= sizeof(int), "Enum size too large.");
		return ((int)value & (int)Flags) == (int)Flags;
	}

	constexpr inline bool ContainsAllFlags(int value, int Flags) { return ((int)value & (int)Flags) == (int)Flags; }

	template<typename ENUM>
	constexpr inline bool ContainsAnyFlags(ENUM value, ENUM Flags)
	{
		static_assert(sizeof(ENUM) <= sizeof(int), "Enum size too large.");
		return ((int)value & (int)Flags) != 0;
	}

	constexpr inline bool ContainsAnyFlags(int value, int Flags) { return ((int)value & (int)Flags) != 0; }

	CORE_DLL_INLINE i32 CountLeadingZeros(u32 mask);
	CORE_DLL_INLINE i32 CountLeadingZeros(u64 mask);
} // namespace Core
#if CODE_INLINE
#include "core/private/misc.inl"
#endif
