#pragma once

#include "job/dll.h"
#include "core/concurrency.h"

#include <utility>

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

	/**
	 * Read/write lock.
	 */
	class JOB_DLL RWLock final
	{
	public:
		RWLock();
		~RWLock();

		/**
		 * Begin read.
		 */
		void BeginRead();

		/**
		 * End read.
		 */
		void EndRead();

		/**
		 * Begin write.
		 */
		void BeginWrite();

		/**
		 * End write.
		 */
		void EndWrite();

	private:
		RWLock(const RWLock&) = delete;

		SpinLock rMutex_;
		SpinLock gMutex_;
		volatile i32 readCount_ = 0;
	};

	/**
	 * Scoped read lock.
	 */
	class JOB_DLL ScopedReadLock final
	{
	public:
		ScopedReadLock(RWLock& lock)
		    : lock_(&lock)
		{
			lock_->BeginRead();
		}

		ScopedReadLock(ScopedReadLock&& other) { std::swap(lock_, other.lock_); }

		~ScopedReadLock()
		{
			if(lock_)
				lock_->EndRead();
		}

	private:
		ScopedReadLock(const ScopedReadLock&) = delete;

		RWLock* lock_ = nullptr;
	};

	/**
	 * Scoped write lock.
	 */
	class JOB_DLL ScopedWriteLock final
	{
	public:
		ScopedWriteLock(RWLock& lock)
		    : lock_(&lock)
		{
			lock_->BeginWrite();
		}

		ScopedWriteLock(ScopedWriteLock&& other) { std::swap(lock_, other.lock_); }

		~ScopedWriteLock()
		{
			if(lock_)
				lock_->EndWrite();
		}

	private:
		ScopedWriteLock(const ScopedReadLock&) = delete;

		RWLock* lock_ = nullptr;
	};

} // namespace Job
