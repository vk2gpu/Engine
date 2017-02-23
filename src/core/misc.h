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

template< typename _Enum >
inline bool ContainsAllFlags( _Enum Value, _Enum Flags )
{
	static_assert( sizeof( _Enum ) <= sizeof( int ), "Enum size too large." );
	return ( (int)Value & (int)Flags ) == (int)Flags;
}

inline bool ContainsAllFlags( int Value, int Flags )
{
	return ( (int)Value & (int)Flags ) == (int)Flags;
}

template< typename _Enum >
inline bool ContainsAnyFlags( _Enum Value, _Enum Flags )
{
	static_assert( sizeof( _Enum ) <= sizeof( int ), "Enum size too large." );
	return ( (int)Value & (int)Flags ) != 0;
}

inline bool ContainsAnyFlags( int Value, int Flags )
{
	return ( (int)Value & (int)Flags ) != 0;
}
