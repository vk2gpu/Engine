#pragma once

template<typename TYPE>
TYPE Min(TYPE a, TYPE b)
{
	return a < b ? a : b;
}

template<typename TYPE>
TYPE Max(TYPE a, TYPE b)
{
	return a > b ? a : b;
}

template<typename ENUM>
inline bool ContainsAllFlags(ENUM Value, ENUM Flags)
{
	static_assert(sizeof(ENUM) <= sizeof(int), "Enum size too large.");
	return ((int)Value & (int)Flags) == (int)Flags;
}

inline bool ContainsAllFlags(int Value, int Flags)
{
	return ((int)Value & (int)Flags) == (int)Flags;
}

template<typename ENUM>
inline bool ContainsAnyFlags(ENUM Value, ENUM Flags)
{
	static_assert(sizeof(ENUM) <= sizeof(int), "Enum size too large.");
	return ((int)Value & (int)Flags) != 0;
}

inline bool ContainsAnyFlags(int Value, int Flags)
{
	return ((int)Value & (int)Flags) != 0;
}
