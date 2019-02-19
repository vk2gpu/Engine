#pragma once

#if PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include "core/os.h"

namespace Core
{
	static_assert(sizeof(LONG) == sizeof(i32), "LONG and i32 sizes are not compatible.");

	// clang-format off
	CORE_DLL_INLINE i32 AtomicInc(volatile i32* dest) { return ::InterlockedIncrement((volatile LONG*)dest); }
	CORE_DLL_INLINE i32 AtomicIncAcq(volatile i32* dest) { return ::InterlockedIncrementAcquire((volatile LONG*)dest); }
	CORE_DLL_INLINE i32 AtomicIncRel(volatile i32* dest) { return ::InterlockedIncrementRelease((volatile LONG*)dest); }

	CORE_DLL_INLINE i32 AtomicDec(volatile i32* dest) { return ::InterlockedDecrement((volatile LONG*)dest); }
	CORE_DLL_INLINE i32 AtomicDecAcq(volatile i32* dest) { return ::InterlockedDecrementAcquire((volatile LONG*)dest); }
	CORE_DLL_INLINE i32 AtomicDecRel(volatile i32* dest) { return ::InterlockedDecrementRelease((volatile LONG*)dest); }

	CORE_DLL_INLINE i32 AtomicAdd(volatile i32* dest, i32 value) { return ::InterlockedAdd((volatile LONG*)dest, value); }
	CORE_DLL_INLINE i32 AtomicAddAcq(volatile i32* dest, i32 value) { return ::InterlockedAddAcquire((volatile LONG*)dest, value); }
	CORE_DLL_INLINE i32 AtomicAddRel(volatile i32* dest, i32 value) { return ::InterlockedAddRelease((volatile LONG*)dest, value); }

	CORE_DLL_INLINE i32 AtomicAnd(volatile i32* dest, i32 value) { return ::InterlockedAnd((volatile LONG*)dest, value); }
	CORE_DLL_INLINE i32 AtomicAndAcq(volatile i32* dest, i32 value) { return ::InterlockedAndAcquire((volatile LONG*)dest, value); }
	CORE_DLL_INLINE i32 AtomicAndRel(volatile i32* dest, i32 value) { return ::InterlockedAndRelease((volatile LONG*)dest, value); }

	CORE_DLL_INLINE i32 AtomicOr(volatile i32* dest, i32 value) { return ::InterlockedOr((volatile LONG*)dest, value); }
	CORE_DLL_INLINE i32 AtomicOrAcq(volatile i32* dest, i32 value) { return ::InterlockedOrAcquire((volatile LONG*)dest, value); }
	CORE_DLL_INLINE i32 AtomicOrRel(volatile i32* dest, i32 value) { return ::InterlockedOrRelease((volatile LONG*)dest, value); }

	CORE_DLL_INLINE i32 AtomicXor(volatile i32* dest, i32 value) { return ::InterlockedXor((volatile LONG*)dest, value); }
	CORE_DLL_INLINE i32 AtomicXorAcq(volatile i32* dest, i32 value) { return ::InterlockedXorAcquire((volatile LONG*)dest, value); }
	CORE_DLL_INLINE i32 AtomicXorRel(volatile i32* dest, i32 value) { return ::InterlockedXorRelease((volatile LONG*)dest, value); }

	CORE_DLL_INLINE i32 AtomicExchg(volatile i32* dest, i32 exchg) { return ::InterlockedExchange((volatile LONG*)dest, exchg); }
	CORE_DLL_INLINE i32 AtomicExchgAcq(volatile i32* dest, i32 exchg) { return ::InterlockedExchangeAcquire((volatile LONG*)dest, exchg); }

	CORE_DLL_INLINE i32 AtomicCmpExchg(volatile i32* dest, i32 exchg, i32 comp) { return ::InterlockedCompareExchange((volatile LONG*)dest, exchg, comp); }
	CORE_DLL_INLINE i32 AtomicCmpExchgAcq(volatile i32* dest, i32 exchg, i32 comp) { return ::InterlockedCompareExchangeAcquire((volatile LONG*)dest, exchg, comp); }
	CORE_DLL_INLINE i32 AtomicCmpExchgRel(volatile i32* dest, i32 exchg, i32 comp) { return ::InterlockedCompareExchangeRelease((volatile LONG*)dest, exchg, comp); }

	CORE_DLL_INLINE i64 AtomicInc(volatile i64* dest) { return ::InterlockedIncrement64(dest); }
	CORE_DLL_INLINE i64 AtomicIncAcq(volatile i64* dest) { return ::InterlockedIncrementAcquire64(dest); }
	CORE_DLL_INLINE i64 AtomicIncRel(volatile i64* dest) { return ::InterlockedIncrementRelease64(dest); }

	CORE_DLL_INLINE i64 AtomicDec(volatile i64* dest) { return ::InterlockedDecrement64(dest); }
	CORE_DLL_INLINE i64 AtomicDecAcq(volatile i64* dest) { return ::InterlockedDecrementAcquire64(dest); }
	CORE_DLL_INLINE i64 AtomicDecRel(volatile i64* dest) { return ::InterlockedDecrementRelease64(dest); }

	CORE_DLL_INLINE i64 AtomicAdd(volatile i64* dest, i64 value) { return ::InterlockedAdd64(dest, value); }
	CORE_DLL_INLINE i64 AtomicAddAcq(volatile i64* dest, i64 value) { return ::InterlockedAddAcquire64(dest, value); }
	CORE_DLL_INLINE i64 AtomicAddRel(volatile i64* dest, i64 value) { return ::InterlockedAddRelease64(dest, value); }

	CORE_DLL_INLINE i64 AtomicAnd(volatile i64* dest, i64 value) { return ::InterlockedAnd64(dest, value); }
	CORE_DLL_INLINE i64 AtomicAndAcq(volatile i64* dest, i64 value) { return ::InterlockedAnd64Acquire(dest, value); }
	CORE_DLL_INLINE i64 AtomicAndRel(volatile i64* dest, i64 value) { return ::InterlockedAnd64Release(dest, value); }

	CORE_DLL_INLINE i64 AtomicOr(volatile i64* dest, i64 value) { return ::InterlockedOr64(dest, value); }
	CORE_DLL_INLINE i64 AtomicOrAcq(volatile i64* dest, i64 value) { return ::InterlockedOr64Acquire(dest, value); }
	CORE_DLL_INLINE i64 AtomicOrRel(volatile i64* dest, i64 value) { return ::InterlockedOr64Release(dest, value); }

	CORE_DLL_INLINE i64 AtomicXor(volatile i64* dest, i64 value) { return ::InterlockedXor64(dest, value); }
	CORE_DLL_INLINE i64 AtomicXorAcq(volatile i64* dest, i64 value) { return ::InterlockedXor64Acquire(dest, value); }
	CORE_DLL_INLINE i64 AtomicXorRel(volatile i64* dest, i64 value) { return ::InterlockedXor64Release(dest, value); }

	CORE_DLL_INLINE i64 AtomicExchg(volatile i64* dest, i64 exchg) { return ::InterlockedExchange64(dest, exchg); }
	CORE_DLL_INLINE i64 AtomicExchgAcq(volatile i64* dest, i64 exchg) { return ::InterlockedExchangeAcquire64(dest, exchg); }

	CORE_DLL_INLINE i64 AtomicCmpExchg(volatile i64* dest, i64 exchg, i64 comp) { return ::InterlockedCompareExchange64(dest, exchg, comp); }
	CORE_DLL_INLINE i64 AtomicCmpExchgAcq(volatile i64* dest, i64 exchg, i64 comp) { return ::InterlockedCompareExchangeAcquire64(dest, exchg, comp); }
	CORE_DLL_INLINE i64 AtomicCmpExchgRel(volatile i64* dest, i64 exchg, i64 comp) { return ::InterlockedCompareExchangeRelease64(dest, exchg, comp); }
	
	CORE_DLL_INLINE void YieldCPU() { ::YieldProcessor(); }
	CORE_DLL_INLINE void Sleep(double seconds) { ::Sleep((DWORD)(1000 * seconds)); }
	CORE_DLL_INLINE void Barrier() { ::MemoryBarrier(); }
	CORE_DLL_INLINE void SwitchThread() { ::SwitchToThread(); };
	// clang-format on
} // namespace Core

#elif defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID) || defined(PLATFORM_OSX) || defined(PLATFORM_IOS)
#include "core/os.h"

namespace Core
{
	//static_assert(sizeof(LONG) == sizeof(i32), "LONG and i32 sizes are not compatible.");

	// clang-format off
	CORE_DLL_INLINE i32 AtomicInc(volatile i32* dest) { return __atomic_add_fetch(dest, 1, __ATOMIC_ACQ_REL); }
	CORE_DLL_INLINE i32 AtomicIncAcq(volatile i32* dest) { return __atomic_add_fetch(dest, 1, __ATOMIC_ACQUIRE); }
	CORE_DLL_INLINE i32 AtomicIncRel(volatile i32* dest) { return __atomic_add_fetch(dest, 1, __ATOMIC_RELEASE); }

	CORE_DLL_INLINE i32 AtomicDec(volatile i32* dest) { return __atomic_sub_fetch(dest, 1, __ATOMIC_ACQ_REL); }
	CORE_DLL_INLINE i32 AtomicDecAcq(volatile i32* dest) { return __atomic_sub_fetch(dest, 1, __ATOMIC_ACQUIRE); }
	CORE_DLL_INLINE i32 AtomicDecRel(volatile i32* dest) { return __atomic_sub_fetch(dest, 1, __ATOMIC_RELEASE); }

	CORE_DLL_INLINE i32 AtomicAdd(volatile i32* dest, i32 value) { return __atomic_add_fetch(dest, value, __ATOMIC_ACQ_REL); }
	CORE_DLL_INLINE i32 AtomicAddAcq(volatile i32* dest, i32 value) { return __atomic_add_fetch(dest, value, __ATOMIC_ACQUIRE); }
	CORE_DLL_INLINE i32 AtomicAddRel(volatile i32* dest, i32 value) { return __atomic_add_fetch(dest, value, __ATOMIC_RELEASE); }

	CORE_DLL_INLINE i32 AtomicAnd(volatile i32* dest, i32 value) { return __atomic_and_fetch(dest, value, __ATOMIC_ACQ_REL); }
	CORE_DLL_INLINE i32 AtomicAndAcq(volatile i32* dest, i32 value) { return __atomic_and_fetch(dest, value, __ATOMIC_ACQUIRE); }
	CORE_DLL_INLINE i32 AtomicAndRel(volatile i32* dest, i32 value) { return __atomic_and_fetch(dest, value, __ATOMIC_RELEASE); }

	CORE_DLL_INLINE i32 AtomicOr(volatile i32* dest, i32 value) { return __atomic_or_fetch(dest, value, __ATOMIC_ACQ_REL); }
	CORE_DLL_INLINE i32 AtomicOrAcq(volatile i32* dest, i32 value) { return __atomic_or_fetch(dest, value, __ATOMIC_ACQUIRE); }
	CORE_DLL_INLINE i32 AtomicOrRel(volatile i32* dest, i32 value) { return __atomic_or_fetch(dest, value, __ATOMIC_RELEASE); }

	CORE_DLL_INLINE i32 AtomicXor(volatile i32* dest, i32 value) { return __atomic_xor_fetch(dest, value, __ATOMIC_ACQ_REL); }
	CORE_DLL_INLINE i32 AtomicXorAcq(volatile i32* dest, i32 value) { return __atomic_xor_fetch(dest, value, __ATOMIC_ACQUIRE); }
	CORE_DLL_INLINE i32 AtomicXorRel(volatile i32* dest, i32 value) { return __atomic_xor_fetch(dest, value, __ATOMIC_RELEASE); }

	CORE_DLL_INLINE i32 AtomicExchg(volatile i32* dest, i32 exchg) { return __atomic_exchange_n(dest, exchg, __ATOMIC_ACQ_REL); }
	CORE_DLL_INLINE i32 AtomicExchgAcq(volatile i32* dest, i32 exchg) { return __atomic_exchange_n(dest, exchg, __ATOMIC_ACQUIRE); }

	CORE_DLL_INLINE i32 AtomicCmpExchg(volatile i32* dest, i32 exchg, i32 comp) { __atomic_compare_exchange(dest, &comp, &exchg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); return *dest; }
	CORE_DLL_INLINE i32 AtomicCmpExchgAcq(volatile i32* dest, i32 exchg, i32 comp) { __atomic_compare_exchange(dest, &comp, &exchg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); return *dest; }
	CORE_DLL_INLINE i32 AtomicCmpExchgRel(volatile i32* dest, i32 exchg, i32 comp) { __atomic_compare_exchange(dest, &comp, &exchg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); return *dest; }

	CORE_DLL_INLINE i64 AtomicInc(volatile i64* dest) { return __atomic_add_fetch(dest, 1, __ATOMIC_ACQ_REL); }
	CORE_DLL_INLINE i64 AtomicIncAcq(volatile i64* dest) { return __atomic_add_fetch(dest, 1, __ATOMIC_ACQUIRE); }
	CORE_DLL_INLINE i64 AtomicIncRel(volatile i64* dest) { return __atomic_add_fetch(dest, 1, __ATOMIC_RELEASE); }

	CORE_DLL_INLINE i64 AtomicDec(volatile i64* dest) { return __atomic_sub_fetch(dest, 1, __ATOMIC_ACQ_REL); }
	CORE_DLL_INLINE i64 AtomicDecAcq(volatile i64* dest) { return __atomic_sub_fetch(dest, 1, __ATOMIC_ACQUIRE); }
	CORE_DLL_INLINE i64 AtomicDecRel(volatile i64* dest) { return __atomic_sub_fetch(dest, 1, __ATOMIC_RELEASE); }

	CORE_DLL_INLINE i64 AtomicAdd(volatile i64* dest, i64 value) { return __atomic_add_fetch(dest, value, __ATOMIC_ACQ_REL); }
	CORE_DLL_INLINE i64 AtomicAddAcq(volatile i64* dest, i64 value) { return __atomic_add_fetch(dest, value, __ATOMIC_ACQUIRE); }
	CORE_DLL_INLINE i64 AtomicAddRel(volatile i64* dest, i64 value) { return __atomic_add_fetch(dest, value, __ATOMIC_RELEASE); }

	CORE_DLL_INLINE i64 AtomicAnd(volatile i64* dest, i64 value) { return __atomic_and_fetch(dest, value, __ATOMIC_ACQ_REL); }
	CORE_DLL_INLINE i64 AtomicAndAcq(volatile i64* dest, i64 value) { return __atomic_and_fetch(dest, value, __ATOMIC_ACQUIRE); }
	CORE_DLL_INLINE i64 AtomicAndRel(volatile i64* dest, i64 value) { return __atomic_and_fetch(dest, value, __ATOMIC_RELEASE); }

	CORE_DLL_INLINE i64 AtomicOr(volatile i64* dest, i64 value) { return __atomic_or_fetch(dest, value, __ATOMIC_ACQ_REL); }
	CORE_DLL_INLINE i64 AtomicOrAcq(volatile i64* dest, i64 value) { return __atomic_or_fetch(dest, value, __ATOMIC_ACQUIRE); }
	CORE_DLL_INLINE i64 AtomicOrRel(volatile i64* dest, i64 value) { return __atomic_or_fetch(dest, value, __ATOMIC_RELEASE); }

	CORE_DLL_INLINE i64 AtomicXor(volatile i64* dest, i64 value) { return __atomic_xor_fetch(dest, value, __ATOMIC_ACQ_REL); }
	CORE_DLL_INLINE i64 AtomicXorAcq(volatile i64* dest, i64 value) { return __atomic_xor_fetch(dest, value, __ATOMIC_ACQUIRE); }
	CORE_DLL_INLINE i64 AtomicXorRel(volatile i64* dest, i64 value) { return __atomic_xor_fetch(dest, value, __ATOMIC_RELEASE); }

	CORE_DLL_INLINE i64 AtomicExchg(volatile i64* dest, i64 exchg) { return __atomic_exchange_n(dest, exchg, __ATOMIC_ACQ_REL); }
	CORE_DLL_INLINE i64 AtomicExchgAcq(volatile i64* dest, i64 exchg) { return __atomic_exchange_n(dest, exchg, __ATOMIC_ACQUIRE); }

	CORE_DLL_INLINE i64 AtomicCmpExchg(volatile i64* dest, i64 exchg, i64 comp) { __atomic_compare_exchange(dest, &comp, &exchg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); return *dest; }
	CORE_DLL_INLINE i64 AtomicCmpExchgAcq(volatile i64* dest, i64 exchg, i64 comp) { __atomic_compare_exchange(dest, &comp, &exchg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); return *dest; }
	CORE_DLL_INLINE i64 AtomicCmpExchgRel(volatile i64* dest, i64 exchg, i64 comp) { __atomic_compare_exchange(dest, &comp, &exchg, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); return *dest; }
	
	CORE_DLL_INLINE void YieldCPU() { sched_yield(); }
	CORE_DLL_INLINE void Sleep(double seconds) { sleep((unsigned)(seconds)); }
	CORE_DLL_INLINE void Barrier() { __sync_synchronize(); }
	CORE_DLL_INLINE void SwitchThread() { sched_yield(); };
	// clang-format on
} // namespace Core

#endif
