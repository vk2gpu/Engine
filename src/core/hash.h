#pragma once

#include "core/types.h"

//////////////////////////////////////////////////////////////////////////
// Base hash functions
u32 HashCRC32(u32 Input, const void* pInData, size_t Size);
u32 HashSDBM(u32 Input, const void* pInData, size_t Size);

//////////////////////////////////////////////////////////////////////////
// String hash function
u32 Hash(u32 Input, const char* Data);

//////////////////////////////////////////////////////////////////////////
// Generic hash function
template<typename TYPE>
inline u32 Hash(u32 Input, const TYPE& Data)
{
	return HashCRC32(Input, &Data, sizeof(Data));
}

inline u32 Hash(u32 Input, u8 Data)
{
	return Input ^ Data;
}

inline u32 Hash(u32 Input, u16 Data)
{
	return Input ^ Data;
}

inline u32 Hash(u32 Input, u32 Data)
{
	return Input ^ Data;
}

inline u32 Hash(u32 Input, u64 Data)
{
	return Input ^ (u32)Data ^ (Data >> 32);
}


