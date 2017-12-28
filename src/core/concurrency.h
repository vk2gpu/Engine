#pragma once

#include "core/dll.h"
#include "core/types.h"
#include "core/debug.h"

namespace Core
{
	/**
	 * Atomic operations.
	 * Acq (acquire) variants:
	 * - CPU is not permitted to reorder subsequent memory accesses to before the operation.
	 * Rel (release) variants:
	 * - CPU is not permitted to reorder preceeding memory accesses to after the operation.
	 */

	/// @return Return incremented result.
	CORE_DLL_INLINE i32 AtomicInc(volatile i32* dest);
	CORE_DLL_INLINE i32 AtomicIncAcq(volatile i32* dest);
	CORE_DLL_INLINE i32 AtomicIncRel(volatile i32* dest);
	/// @return Return decremented result.
	CORE_DLL_INLINE i32 AtomicDec(volatile i32* dest);
	CORE_DLL_INLINE i32 AtomicDecAcq(volatile i32* dest);
	CORE_DLL_INLINE i32 AtomicDecRel(volatile i32* dest);
	/// @return Return result of @a dest + value.
	CORE_DLL_INLINE i32 AtomicAdd(volatile i32* dest, i32 value);
	CORE_DLL_INLINE i32 AtomicAddAcq(volatile i32* dest, i32 value);
	CORE_DLL_INLINE i32 AtomicAddRel(volatile i32* dest, i32 value);
	/// @return Return original value of @a dest, set @dest to @a dest & @a value.
	CORE_DLL_INLINE i32 AtomicAnd(volatile i32* dest, i32 value);
	CORE_DLL_INLINE i32 AtomicAndAcq(volatile i32* dest, i32 value);
	CORE_DLL_INLINE i32 AtomicAndRel(volatile i32* dest, i32 value);
	/// @return Return original value of @a dest, set @dest to @a dest | @a value.
	CORE_DLL_INLINE i32 AtomicOr(volatile i32* dest, i32 value);
	CORE_DLL_INLINE i32 AtomicOrAcq(volatile i32* dest, i32 value);
	CORE_DLL_INLINE i32 AtomicOrRel(volatile i32* dest, i32 value);
	/// @return Return original value of @a dest, set @dest to @a dest ^ @a value.
	CORE_DLL_INLINE i32 AtomicXor(volatile i32* dest, i32 value);
	CORE_DLL_INLINE i32 AtomicXorAcq(volatile i32* dest, i32 value);
	CORE_DLL_INLINE i32 AtomicXorRel(volatile i32* dest, i32 value);
	/// @return Original value of dest, and set @dest to @exchg.
	CORE_DLL_INLINE i32 AtomicExchg(volatile i32* dest, i32 exchg);
	CORE_DLL_INLINE i32 AtomicExchgAcq(volatile i32* dest, i32 exchg);
	/// @return Original value of dest, and set @dest to @exchg.
	CORE_DLL_INLINE i32 AtomicCmpExchg(volatile i32* dest, i32 exchg, i32 comp);
	CORE_DLL_INLINE i32 AtomicCmpExchgAcq(volatile i32* dest, i32 exchg, i32 comp);
	CORE_DLL_INLINE i32 AtomicCmpExchgRel(volatile i32* dest, i32 exchg, i32 comp);

	/// @return Return incremented result.
	CORE_DLL_INLINE i64 AtomicInc(volatile i64* dest);
	CORE_DLL_INLINE i64 AtomicIncAcq(volatile i64* dest);
	CORE_DLL_INLINE i64 AtomicIncRel(volatile i64* dest);
	/// @return Return decremented result.
	CORE_DLL_INLINE i64 AtomicDec(volatile i64* dest);
	CORE_DLL_INLINE i64 AtomicDecAcq(volatile i64* dest);
	CORE_DLL_INLINE i64 AtomicDecRel(volatile i64* dest);
	/// @return Return result of @a dest + value.
	CORE_DLL_INLINE i64 AtomicAdd(volatile i64* dest, i64 value);
	CORE_DLL_INLINE i64 AtomicAddAcq(volatile i64* dest, i64 value);
	CORE_DLL_INLINE i64 AtomicAddRel(volatile i64* dest, i64 value);
	/// @return Return original value of @a dest, set @dest to @a dest & @a value.
	CORE_DLL_INLINE i64 AtomicAnd(volatile i64* dest, i64 value);
	CORE_DLL_INLINE i64 AtomicAndAcq(volatile i64* dest, i64 value);
	CORE_DLL_INLINE i64 AtomicAndRel(volatile i64* dest, i64 value);
	/// @return Return original value of @a dest, set @dest to @a dest | @a value.
	CORE_DLL_INLINE i64 AtomicOr(volatile i64* dest, i64 value);
	CORE_DLL_INLINE i64 AtomicOrAcq(volatile i64* dest, i64 value);
	CORE_DLL_INLINE i64 AtomicOrRel(volatile i64* dest, i64 value);
	/// @return Return original value of @a dest, set @dest to @a dest ^ @a value.
	CORE_DLL_INLINE i64 AtomicXor(volatile i64* dest, i64 value);
	CORE_DLL_INLINE i64 AtomicXorAcq(volatile i64* dest, i64 value);
	CORE_DLL_INLINE i64 AtomicXorRel(volatile i64* dest, i64 value);
	/// @return Original value of dest, and set @dest to @exchg.
	CORE_DLL_INLINE i64 AtomicExchg(volatile i64* dest, i64 exchg);
	CORE_DLL_INLINE i64 AtomicExchgAcq(volatile i64* dest, i64 exchg);
	/// @return Original value of dest, and if original value matches @a comp, set @dest to @exchg.
	CORE_DLL_INLINE i64 AtomicCmpExchg(volatile i64* dest, i64 exchg, i64 comp);
	CORE_DLL_INLINE i64 AtomicCmpExchgAcq(volatile i64* dest, i64 exchg, i64 comp);
	CORE_DLL_INLINE i64 AtomicCmpExchgRel(volatile i64* dest, i64 exchg, i64 comp);

	/**
	 * Utility.
	 */

	/// Allow CPU to give resources to waiting threads.
	CORE_DLL_INLINE void YieldCPU();
	/// Sleep thread for a number of @a seconds.
	CORE_DLL_INLINE void Sleep(double seconds);
	/// Tell CPU not to reorder instructions prior to the barrier to after.
	CORE_DLL_INLINE void Barrier();
	/// Allow OS to switch threads.
	CORE_DLL_INLINE void SwitchThread();
	/// Get number of logical cores.
	CORE_DLL i32 GetNumLogicalCores();

	/**
	 * Thread.
	 */
	class CORE_DLL Thread
	{
	public:
		typedef int (*EntryPointFunc)(void*);
		static const i32 DEFAULT_STACK_SIZE = 16 * 1024;

		/**
		 * Create thread.
		 * @param entryPointFunc Entry point for thread to call.
		 * @param userData User data to pass to thread.
		 * @param stackSize Size of stack for thread.
		 * @param debugName Debug name of thread.
		 * @pre entryPointFunc != nullptr.
		 */
		Thread(EntryPointFunc entryPointFunc, void* userData, i32 stackSize = DEFAULT_STACK_SIZE,
		    const char* debugName = nullptr);

		Thread() = default;
		~Thread();
		Thread(Thread&&);
		Thread& operator=(Thread&&);

		/**
		 * Set affinity.
		 * Controls which cores the thread is allowed to run on.
		 * @return Old mask.
		 */
		u64 SetAffinity(u64 mask);

		/**
		 * Wait for thread completion.
		 * @return Return value from thread.
		 * @post Thread no longer valid.
		 */
		i32 Join();

		/**
		 * @return Is thread valid?
		 */
		operator bool() const { return impl_ != nullptr; }

	private:
		Thread(const Thread&) = delete;
		struct ThreadImpl* impl_ = nullptr;
#if !defined(FINAL)
		const char* debugName_ = nullptr;
#endif
	};

	/**
	 * Fiber.
	 */
	class CORE_DLL Fiber final
	{
	public:
		typedef void (*EntryPointFunc)(void*);
		static const i32 DEFAULT_STACK_SIZE = 16 * 1024;

		enum ThisThread
		{
			THIS_THREAD
		};

		/**
		 * Create fiber.
		 * @param entryPointFunc Entry point for thread to call.
		 * @param userData User data to pass to thread.
		 * @param stackSize Size of stack for thread.
		 * @param debugName Debug name of fiber.
		 * @pre entryPointFunc != nullptr.
		 */
		Fiber(EntryPointFunc entryPointFunc, void* userData, i32 stackSize = DEFAULT_STACK_SIZE,
		    const char* debugName = nullptr);

		/**
		 * Create fiber from this thread.
		 * @param debugName Debug name.
		 */
		Fiber(ThisThread, const char* debugName = nullptr);

		Fiber() = default;
		~Fiber();
		Fiber(Fiber&&);
		Fiber& operator=(Fiber&&);

		/**
		 * Switch to this fiber.
		 */
		void SwitchTo();

		/**
		 * Get user data.
		 */
		void* GetUserData() const;

		/**
		 * @return Currently executing fiber. nullptr if not in fiber.
		 */
		static Fiber* GetCurrentFiber();

		/**
		 * @return Is thread valid?
		 */
		operator bool() const { return impl_ != nullptr; }

	private:
		Fiber(const Fiber&) = delete;
		struct FiberImpl* impl_ = nullptr;
#if !defined(FINAL)
		const char* debugName_ = nullptr;
#endif
	};

	/**
	 * Semaphore.
	 */
	class CORE_DLL Semaphore final
	{
	public:
		/**
		 * Create semaphore.
		 * @param initialCount Initial count for semaphore.
		 * @param maximumCount Maximum count for semaphore.
		 * @param debugName Debug name for semaphore.
		 */
		Semaphore(i32 initialCount, i32 maximumCount, const char* debugName = nullptr);
		~Semaphore();
		Semaphore(Semaphore&&);
		/**
		 * Wait for semaphore to be signalled, and decrease count by one if so.
		 * @param timeout Timeout in milliseconds.
		 * @return True if signalled, false if timed out or spurious wakeup.
		 */
		bool Wait(i32 timeout = -1);

		/**
		 * Signal sempahore.
		 * @param count Value to increment semaphore count by.
		 * @return Success.
		 */
		bool Signal(i32 count);

	private:
		Semaphore(const Semaphore&) = delete;

		struct SemaphoreImpl* impl_;
#if !defined(FINAL)
		const char* debugName_ = nullptr;
#endif
	};

	/**
	 * Simple spin lock that'll yield.
	 */
	class CORE_DLL SpinLock final
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
	class CORE_DLL ScopedSpinLock final
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
	 * Mutex lock. Can be used recursively.
	 */
	class CORE_DLL Mutex final
	{
	public:
		Mutex();
		~Mutex();
		Mutex(Mutex&&);
		Mutex& operator=(Mutex&& other);

		/**
		 * Lock.
		 */
		void Lock();

		/**
		 * Try lock.
		 * @return Success.
		 */
		bool TryLock();

		/**
		 * Unlock.
		 */
		void Unlock();

	private:
		Mutex(const Mutex&) = delete;
		struct MutexImpl* impl_ = nullptr;
	};

	/**
	 * Scoped mutex. Will lock and unlock within scope.
	 */
	class CORE_DLL ScopedMutex final
	{
	public:
		ScopedMutex(Mutex& mutex)
		    : mutex_(mutex)
		{
			mutex_.Lock();
		}

		~ScopedMutex() { mutex_.Unlock(); }

	private:
		ScopedMutex(const ScopedMutex&) = delete;
		ScopedMutex(ScopedMutex&&) = delete;

		Mutex& mutex_;
	};

	/**
	 * Read/write lock.
	 */
	class CORE_DLL RWLock final
	{
	public:
		RWLock();
		~RWLock();
		RWLock(RWLock&&);
		RWLock& operator=(RWLock&& other);

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

		struct RWLockImpl* impl_ = nullptr;
	};

	/**
	 * Scoped read lock.
	 */
	class CORE_DLL ScopedReadLock final
	{
	public:
		ScopedReadLock(ScopedReadLock&&) = default;
		ScopedReadLock(RWLock& lock)
		    : lock_(&lock)
		{
			DBG_ASSERT(lock_);
			lock_->BeginRead();
		}

		~ScopedReadLock()
		{
			if(lock_)
				lock_->EndRead();
		}

		explicit operator bool() const { return !!lock_; }

	private:
		ScopedReadLock(const ScopedReadLock&) = delete;

		RWLock* lock_ = nullptr;
	};

	/**
	 * Scoped write lock.
	 */
	class CORE_DLL ScopedWriteLock final
	{
	public:
		ScopedWriteLock(ScopedWriteLock&&) = default;
		ScopedWriteLock(RWLock& lock)
		    : lock_(&lock)
		{
			DBG_ASSERT(lock_);
			lock_->BeginWrite();
		}

		~ScopedWriteLock()
		{
			if(lock_)
				lock_->EndWrite();
		}

		explicit operator bool() const { return !!lock_; }

	private:
		ScopedWriteLock(const ScopedReadLock&) = delete;

		RWLock* lock_ = nullptr;
	};

	/**
	 * Thread local storage.
	 */
	class CORE_DLL TLS final
	{
	public:
		TLS();
		~TLS();

		/**
		 * Set data.
		 * @param data Data to set.
		 * @return Success.
		 */
		bool Set(void* data);

		/**
		 * Get data.
		 * @param data
		 * @return Data. nullptr if none set.
		 */
		void* Get() const;

		/**
		 * @return Is valid?
		 */
		operator bool() { return impl_ != nullptr; }

	private:
		TLS(TLS&&) = delete;
		TLS& operator=(TLS&&) = delete;
		TLS(const TLS&) = delete;

		struct TLSImpl* impl_ = nullptr;
	};

	/**
	 * Fiber local storage.
	 */
	class CORE_DLL FLS final
	{
	public:
		FLS();
		~FLS();

		/**
		 * Set data.
		 * @param data Data to set.
		 * @return Success.
		 */
		bool Set(void* data);

		/**
		 * Get data.
		 * @param data
		 * @return Data. nullptr if none set.
		 */
		void* Get() const;

		/**
		 * @return Is valid?
		 */
		operator bool() { return impl_ != nullptr; }

	private:
		FLS(FLS&&) = delete;
		FLS& operator=(FLS&&) = delete;
		FLS(const FLS&) = delete;

		struct FLSImpl* impl_ = nullptr;
	};

} // namespace Core
#if CODE_INLINE
#include "core/private/concurrency.inl"
#endif
