#pragma once

#include "core/types.h"
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
		/// Should counter be freed by the last that's using it?
		bool freeCounter_ = false;

	};

	/**
	 * Counter used for waiting on jobs.
	 */
	struct Counter;

} // namespace Job

/**
 * Define a job entrypoint.
 */
#define JOB_ENTRY_POINT(NAME) static void NAME(i32 jobParam, void* jobData)
