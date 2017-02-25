#include "job/manager.h"
#include "core/concurrency.h"
#include "core/mpmc_bounded_queue.h"
#include "core/timer.h"
#include "core/vector.h"

#include <utility>

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
				if(fiber->job_.counter_)
				{
					u32 counter = Core::AtomicDec(&fiber->job_.counter_->value_);
					if(counter == 0)
					{
						fiber->job_.counter_->event_->Signal();
					}
				}

				fiber->job_.func_ = nullptr;

				Core::AtomicDec(&fiber->manager_->jobCount_);

				// Switch back to worker.
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

		void SwitchTo(Core::Fiber* workerFiber)
		{
			workerFiber_ = workerFiber;
			fiber_.SwitchTo();
		}

		ManagerImpl* manager_ = nullptr;
		Core::Fiber fiber_;
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
			thread_ = Core::Thread(ThreadEntryPoint, this);
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
					jobFiber->SwitchTo(&workerFiber);
					worker->manager_->ReleaseFiber(jobFiber, jobFiber->job_.func_ == nullptr);
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
		bool exiting_ = false;
		bool exited_ = false;
	};

	bool ManagerImpl::GetFiber(Fiber** outFiber)
	{
		*outFiber = nullptr;

		// First check for waiting fibers.
		Fiber* fiber = nullptr;
		if(waitingFibers_.Dequeue(fiber))
		{
			*outFiber = fiber;
			return true;
		}

		// No waiting fibers, check pending jobs.
		JobDesc job;
		if(pendingJobs_.Dequeue(job))
		{
			DBG_ASSERT(job.func_);

#if !defined(FINAL)
			double startTime = Core::Timer::GetAbsoluteTime();
			const double LOG_TIME_THRESHOLD = 100.0f / 1000000.0; // 100us.
			const double LOG_TIME_REPEAT = 10000.0f / 1000.0;     // 1000ms.
			double nextLogTime = startTime + LOG_TIME_THRESHOLD;
#endif
			while(!freeFibers_.Dequeue(fiber))
			{
#if !defined(FINAL)
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
			}

			*outFiber = fiber;
			fiber->SetJob(job);
		}

		return !exiting_;
	}

	void ManagerImpl::ReleaseFiber(Fiber* fiber, bool complete)
	{
		if(complete)
		{
			freeFibers_.Enqueue(fiber);
		}
		else
		{
			waitingFibers_.Enqueue(fiber);
		}
	}


	Manager::Manager(i32 numWorkers, i32 numFibers, i32 maxJobs, i32 fiberStackSize)
	{
		DBG_ASSERT(numWorkers > 0);
		DBG_ASSERT(numFibers > 0);
		DBG_ASSERT(fiberStackSize > (4 * 1024));

		impl_ = new ManagerImpl();
		impl_->workers_.reserve(numWorkers);
		impl_->freeFibers_ = Core::MPMCBoundedQueue<class Fiber*>(numFibers);
		impl_->waitingFibers_ = Core::MPMCBoundedQueue<class Fiber*>(numFibers);
		impl_->pendingJobs_ = Core::MPMCBoundedQueue<JobDesc>(maxJobs);
		impl_->fiberStackSize_ = fiberStackSize;

		for(i32 i = 0; i < numWorkers; ++i)
		{
			impl_->workers_.emplace_back(new Worker(impl_, i));
		}
		for(i32 i = 0; i < numFibers; ++i)
		{
			bool retVal = impl_->freeFibers_.Enqueue(new Fiber(impl_));
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
				fiber->SwitchTo(nullptr);
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

	void Manager::RunJobs(JobDesc* jobDescs, i32 numJobDesc, Counter** waitCounter)
	{
		if(waitCounter)
		{
			// TODO: Pool.
			(*waitCounter) = new Counter;
			(*waitCounter)->value_ = numJobDesc;
			(*waitCounter)->event_ = new Core::Event(true, false);

			// Assign counter to incoming jobs.
			for(i32 i = 0; i < numJobDesc; ++i)
			{
				DBG_ASSERT(jobDescs[i].counter_ == nullptr);
				jobDescs[i].counter_ = *waitCounter;
			}
		}
		Core::AtomicAdd(&impl_->jobCount_, numJobDesc);

// Push jobs into pending job queue ready to be given fibers.
#if !defined(FINAL)
		double startTime = Core::Timer::GetAbsoluteTime();
		const double LOG_TIME_THRESHOLD = 100.0f / 1000000.0; // 100us.
		const double LOG_TIME_REPEAT = 10000.0f / 1000.0;     // 1000ms.
		double nextLogTime = startTime + LOG_TIME_THRESHOLD;
#endif
		for(i32 i = 0; i < numJobDesc; ++i)
		{
			while(!impl_->pendingJobs_.Enqueue(jobDescs[i]))
			{
#if !defined(FINAL)
				double time = Core::Timer::GetAbsoluteTime();
				if((time - startTime) > LOG_TIME_THRESHOLD)
				{
					if(time > nextLogTime)
					{
						Core::Log("Unable to enqueue job, waiting. Increase maxJobs. (Total time waiting: %f ms)\n",
						    (time - startTime) * 1000.0);
						nextLogTime = time + LOG_TIME_REPEAT;
					}
				}
#endif
				Core::SwitchThread();
			}
		}
	}

	void Manager::WaitForCounter(Counter* counter, i32 value)
	{
		DBG_ASSERT(counter);

		// If we're waiting from within a fiber, we need to move it into the waiting list.
		if(Core::Fiber::InFiber())
		{
			DBG_BREAK;
		}
		else
		{
			if(value == 0)
			{
				counter->event_->Wait();
			}
			else
			{
				while(counter->value_ > value)
				{
					Core::SwitchThread();
				}
			}

			// Free counter.
			// TODO: Keep a pool of these?
			if(value == 0)
			{
				delete counter->event_;
				delete counter;
			}
		}
	}

} // namespace Job
