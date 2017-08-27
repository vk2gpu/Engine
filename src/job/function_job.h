#pragma once
#pragma once

#include "job/dll.h"
#include "job/types.h"
#include "core/function.h"

namespace Job
{
	/// Job function alias.
	using JobFunction = Core::Function<void(i32), 64>;

	/**
	 * Basic job for simple tasks.
	 */
	class JOB_DLL FunctionJob
	{
	public:
		FunctionJob(const char* name, JobFunction onWorkFn);
		virtual ~FunctionJob();

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

	private:
		Job::JobDesc baseJobDesc_;
		volatile i32 running_ = 0;
		JobFunction onWorkFn_;
	};


} // namespace Job
