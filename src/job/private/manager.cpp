#include "job/manager.h"
#include "core/concurrency.h"
#include "core/mpmc_bounded_queue.h"
#include "core/timer.h"
#include "core/vector.h"

#include <utility>

#define VERBOSE_LOGGING (0)

namespace Job
{
	/**
	 * Counter internal details.
	 */
	struct Counter final
	{
		volatile i32 value_ = 0;
		Core::Event* event_ = nullptr;

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
		Core::MPMCBoundedQueue<class Fiber*> waitingFibers_;
		/// Jobs.
		Core::MPMCBoundedQueue<JobDesc> pendingJobs_;
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

		bool GetFiber(Fiber** outFiber);
		void ReleaseFiber(Fiber* fiber, bool complete);
	};

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
					fiber->job_.counter_->event_->Signal();
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
		{
			// Create thread.
			thread_ = Core::Thread(ThreadEntryPoint, this, Core::Thread::DEFAULT_STACK_SIZE, "Job Worker Thread");
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

			// Grab fiber from manager to execute.
			Job::Fiber* jobFiber = nullptr;
			while(worker->manager_->GetFiber(&jobFiber))
			{
				if(jobFiber)
				{
					// Reset moveToWaiting_.
					Core::AtomicExchg(&worker->moveToWaiting_, 0);
					jobFiber->SwitchTo(worker, &workerFiber);

					// Reset moveToWaiting, if it was set, move fiber to waiting.
					bool complete = !Core::AtomicExchg(&worker->moveToWaiting_, 0);
					DBG_ASSERT(jobFiber->job_.func_ || complete);
					complete |= jobFiber->job_.func_ == nullptr;
					worker->manager_->ReleaseFiber(jobFiber, complete);
				}
				Core::SwitchThread();
			}
			while(!worker->exiting_)
				Core::SwitchThread();
			worker->exited_ = true;
			return 0;
		}

		ManagerImpl* manager_ = nullptr;
		Core::Thread thread_;
		volatile i32 moveToWaiting_ = 0;
		bool exiting_ = false;
		bool exited_ = false;
	};

	bool ManagerImpl::GetFiber(Fiber** outFiber)
	{
		Fiber* fiber = nullptr;
		*outFiber = nullptr;

		// Check pending jobs.
		JobDesc job;
		if(pendingJobs_.Dequeue(job))
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

		// First check for waiting fibers.
		if(waitingFibers_.Dequeue(fiber))
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
			while(!waitingFibers_.Enqueue(fiber))
			{
#if VERBOSE_LOGGING >= 1
				Core::Log("Unable to enqueue waiting fiber.\n");
#endif
				Core::SwitchThread();
			}
#ifdef DEBUG
			Core::AtomicInc(&numWaitingFibers_);
#endif
		}
	}

	Manager::Manager(i32 numWorkers, i32 numFibers, i32 fiberStackSize)
	{
		DBG_ASSERT(numWorkers > 0);
		DBG_ASSERT(numFibers > 0);
		DBG_ASSERT(fiberStackSize > (4 * 1024));

		impl_ = new ManagerImpl();
		impl_->workers_.reserve(numWorkers);
		impl_->freeFibers_ = Core::MPMCBoundedQueue<class Fiber*>(numFibers);
		impl_->waitingFibers_ = Core::MPMCBoundedQueue<class Fiber*>(numFibers);
		impl_->pendingJobs_ = Core::MPMCBoundedQueue<JobDesc>(numFibers);
		impl_->fiberStackSize_ = fiberStackSize;

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

	Manager::~Manager()
	{
		impl_->exiting_ = true;

		// Wait for jobs to complete, and exit all fibers.
		{
			Core::Fiber exitFiber(Core::Fiber::THIS_THREAD, "Job Manager Deletion Fiber");
			DBG_ASSERT(exitFiber);

			while(impl_->jobCount_ > 0)
				Core::SwitchThread();

			Fiber* fiber = nullptr;
			DBG_ASSERT(!impl_->waitingFibers_.Dequeue(fiber));

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
	}

	void Manager::RunJobs(JobDesc* jobDescs, i32 numJobDesc, Counter** counter)
	{
		// Setup counter.
		auto* localCounter = new Counter();
		localCounter->value_ = numJobDesc;
		localCounter->event_ = new Core::Event(true, false);

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

			while(!impl_->pendingJobs_.Enqueue(jobDescs[i]))
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

#ifdef DEBUG
			Core::AtomicInc(&impl_->numPendingJobs_);
#endif
		}

		// If no counter is specified, wait on it.
		if(counter == nullptr)
		{
			WaitForCounter(localCounter, 0);
		}
		else
		{
			*counter = localCounter;
		}
	}

	void Manager::WaitForCounter(Counter*& counter, i32 value)
	{
		DBG_ASSERT(counter);

		while(counter->value_ > value)
		{
			YieldCPU();
		}

		// Free counter.
		// TODO: Keep a pool of these?
		if(value == 0)
		{
			delete counter->event_;
			counter->event_ = nullptr;
			delete counter;
		}
	}

	void Manager::YieldCPU()
	{
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

} // namespace Job
