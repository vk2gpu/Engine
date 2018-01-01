#include "job/dll.h"
#include "job/concurrency.h"
#include "job/manager.h"
#include "core/concurrency.h"
#include "core/debug.h"

namespace Job
{
	SpinLock::SpinLock() {}

	SpinLock::~SpinLock() { DBG_ASSERT(count_ == 0); }

	void SpinLock::Lock()
	{
		while(Core::AtomicCmpExchgAcq(&count_, 1, 0) == 1)
		{
			Job::Manager::YieldCPU();
		}
	}

	bool SpinLock::TryLock() { return (Core::AtomicCmpExchgAcq(&count_, 1, 0) == 0); }

	void SpinLock::Unlock()
	{
		i32 count = Core::AtomicExchg(&count_, 0);
		DBG_ASSERT(count == 1);
	}


	RWLock::RWLock() {}

	RWLock::~RWLock()
	{
		DBG_ASSERT(readCount_ == 0);
#ifdef DEBUG
		if(gMutex_.TryLock() == false)
			DBG_ASSERT(false);
		gMutex_.Unlock();
#endif
	}

	void RWLock::BeginRead()
	{
		ScopedSpinLock lock(rMutex_);
		if(Core::AtomicInc(&readCount_) == 1)
		{
			gMutex_.Lock();
		}
	}

	void RWLock::EndRead()
	{
		ScopedSpinLock lock(rMutex_);
		if(Core::AtomicDec(&readCount_) == 0)
		{
			gMutex_.Unlock();
		}
	}

	void RWLock::BeginWrite() { gMutex_.Lock(); }

	void RWLock::EndWrite() { gMutex_.Unlock(); }

} // namespace Job
