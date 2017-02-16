#pragma once

#include "core/types.h"

//////////////////////////////////////////////////////////////////////////
// Definition
class Timer
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
	f64 GetTime() { return GetTime() - start_; }

	/**
	 * @return absolute time in seconds.
	 */
	static f64 GetAbsoluteTime();

private:
	f64 start_ = 0;
};
