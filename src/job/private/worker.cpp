#include "job/private/worker.h"
#include "core/debug.h"

namespace Job
{
	Worker::Worker()		
	{
		// Create thread.
		thread_ = Core::Thread(ThreadEntryPoint, this);
	}

	Worker::~Worker()
	{
		// Wait for thread to complete.
		i32 retVal = thread_.Join();
		DBG_ASSERT(retVal == 0);
	}

	i32 Worker::ThreadEntryPoint(void* param)
	{
		auto* worker = reinterpret_cast<Worker*>(param);
		DBG_ASSERT(worker);
		
		// Get pending job from manager impl.


		return 0;
	}

}
