#pragma once

#include "job/dll.h"
#include "job/types.h"

namespace Job
{
	/**
	 * Basic job for simple tasks.
	 */
	class JOB_DLL BasicJob
	{
	public:
		BasicJob(const char* name);
		virtual ~BasicJob();

		/**
		 * Run single job.
		 * @param param To pass to the OnWork function.
		 * @param counter Counter for how many jobs are currently pending completion.
		 */
		void RunSingle(i32 param, Counter** counter = nullptr);

		/**
		 * Run multiple jobs.
		 * Will run jobs starting with the value @a paramMin, to @a paramMin inclusively.
		 * @param paramMin Minimum parameter value.
		 * @param paramMax Maximum parameter value.
		 * @param counter Counter for how many jobs are currently pending completion.
		 */
		void RunMultiple(i32 paramMin, i32 paramMax, Counter** counter = nullptr);

		/**
		 * Called when job should do its work.
		 */
		virtual void OnWork(i32 param) = 0;

		/**
		 * Called when final scheduled instance has completed.
		 */
		virtual void OnCompleted();

	private:
		Job::JobDesc baseJobDesc_;
		volatile i32 running_ = 0;
	};


} // namespace Job
