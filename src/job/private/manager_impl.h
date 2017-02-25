#pragma once

#include "core/concurrency.h"
#include "core/debug.h"
#include "core/vector.h"

#include "job/private/queue.h"
#include "job/private/worker.h"

namespace Job
{
	struct ManagerImpl
	{
		/// Worker pool.
		Core::Vector<Worker> workers_;
		/// Jobs.
		Core::Vector<JobDesc> pendingJobs_;
		/// Free fibers.
		Core::Vector<Core::Fiber> freeFibers_;
		/// Waiting fibers.
		Core::Vector<Core::Fiber> waitingFibers_;

		
	};
}
