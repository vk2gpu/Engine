#pragma once

#include "core/types.h"
#include "job/types.h"

namespace Job
{
	/**
	 * Job manager.
	 * This is based upon the "Parallelizing the Naughty Dog Engine Using Fibers" talk found here:
	 * http://www.gdcvault.com/play/1022186/Parallelizing-the-Naughty-Dog-Engine
	 */
	class JOB_DLL Manager final
	{
	public:
		/**
		 * Initialize job manager.
		 * @param numWorkers Number of workers to create.
		 * @param numFibers Number of fibers to allocate.
		 * @param fiberStackSize Stack size for each fiber.
		 */
		static void Initialize(i32 numWorkers, i32 numFibers, i32 fiberStackSize);

		/**
		 * Shutdown job manager.
		 */
		static void Finalize();

		/**
		 * Is job manager initialized?
		 */
		static bool IsInitialized();

		/**
		 * Run jobs.
		 * @param jobDescs Jobs to run.
		 * @param numJobDesc Number of jobs to run.
		 * @param counter Counter for how many jobs are currently pending completion.
		 * @pre jobDescs != nullptr.
		 * @pre numJobDesc > 0.
		 */
		static void RunJobs(JobDesc* jobDescs, i32 numJobDesc, Counter** counter = nullptr);

		/**
		 * Wait for counter.
		 * If @a value is zero, then it will free once complete.
		 * @param counter Counter to wait on.
		 * @param value Value to wait on.
		 */
		static void WaitForCounter(Counter*& counter, i32 value);

		/**
		 * Yield execution.
		 */
		static void YieldCPU();

		/**
		 * Scoped manager init/fini.
		 * Mostly a convenience for unit tests.
		 */
		class Scoped
		{
		public:
			Scoped(i32 numWorkers, i32 numFibers, i32 fiberStackSize)
			{
				Initialize(numWorkers, numFibers, fiberStackSize);
			}
			~Scoped() { Finalize(); }
		};

	private:
		Manager() = delete;
		~Manager() = delete;
		Manager(const Manager&) = delete;
	};
} // namespace Job
