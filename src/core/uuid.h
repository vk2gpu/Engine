#pragma once

#include "core/types.h"
#include "core/dll.h"

namespace Core
{
	class CORE_DLL UUID final
	{
	public:
		UUID() = default;

		/**
		 * Generate UUID (v.4) from a random number generator.
		 */
		template<class RANDOM>
		UUID(RANDOM& random, u8 variant = 0)
		{
			SetInternal(random.Generate(), random.Generate(), random.Generate(), random.Generate(), 4, variant);
		}

		/**
		 * Generate a UUID (v.5) from a string.
		 */
		UUID(const char* string, u8 variant = 0);

		/**
		 * Convert to string.
		 * Pointer passed in MUST be at least 37 bytes long.
		 */
		void AsString(char* outStr);

		bool operator==(const UUID& other) const
		{
			return data64_[0] == other.data64_[0] && data64_[1] == other.data64_[1];
		}

		bool operator!=(const UUID& other) const
		{
			return data64_[0] != other.data64_[0] || data64_[1] != other.data64_[1];
		}

	private:
		void SetInternal(u32 data0, u32 data1, u32 data2, u32 data3, u8 version, u8 variant);

		union CORE_DLL
		{
			;
			u8 data8_[16] = {0};
			u32 data32_[4];
			u64 data64_[2];
		};
	};

	/**
	 * Custom hash function.
	 */
	CORE_DLL u32 Hash(u32 input, const UUID& data);


} // namespace Core
