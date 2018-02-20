#include "job/manager.h"
#include "core/concurrency.h"
#include "core/misc.h"
#include "core/mpmc_bounded_queue.h"
#include "core/string.h"
#include "core/timer.h"
#include "core/vector.h"

#include "Remotery.h"

#include <utility>

#define VERBOSE_LOGGING (0)
#define ENABLE_JOB_PROFILER (!defined(_RELEASE))

namespace Job
{
	// 100ms semaphore timeout.
	static const i32 WORKER_SEMAPHORE_TIMEOUT = 100;

	/**
	 * Counter internal details.
	 */
	struct Counter final
	{
		/// Counter value. Decreases as job completes.
		volatile i32 value_ = 0;

		Counter() = default;
		Counter(const Counter&) = delete;
	};


	/**
	 * Private manager implementation.
	 */
	struct ManagerImpl
	{
		/// Worker pool.
		Core::Vector<class Worker*> workers_;
		/// Free fibers.
		Core::MPMCBoundedQueue<class Fiber*> freeFibers_;
		/// Waiting fibers.
		Core::Array<Core::MPMCBoundedQueue<class Fiber*>, (i32)Priority::MAX> waitingFibers_;
		/// Jobs.
		Core::Array<Core::MPMCBoundedQueue<JobDesc>, (i32)Priority::MAX> pendingJobs_;
		/// Out of fibers counter.
		volatile i32 outOfFibers_ = 0;
#ifdef DEBUG
		/// Number of free fibers. ONLY FOR DEBUG PURPOSES.
		volatile i32 numFreeFibers_ = 0;
		/// Number of waiting fibers. ONLY FOR DEBUG PURPOSES.
		volatile i32 numWaitingFibers_ = 0;
		/// Number of pending jobs. ONLY FOR DEBUG PURPOSES.
		volatile i32 numPendingJobs_ = 0;
#endif
		/// Fiber stack size.
		i32 fiberStackSize_ = 0;
		/// Are we exiting?
		bool exiting_ = false;
		/// How many jobs are in flight.
		volatile i32 jobCount_ = 0;

#if ENABLE_JOB_PROFILER
		/// Is job profiling enabled?
		volatile i32 profilerEnabled_ = 0;
		/// Is job profliing running?
		volatile i32 profilerRunning_ = 0;
		/// Entry index.
		volatile i32 profilerEntryIdx_ = 0;
		/// Job index.
		volatile i32 profilerJobIdx_ = 0;

		struct PaddedProfilerEntry
		{
			ProfilerEntry data_;

			// Pad up to at least the size of a cache line to avoid false sharing when writing the entries.
			u8 padding_[CACHE_LINE_SIZE];
		};

		Core::Vector<PaddedProfilerEntry> profilerEntries_;
#endif // ENABLE_JOB_PROFILER

		/// Semaphore for workers to wait on.
		Core::Semaphore scheduleSem_ = Core::Semaphore(0, 65536);

		bool GetFiber(Fiber** outFiber);
		void ReleaseFiber(Fiber* fiber, bool complete);
	};

	ManagerImpl* impl_ = nullptr;

	class Fiber final
	{
	public:
		Fiber(ManagerImpl* manager)
		    : manager_(manager)
		{
			fiber_ = Core::Fiber(FiberEntryPoint, this, manager_->fiberStackSize_, "Job Fiber");
		}

		static void FiberEntryPoint(void* param)
		{
			auto* fiber = reinterpret_cast<Fiber*>(param);
			DBG_ASSERT(fiber);

			while(!fiber->exiting_ || fiber->workerFiber_)
			{
				DBG_ASSERT(fiber->job_.func_);

				// Execute job.
				fiber->job_.func_(fiber->job_.param_, fiber->job_.data_);

				// Tick counter down.
				u32 counter = Core::AtomicDec(&fiber->job_.counter_->value_);
				if(counter == 0)
				{
					if(fiber->job_.freeCounter_)
						delete fiber->job_.counter_;
				}

				fiber->job_.func_ = nullptr;

				Core::AtomicDec(&fiber->manager_->jobCount_);

				// Switch back to worker.
				DBG_ASSERT(fiber->worker_);
				DBG_ASSERT(fiber->workerFiber_);
				fiber->workerFiber_->SwitchTo();
			}

			// Validate exit conditions.
			DBG_ASSERT(fiber->exiting_);
			DBG_ASSERT(fiber->workerFiber_ == nullptr);

			fiber->exited_ = true;
		}

		void SetJob(const JobDesc& job)
		{
			DBG_ASSERT(job_.func_ == nullptr);
			job_ = job;
		}

		void SwitchTo(class Worker* worker, Core::Fiber* workerFiber)
		{
			worker_ = worker;
			workerFiber_ = workerFiber;
			fiber_.SwitchTo();
		}

		ManagerImpl* manager_ = nullptr;
		Core::Fiber fiber_;
		class Worker* worker_ = nullptr;
		Core::Fiber* workerFiber_ = nullptr;
		JobDesc job_;
		bool exiting_ = false;
		bool exited_ = false;
	};


	class Worker final
	{
	public:
		Worker(ManagerImpl* manager, i32 idx)
		    : manager_(manager)
		    , idx_(idx)
		{
			// Create thread.
			auto debugName = Core::String().Printf("Job Worker Thread %i", idx);
			thread_ = Core::Thread(ThreadEntryPoint, this, Core::Thread::DEFAULT_STACK_SIZE, debugName.c_str());

			// Set thread affinity to physical cores.
			const i32 numPhysCores = Core::GetNumPhysicalCores();
			if(u64 mask = Core::GetPhysicalCoreAffinityMask(idx) % numPhysCores)
				thread_.SetAffinity(mask);
		}

		~Worker()
		{
			// Wait for thread to complete.
			i32 retVal = thread_.Join();
			DBG_ASSERT(retVal == 0);
		}

		static i32 ThreadEntryPoint(void* param)
		{
			auto* worker = reinterpret_cast<Worker*>(param);
			DBG_ASSERT(worker);

			// Fiber for the worker thread.
			Core::Fiber workerFiber(Core::Fiber::THIS_THREAD, "Job Worker Fiber");
			DBG_ASSERT(workerFiber);

			ManagerImpl* manager = worker->manager_;

#if ENABLE_JOB_PROFILER
			ProfilerEntry profilerEntry;
			i32 profilerEntryIdx = -1;
#endif

			// Grab fiber from manager to execute.
			Job::Fiber* jobFiber = nullptr;
			while(manager->GetFiber(&jobFiber))
			{
				if(jobFiber)
				{
#if ENABLE_JOB_PROFILER
					if(impl_->profilerRunning_)
					{
						profilerEntryIdx = Core::AtomicInc(&impl_->profilerEntryIdx_) - 1;
						if(profilerEntryIdx >= impl_->profilerEntries_.size())
							profilerEntryIdx = -1;
					}

					if(profilerEntryIdx >= 0)
					{
						profilerEntry.workerIdx_ = worker->idx_;
						profilerEntry.jobIdx_ = jobFiber->job_.idx_;
						profilerEntry.startTime_ = Core::Timer::GetAbsoluteTime();
						profilerEntry.name_[0] = '\0';
						sprintf_s(profilerEntry.name_.data(), profilerEntry.name_.size(), "%s (%i)",
						    jobFiber->job_.name_, jobFiber->job_.param_);
						profilerEntry.param_ = jobFiber->job_.param_;
					}
#endif // ENABLE_JOB_PROFILER

					// Reset moveToWaiting_.
					Core::AtomicExchg(&worker->moveToWaiting_, 0);
					jobFiber->SwitchTo(worker, &workerFiber);

#if ENABLE_JOB_PROFILER
					if(profilerEntryIdx >= 0)
					{
						profilerEntry.endTime_ = Core::Timer::GetAbsoluteTime();
						manager->profilerEntries_[profilerEntryIdx].data_ = profilerEntry;
					}
#endif // ENABLE_JOB_PROFILER

					// Reset moveToWaiting, if it was set, move fiber to waiting.
					bool complete = !Core::AtomicExchg(&worker->moveToWaiting_, 0);
					DBG_ASSERT(jobFiber->job_.func_ || complete);
					complete |= jobFiber->job_.func_ == nullptr;
					manager->ReleaseFiber(jobFiber, complete);
				}
				else
				{
					manager->scheduleSem_.Wait(WORKER_SEMAPHORE_TIMEOUT);
				}
#if ENABLE_JOB_PROFILER
				profilerEntryIdx = -1;
#endif
			}
			while(!worker->exiting_)
				Core::SwitchThread();
			worker->exited_ = true;
			return 0;
		}

		ManagerImpl* manager_ = nullptr;
		i32 idx_ = -1;
		Core::Thread thread_;
		volatile i32 moveToWaiting_ = 0;
		bool exiting_ = false;
		bool exited_ = false;
	};

	bool ManagerImpl::GetFiber(Fiber** outFiber)
	{
		Fiber* fiber = nullptr;
		*outFiber = nullptr;

		for(i32 prio = 0; prio < (i32)Priority::MAX; ++prio)
		{
			JobDesc job;
			if(pendingJobs_[prio].Dequeue(job))
			{
#ifdef DEBUG
				Core::AtomicDec(&numPendingJobs_);
#endif

				DBG_ASSERT(job.func_);

#if VERBOSE_LOGGING >= 1
				double startTime = Core::Timer::GetAbsoluteTime();
				const double LOG_TIME_THRESHOLD = 100.0f / 1000000.0; // 100us.
				const double LOG_TIME_REPEAT = 1000.0f / 1000.0;      // 1000ms.
				double nextLogTime = startTime + LOG_TIME_THRESHOLD;
#endif
				i32 spinCount = 0;
				i32 spinCountMax = 100;
				while(!freeFibers_.Dequeue(fiber))
				{
					++spinCount;
					if(spinCount > spinCountMax)
					{
						Core::AtomicInc(&outOfFibers_);
					}
#if VERBOSE_LOGGING >= 1
					double time = Core::Timer::GetAbsoluteTime();
					if((time - startTime) > LOG_TIME_THRESHOLD)
					{
						if(time > nextLogTime)
						{
							Core::Log("Unable to get free fiber. Increase numFibers. (Total time waiting: %f ms)\n",
							    (time - startTime) * 1000.0);
							nextLogTime = time + LOG_TIME_REPEAT;
						}
					}
#endif
					Core::SwitchThread();

					// If all threads have spun this loop for too long simultaneously,
					// we probably have a deadlock due to exhausting the fiber pool.
					if(spinCount > spinCountMax)
					{
						if((Core::AtomicDec(&outOfFibers_) + 1) == workers_.size())
						{
							static bool breakHere = true;
							if(breakHere)
							{
								DBG_BREAK;
								breakHere = false;
							}
						}
					}
				}

#ifdef DEBUG
				Core::AtomicDec(&numFreeFibers_);
#endif

				*outFiber = fiber;
				fiber->SetJob(job);
#if VERBOSE_LOGGING >= 3
				Core::Log("Pending job \"%s\" (%u) being scheduled.\n", fiber->job_.name_, fiber->job_.param_);
#endif
				return true;
			}

			// Check for waiting fibers that need ran.
			if(waitingFibers_[prio].Dequeue(fiber))
			{
#ifdef DEBUG
				Core::AtomicInc(&numWaitingFibers_);
#endif
				*outFiber = fiber;
				DBG_ASSERT(fiber->job_.func_);
#if VERBOSE_LOGGING >= 3
				Core::Log("Waiting job \"%s\" (%u) being rescheduled.\n", fiber->job_.name_, fiber->job_.param_);
#endif
				return true;
			}
		}

		return !exiting_;
	}

	void ManagerImpl::ReleaseFiber(Fiber* fiber, bool complete)
	{
#if VERBOSE_LOGGING >= 2
		Core::Log("Job %s \"%s\" (%u).\n", complete ? "complete" : "waiting", fiber->job_.name_, fiber->job_.param_);
#endif
		if(complete)
		{
			while(!freeFibers_.Enqueue(fiber))
			{
#if VERBOSE_LOGGING >= 1
				Core::Log("Unable to enqueue free fiber.\n");
#endif
				Core::SwitchThread();
			}
#ifdef DEBUG
			Core::AtomicInc(&numFreeFibers_);
#endif
		}
		else
		{
			const i32 prio = (i32)fiber->job_.prio_;
			while(!waitingFibers_[prio].Enqueue(fiber))
			{
#if VERBOSE_LOGGING >= 1
				Core::Log("Unable to enqueue waiting fiber.\n");
#endif
				Core::SwitchThread();
			}
#ifdef DEBUG
			impl_->scheduleSem_.Signal(1);
			Core::AtomicInc(&numWaitingFibers_);
#endif
		}
	}

	void Manager::Initialize(i32 numWorkers, i32 numFibers, i32 fiberStackSize)
	{
		DBG_ASSERT(impl_ == nullptr);
		DBG_ASSERT(numWorkers > 0);
		DBG_ASSERT(numFibers > 0);
		DBG_ASSERT(fiberStackSize > (4 * 1024));

		impl_ = new ManagerImpl();
		impl_->workers_.reserve(numWorkers);
		impl_->freeFibers_ = Core::MPMCBoundedQueue<class Fiber*>(numFibers);
		for(auto& waitingFibers : impl_->waitingFibers_)
			waitingFibers = Core::MPMCBoundedQueue<class Fiber*>(numFibers);
		for(auto& pendingJobs : impl_->pendingJobs_)
			pendingJobs = Core::MPMCBoundedQueue<JobDesc>(numFibers);
		impl_->fiberStackSize_ = fiberStackSize;
#if ENABLE_JOB_PROFILER
		impl_->profilerEntries_.resize(65536);
#endif

		for(i32 i = 0; i < numWorkers; ++i)
		{
			impl_->workers_.emplace_back(new Worker(impl_, i));
		}
		for(i32 i = 0; i < numFibers; ++i)
		{
			bool retVal = impl_->freeFibers_.Enqueue(new Fiber(impl_));
#ifdef DEBUG
			Core::AtomicInc(&impl_->numFreeFibers_);
#endif
			DBG_ASSERT(retVal);
		}
	}

	void Manager::Finalize()
	{
		DBG_ASSERT(impl_);

		impl_->exiting_ = true;
		Core::Barrier();
		impl_->scheduleSem_.Signal(impl_->workers_.size());

		// Wait for jobs to complete, and exit all fibers.
		{
			Core::Fiber exitFiber(Core::Fiber::THIS_THREAD, "Job Manager Deletion Fiber");
			DBG_ASSERT(exitFiber);

			while(impl_->jobCount_ > 0)
				Core::SwitchThread();

			Fiber* fiber = nullptr;
			for(auto& waitingFibers : impl_->waitingFibers_)
				DBG_ASSERT(!waitingFibers.Dequeue(fiber));

			// Ensure all fibers exit.
			while(impl_->freeFibers_.Dequeue(fiber))
			{
				fiber->exiting_ = true;
				fiber->SwitchTo(nullptr, nullptr);
				DBG_ASSERT(fiber->exited_);
				delete fiber;
			}

			// Ensure all threads exit.
			for(auto* worker : impl_->workers_)
			{
				worker->exiting_ = true;
				worker->thread_.Join();
				DBG_ASSERT(worker->exited_);
				delete worker;
			}
		}
		delete impl_;
		impl_ = nullptr;
	}

	bool Manager::IsInitialized() { return !!impl_; }

	void Manager::RunJobs(JobDesc* jobDescs, i32 numJobDesc, Counter** counter)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(counter == nullptr || *counter == nullptr);

		const bool jobShouldFreeCounter = (counter == nullptr);

		// Setup counter.
		auto* localCounter = new Counter();
		localCounter->value_ = numJobDesc;

		Core::AtomicAdd(&impl_->jobCount_, numJobDesc);

// Push jobs into pending job queue ready to be given fibers.
#if VERBOSE_LOGGING >= 1
		double startTime = Core::Timer::GetAbsoluteTime();
		const double LOG_TIME_THRESHOLD = 100.0f / 1000000.0; // 100us.
		const double LOG_TIME_REPEAT = 1000.0f / 1000.0;      // 1000ms.
		double nextLogTime = startTime + LOG_TIME_THRESHOLD;
#endif

		for(i32 i = 0; i < numJobDesc; ++i)
		{
			DBG_ASSERT(jobDescs[i].counter_ == nullptr);
			jobDescs[i].counter_ = localCounter;
			jobDescs[i].freeCounter_ = jobShouldFreeCounter;

#if ENABLE_JOB_PROFILER
			if(impl_->profilerRunning_)
				jobDescs[i].idx_ = Core::AtomicInc(&impl_->profilerJobIdx_) - 1;
#endif
			const auto& jobDesc = jobDescs[i];
			auto& pendingJobs = impl_->pendingJobs_[(i32)jobDesc.prio_];

			while(!pendingJobs.Enqueue(jobDescs[i]))
			{
#if VERBOSE_LOGGING >= 1
				double time = Core::Timer::GetAbsoluteTime();
				if((time - startTime) > LOG_TIME_THRESHOLD)
				{
					if(time > nextLogTime)
					{
						Core::Log("Unable to enqueue job, waiting for free  (Total time waiting: %f ms)\n",
						    (time - startTime) * 1000.0);
						nextLogTime = time + LOG_TIME_REPEAT;
					}
				}
#endif
				YieldCPU();
			}
			impl_->scheduleSem_.Signal(1);

#ifdef DEBUG
			Core::AtomicInc(&impl_->numPendingJobs_);
#endif
		}

		// If counter is requests, store it.
		if(counter != nullptr)
		{
			*counter = localCounter;
		}
	}

	void Manager::WaitForCounter(Counter*& counter, i32 value)
	{
		DBG_ASSERT(IsInitialized());
		if(counter)
		{
			while(counter->value_ > value)
			{
				YieldCPU();
			}

			// Free counter.
			// TODO: Keep a pool of these?
			if(value == 0)
			{
				delete counter;
				counter = nullptr;
			}
		}
	}

	i32 Manager::GetCounterValue(Counter* counter)
	{
		if(counter)
		{
			return counter->value_;
		}
		return 0;
	}

	void Manager::YieldCPU()
	{
		DBG_ASSERT(IsInitialized());

		auto* callingFiber = Core::Fiber::GetCurrentFiber();
		if(callingFiber)
		{
			auto* fiber = reinterpret_cast<Fiber*>(callingFiber->GetUserData());
			DBG_ASSERT(fiber->worker_);
			DBG_ASSERT(fiber->workerFiber_);

#if VERBOSE_LOGGING >= 2
			Core::Log("Yielding job \"%s\"\n", fiber->job_.name_);
#endif

			// Switch back to worker, but set waiting flag.
			Core::AtomicExchg(&fiber->worker_->moveToWaiting_, 1);
			fiber->workerFiber_->SwitchTo();
		}
		else
		{
			Core::SwitchThread();
		}
	}

	void Manager::BeginProfiling()
	{
#if ENABLE_JOB_PROFILER
		DBG_ASSERT(IsInitialized());
		// Enable the profiler.
		i32 wasEnabled = Core::AtomicExchgAcq(&impl_->profilerEnabled_, 1);
		DBG_ASSERT(wasEnabled == 0);
		if(wasEnabled == 1)
			return;

		// Reset profiler entry index. It should already be 0 by this point, but confirm it.
		i32 idx = Core::AtomicExchgAcq(&impl_->profilerEntryIdx_, 0);
		DBG_ASSERT(idx == 0);

		// Reset job index.
		Core::AtomicExchgAcq(&impl_->profilerJobIdx_, 0);

		// Add begin marker.
		idx = Core::AtomicInc(&impl_->profilerEntryIdx_) - 1;
		DBG_ASSERT(idx == 0);

		auto& entry = impl_->profilerEntries_[0].data_;
		strcpy_s(entry.name_.data(), entry.name_.size(), "Profile");
		entry.startTime_ = Core::Timer::GetAbsoluteTime();

		// Enable running.
		i32 wasRunning = Core::AtomicExchgAcq(&impl_->profilerRunning_, 1);
		DBG_ASSERT(wasRunning == 0);
#endif
	}

	i32 Manager::EndProfiling(ProfilerEntry* profilerEntries, i32 maxProfilerEntries)
	{
#if ENABLE_JOB_PROFILER
		DBG_ASSERT(IsInitialized());

		// Stop the profiler from running.
		i32 wasRunning = Core::AtomicExchgAcq(&impl_->profilerRunning_, 0);
		DBG_ASSERT(wasRunning == 1);
		if(wasRunning == 0)
			return 0;

		// End time marker.
		auto& entry = impl_->profilerEntries_[0].data_;
		entry.endTime_ = Core::Timer::GetAbsoluteTime();

		i32 numProfilerEntries = Core::AtomicExchgAcq(&impl_->profilerEntryIdx_, 0);
		numProfilerEntries = Core::Min(numProfilerEntries, impl_->profilerEntries_.size());
		numProfilerEntries = Core::Min(numProfilerEntries, maxProfilerEntries);
		for(i32 idx = 0; idx < numProfilerEntries; ++idx)
		{
			profilerEntries[idx] = impl_->profilerEntries_[idx].data_;
			if(idx != 0 && profilerEntries[idx].workerIdx_ == -1)
			{
				numProfilerEntries = idx;
				break;
			}
		}

		// Now disable the profiler.
		i32 wasEnabled = Core::AtomicExchgAcq(&impl_->profilerEnabled_, 0);
		DBG_ASSERT(wasEnabled == 1);

		return numProfilerEntries;
#else
		return 0;
#endif
	}


} // namespace Job
