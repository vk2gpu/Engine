#include "job/function_job.h"
#include "job/manager.h"
#include "core/concurrency.h"
#include "core/timer.h"
#include "core/vector.h"

namespace Job
{
	FunctionJob::FunctionJob(const char* name, JobFunction onWorkFn)
	    : onWorkFn_(onWorkFn)
	{
		// Setup base job descriptor.
		baseJobDesc_.func_ = [](i32 param, void* data) {
			auto* _this = reinterpret_cast<FunctionJob*>(data);
			_this->onWorkFn_(param);

			i32 numRunning = Core::AtomicDec(&(_this->running_));
			DBG_ASSERT(numRunning >= 0);
		};
		baseJobDesc_.data_ = this;
		baseJobDesc_.name_ = name;
	}

	FunctionJob::~FunctionJob()
	{
		const f64 MAX_WAIT_TIME = 30.0;
		Core::Timer timer;
		timer.Mark();
		while(running_ > 0)
		{
			Job::Manager::YieldCPU();
			DBG_ASSERT(timer.GetTime() < MAX_WAIT_TIME);
		}
	}

	void FunctionJob::RunSingle(Priority prio, i32 param, Counter** counter)
	{
		JobDesc jobDesc = baseJobDesc_;
		jobDesc.prio_ = prio;
		jobDesc.param_ = param;
		Core::AtomicInc(&running_);
		Job::Manager::RunJobs(&jobDesc, 1, counter);
	}

	void FunctionJob::RunMultiple(Priority prio, i32 paramMin, i32 paramMax, Counter** counter)
	{
		DBG_ASSERT(paramMax >= paramMin);
		Core::Vector<JobDesc> jobDescs;
		jobDescs.resize((paramMax - paramMin) + 1);
		i32 param = paramMin;
		for(auto& jobDesc : jobDescs)
		{
			jobDesc = baseJobDesc_;
			jobDesc.prio_ = prio;
			jobDesc.param_ = param++;
		}
		Core::AtomicAdd(&running_, jobDescs.size());
		Job::Manager::RunJobs(jobDescs.data(), jobDescs.size(), counter);
	}

} // namespace Job
