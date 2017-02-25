#pragma once

#include "core/dll.h"
#include "core/types.h"

namespace Core
{
	/**
	 * Core hashing algorithms.
	 */
	CORE_DLL u32 HashCRC32(u32 Input, const void* pInData, size_t Size);
	CORE_DLL u32 HashSDBM(u32 Input, const void* pInData, size_t Size);

	/**
	 * Templated hash function to ensure users define their own.
	 */
	template<typename TYPE>
	inline u32 Hash(u32 Input, const TYPE& Data)
	{
		static_assert(false, "Hash function not defined for type. Did you define it in the Core namespace?");
		return 0;
	}

	/**
	 * Specialised hash functions.
	 */
	CORE_DLL u32 Hash(u32 Input, const char* Data);
	inline u32 Hash(u32 Input, u8 Data) { return Input ^ Data; }
	inline u32 Hash(u32 Input, u16 Data) { return Input ^ Data; }
	inline u32 Hash(u32 Input, u32 Data) { return Input ^ Data; }
	inline u32 Hash(u32 Input, u64 Data) { return HashCRC32(Input, &Data, sizeof(Data)); }
	inline u32 Hash(u32 Input, i8 Data) { return Input ^ Data; }
	inline u32 Hash(u32 Input, i16 Data) { return Input ^ Data; }
	inline u32 Hash(u32 Input, i32 Data) { return Input ^ Data; }
	inline u32 Hash(u32 Input, i64 Data) { return HashCRC32(Input, &Data, sizeof(Data)); }

} // namespace Core
