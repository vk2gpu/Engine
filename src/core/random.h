#pragma once

#include "core/types.h"
#include "core/dll.h"

namespace Core
{
	class CORE_DLL Random
	{
	public:
		Random() = default;
		Random(u32 seed);

		/**
		 * Get a random number.
		 */
		i32 Generate();

	private:
		u32 seed_ = 848256;
	};
} // namespace Core
