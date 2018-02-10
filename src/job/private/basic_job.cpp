#include "job/basic_job.h"
#include "job/manager.h"
#include "core/concurrency.h"
#include "core/vector.h"

namespace Job
{
	BasicJob::BasicJob(const char* name)
	{
		// Setup base job descriptor.
		baseJobDesc_.func_ = [](i32 param, void* data) {
			auto* _this = reinterpret_cast<BasicJob*>(data);
			_this->OnWork(param);

			i32 numRunning = Core::AtomicDec(&(_this->running_));
			DBG_ASSERT(numRunning >= 0);

			if(numRunning == 0)
				_this->OnCompleted();
		};
		baseJobDesc_.data_ = this;
		baseJobDesc_.name_ = name;
	}

	BasicJob::~BasicJob() {}

	void BasicJob::RunSingle(Priority prio, i32 param, Counter** counter)
	{
		JobDesc jobDesc = baseJobDesc_;
		jobDesc.prio_ = prio;
		jobDesc.param_ = param;
		Core::AtomicInc(&running_);
		Job::Manager::RunJobs(&jobDesc, 1, counter);
	}

	void BasicJob::RunMultiple(Priority prio, i32 paramMin, i32 paramMax, Counter** counter)
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

	void BasicJob::OnCompleted() {}

} // namespace Job
