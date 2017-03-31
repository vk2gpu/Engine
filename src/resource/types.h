#pragma once

#include "core/types.h"

namespace Resource
{
	enum Result : i32
	{
		INITIAL = 0,
		PENDING = 1,
		RUNNING = 2,
		SUCCESS = 3,
		FAILURE = -1,
	};

	/**
	 * Async result.
	 */
	struct AsyncResult final
	{
		/// Work left remaining for job completion.
		volatile i64 workRemaining_ = 0;
		/// Result.
		volatile Result result_ = Result::INITIAL;

		AsyncResult() = default;
		AsyncResult(const AsyncResult&) = delete;

		// Check if complete.
		bool IsComplete() const { return result_ == Result::SUCCESS || result_ == Result::FAILURE; }
	};

} // namespace Resource
