#pragma once

#include "job/dll.h"
#include "core/concurrency.h"

namespace Job
{
	/**
	 * Simple spin lock that'll yield to other running jobs.
	 */
	class JOB_DLL SpinLock final
	{
	public:
		SpinLock();
		~SpinLock();

		void Lock();
		bool TryLock();
		void Unlock();

	private:
		volatile i32 count_ = 0;
	};

	/**
	 * Scoped spin lock to auto lock/unlock.
	 */
	class JOB_DLL ScopedSpinLock final
	{
	public:
		ScopedSpinLock(SpinLock& spinLock)
		    : spinLock_(spinLock)
		{
			spinLock_.Lock();
		}

		~ScopedSpinLock() { spinLock_.Unlock(); }

	private:
		ScopedSpinLock(const ScopedSpinLock&) = delete;
		ScopedSpinLock(ScopedSpinLock&&) = delete;

		SpinLock& spinLock_;
	};
} // namespace Job
