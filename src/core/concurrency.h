#pragma once

#include "core/types.h"

/**
 * Atomic operations.
 * Acq (acquire) variants:
 * - CPU is not permitted to reorder subsequent memory acceses to before the operation.
 * Rel (release) variants:
 * - CPU is not permitted to reorder preceeding memory accesses to after the operation.
 */

/// @return Return incremented result.
i32 AtomicInc(volatile i32* dest);
i32 AtomicIncAcq(volatile i32* dest);
i32 AtomicIncRel(volatile i32* dest);
/// @return Return incremented result.
i32 AtomicDec(volatile i32* dest);
i32 AtomicDecAcq(volatile i32* dest);
i32 AtomicDecRel(volatile i32* dest);
/// @return Return result of @a dest + value.
i32 AtomicAdd(volatile i32* dest, i32 value);
i32 AtomicAddAcq(volatile i32* dest, i32 value);
i32 AtomicAddRel(volatile i32* dest, i32 value);
/// @return Return original value of @a dest, set @dest to @a dest & @a value.
i32 AtomicAnd(volatile i32* dest, i32 value);
i32 AtomicAndAcq(volatile i32* dest, i32 value);
i32 AtomicAndRel(volatile i32* dest, i32 value);
/// @return Return original value of @a dest, set @dest to @a dest | @a value.
i32 AtomicOr(volatile i32* dest, i32 value);
i32 AtomicOrAcq(volatile i32* dest, i32 value);
i32 AtomicOrRel(volatile i32* dest, i32 value);
/// @return Return original value of @a dest, set @dest to @a dest ^ @a value.
i32 AtomicXor(volatile i32* dest, i32 value);
i32 AtomicXorAcq(volatile i32* dest, i32 value);
i32 AtomicXorRel(volatile i32* dest, i32 value);
/// @return Original value of dest, and if original value matches @a comp, set @dest to @exchg.
i32 AtomicCmpExchg(volatile i32* dest, i32 exchg, i32 comp);
i32 AtomicCmpExchgAcq(volatile i32* dest, i32 exchg, i32 comp);
i32 AtomicCmpExchgRel(volatile i32* dest, i32 exchg, i32 comp);

/// @return Return incremented result.
i64 AtomicInc(volatile i64* dest);
i64 AtomicIncAcq(volatile i64* dest);
i64 AtomicIncRel(volatile i64* dest);
/// @return Return incremented result.
i64 AtomicDec(volatile i64* dest);
i64 AtomicDecAcq(volatile i64* dest);
i64 AtomicDecRel(volatile i64* dest);
/// @return Return result of @a dest + value.
i64 AtomicAdd(volatile i64* dest, i64 value);
i64 AtomicAddAcq(volatile i64* dest, i64 value);
i64 AtomicAddRel(volatile i64* dest, i64 value);
/// @return Return original value of @a dest, set @dest to @a dest & @a value.
i64 AtomicAnd(volatile i64* dest, i64 value);
i64 AtomicAndAcq(volatile i64* dest, i64 value);
i64 AtomicAndRel(volatile i64* dest, i64 value);
/// @return Return original value of @a dest, set @dest to @a dest | @a value.
i64 AtomicOr(volatile i64* dest, i64 value);
i64 AtomicOrAcq(volatile i64* dest, i64 value);
i64 AtomicOrRel(volatile i64* dest, i64 value);
/// @return Return original value of @a dest, set @dest to @a dest ^ @a value.
i64 AtomicXor(volatile i64* dest, i64 value);
i64 AtomicXorAcq(volatile i64* dest, i64 value);
i64 AtomicXorRel(volatile i64* dest, i64 value);
/// @return Original value of dest, and if original value matches @a comp, set @dest to @exchg.
i64 AtomicCmpExchg(volatile i64* dest, i64 exchg, i64 comp);
i64 AtomicCmpExchgAcq(volatile i64* dest, i64 exchg, i64 comp);
i64 AtomicCmpExchgRel(volatile i64* dest, i64 exchg, i64 comp);

/**
 * Thread.
 */
class Thread
{
public:
	typedef int (*EntryPointFunc)(void*);
	static const i32 DEFAULT_STACK_SIZE = 16 * 1024;

	/**
	 * Create thread.
	 * @param entryPointFunc Entry point for thread to call.
	 * @param userData User data to pass to thread.
	 * @param stackSize Size of stack for thread.
	 * @pre entryPointFunc != nullptr.
	 */
	Thread(EntryPointFunc entryPointFunc, void* userData, i32 stackSize = DEFAULT_STACK_SIZE);

	Thread() = default;
	~Thread();
	Thread(Thread&&);
	Thread& operator=(Thread&&);

	/**
	 * Wait for thread completion.
	 * @return Return value from thread.
	 * @pre Is valid.
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
};

/**
 * Fiber.
 */
class Fiber final
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
	 * @pre entryPointFunc != nullptr.
	 */
	Fiber(EntryPointFunc entryPointFunc, void* userData, i32 stackSize = DEFAULT_STACK_SIZE);

	/**
	 * Create fiber from this thread.
	 */
	Fiber(ThisThread);

	Fiber() = default;
	~Fiber();
	Fiber(Fiber&&);
	Fiber& operator=(Fiber&&);

	/**
	 * Switch to this fiber.
	 * @param caller Fiber we are calling from.
	 */
	void SwitchTo(Fiber& caller);

	/**
	 * @return Is thread valid?
	 */
	operator bool() const { return impl_ != nullptr; }

private:
	Fiber(const Fiber&) = delete;
	struct FiberImpl* impl_ = nullptr;
};

/**
 * Event.
 */
class Event final
{
public:
	/**
	 * Create event.
	 * @param manualReset Should manual reset be required?
	 * @param initialState Should start signalled?
	 */
	Event(bool manualReset = false, bool initialState = false);
	~Event();

	/**
	 * Wait for event to be signalled.
	 * @param timeout Timeout in milliseconds.
	 * @return True if signalled, false if timed out or spurious wakeup.
	 */
	bool Wait(i32 timeout = -1);

	/**
	 * Signal.
	 * @return Success.
	 */
	bool Signal();

	/**
	 * Reset.
	 * @return Success.
	 */
	bool Reset();

private:
	Event(const Event&) = delete;
	Event(Event&&) = delete;

	struct EventImpl* impl_;
};

/**
 * Mutex lock. Can be used recursively.
 */
class Mutex final
{
public:
	Mutex();
	~Mutex();

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
	Mutex(Mutex&&) = delete;

	struct MutexImpl* impl_ = nullptr;
};

/**
 * Scoped mutex. Will lock and unlock within scope.
 */
class ScopedMutex final
{
public:
	ScopedMutex(Mutex& mutex);
	~ScopedMutex();

private:
	ScopedMutex(const ScopedMutex&) = delete;
	ScopedMutex(ScopedMutex&&) = delete;

	Mutex& mutex_;
};

/**
 * Thread local storage.
 */
class TLS
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


#if CODE_INLINE
#include "core/private/concurrency.inl"
#endif
