#pragma once

#include "core/types.h"
#include "core/dll.h"

namespace Core
{
	/**
	 * High precision timer.
	 */
	class CORE_DLL Timer
	{
	public:
		Timer() = default;

		/**
		 * Mark point of reference.
		 */
		void Mark() { start_ = GetAbsoluteTime(); }

		/**
		 * @return seconds since last call to Mark.
		 */
		f64 GetTime() { return GetAbsoluteTime() - start_; }

		/**
		 * @return absolute time in seconds.
		 */
		static f64 GetAbsoluteTime();

	private:
		f64 start_ = 0;
	};
} // namespace Core
