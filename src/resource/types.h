#pragma once

#include "core/types.h"

namespace Resource
{
	/**
	 * Async result.
	 */
	struct AsyncResult final
	{
		/// Number of bytes processed in total.
		volatile i64 bytesProcessed_ = 0;
		/// Work that is remaining.
		volatile i32 workRemaining_ = 0;

		AsyncResult() = default;
		AsyncResult(const AsyncResult&) = delete;
	};


} // namespace Resource
