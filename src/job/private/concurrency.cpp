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
} // namespace Job
