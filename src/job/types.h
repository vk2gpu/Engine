#pragma once

#include "core/types.h"
#include "core/array.h"
#include "job/dll.h"

namespace Job
{
	/**
	 * Job function.
	 * First parameter comes from JobDesc::jobParam_.
	 * Second parameter comes from JobDesc::jobData_.
	 */
	typedef void (*JobFunc)(i32, void*);

	/**
	 * Job descriptor.
	 */
	struct JobDesc final
	{
		/// Function to call to execute job.
		JobFunc func_ = nullptr;
		/// Paramaeter to be passed to job.
		i32 param_ = 0;
		/// Data to be passed to job.
		void* data_ = nullptr;
		/// Name of job.
		const char* name_ = nullptr;

		/// Internal use. Do not use.
		struct Counter* counter_ = nullptr;
		/// Internal use. Do not use.
		i32 idx_ = -1;
		/// Internal use. Do not use.
		bool freeCounter_ = false;
	};

	/**
	 * Counter used for waiting on jobs.
	 */
	struct Counter;

	/**
	 * Profiler entry data.
	 */
	struct ProfilerEntry
	{
		Core::Array<char, 64> name_;
		i32 param_ = 0;
		i32 workerIdx_ = -1;
		i32 jobIdx_ = -1;
		f64 startTime_ = 0.0;
		f64 endTime_ = 0.0;
	};

} // namespace Job

/**
 * Define a job entrypoint.
 */
#define JOB_ENTRY_POINT(NAME) static void NAME(i32 jobParam, void* jobData)
