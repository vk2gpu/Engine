#pragma once

#include "core/types.h"
#include "job/types.h"

namespace Job
{
	class JOB_DLL Manager final
	{
	public:
		/**
		 * Create job manager.
		 * @param numWorkers Number of workers to create.
		 * @param numFibers Number of fibers to allocate.
		 * @param fiberStackSize Stack size for each fiber.
		 */
		Manager(i32 numWorkers, i32 numFibers, i32 fiberStackSize);
		~Manager();

		/**
		 * Run jobs.
		 * @param jobDescs Jobs to run.
		 * @param numJobDesc Number of jobs to run.
		 * @param counter Counter for how many jobs are currently pending completion.
		 * @pre jobDescs != nullptr.
		 * @pre numJobDesc > 0.
		  */
		void RunJobs(JobDesc* jobDescs, i32 numJobDesc, Counter** counter = nullptr);

		/**
		 * Wait for counter.
		 * If @a value is zero, then it will free once complete. 
		 * @param counter Counter to wait on.
		 * @param value Value to wait on.
		 */
		void WaitForCounter(Counter* counter, i32 value);
		

	private:
		Manager(const Manager&) = delete;
		struct ManagerImpl* impl_ = nullptr;
	};
} // namespace Job
