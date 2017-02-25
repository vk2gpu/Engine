#include "job/manager.h"
#include "core/concurrency.h"
#include "core/vector.h"

#include <utility>

namespace Job
{

	/**
	 * Private manager implementation.
	 */
	struct ManagerImpl
	{
		/// Worker pool.
		Core::Vector<class Worker*> workers_;
		/// Free fibers.
		Core::Vector<class Fiber*> freeFibers_;
		/// Waiting fibers.
		Core::Vector<class Fiber*> waitingFibers_;
		/// Jobs. TODO USE A MPMC QUEUE INSTEAD.
		Core::Vector<JobDesc> pendingJobs_;
		/// TODO: MPMC queues.
		Core::Mutex mutex_;
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
					Core::AtomicDec(&fiber->job_.counter_->value_);
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
		Worker(ManagerImpl* manager)
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
				Core::Yield();
			}
			while(!worker->exiting_)
				Core::Yield();
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

		// TODO: MPMC queues.
		Core::ScopedMutex lock(mutex_);

		// First check for waiting fibers.
		if(waitingFibers_.size() > 0)
		{
			*outFiber = waitingFibers_.front();
			waitingFibers_.erase(waitingFibers_.begin());
			return true;
		}

		// No waiting fibers, check pending jobs.
		if(pendingJobs_.size() > 0)
		{
			JobDesc job = pendingJobs_.front();
			pendingJobs_.erase(pendingJobs_.begin());

			// If we hit this busy-wait, more fibers are needed.
			while(freeFibers_.size() == 0)
				Core::Yield();

			// Grab a fiber.
			*outFiber = freeFibers_.back();
			freeFibers_.pop_back();		

			(*outFiber)->SetJob(job);
		}

		return !exiting_;
	}

	void ManagerImpl::ReleaseFiber(Fiber* fiber, bool complete)
	{
		// TODO: MPMC queues.
		Core::ScopedMutex lock(mutex_);

		if(complete)
		{
			freeFibers_.push_back(fiber);
		}
		else
		{
			// TODO: Push front.
			waitingFibers_.push_back(fiber);
		}
	}


	Manager::Manager(i32 numWorkers, i32 numFibers, i32 fiberStackSize)
	{
		DBG_ASSERT(numWorkers > 0);
		DBG_ASSERT(numFibers > 0);
		DBG_ASSERT(fiberStackSize > (4 * 1024));

		impl_ = new ManagerImpl();
		impl_->workers_.reserve(numWorkers);
		impl_->freeFibers_.reserve(numFibers);
		impl_->waitingFibers_.reserve(numFibers);
		impl_->fiberStackSize_ = fiberStackSize;

		for(i32 i = 0; i < numWorkers; ++i)
		{
			impl_->workers_.emplace_back(new Worker(impl_));
		}
		for(i32 i = 0; i < numFibers; ++i)
		{
			impl_->freeFibers_.emplace_back(new Fiber(impl_));
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
				Core::Yield();

			DBG_ASSERT(impl_->waitingFibers_.size() == 0);

			// Ensure all fibers exit.
			for(auto* fiber : impl_->freeFibers_)
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
		// TODO MPMC
		Core::ScopedMutex lock(impl_->mutex_);

		if(waitCounter)
		{
			// TODO: Pool.
			(*waitCounter) = new Counter;
			(*waitCounter)->value_ = numJobDesc;

			// Assign counter to incoming jobs.
			for(i32 i = 0; i < numJobDesc; ++i)
			{
				DBG_ASSERT(jobDescs[i].counter_ == nullptr);
				jobDescs[i].counter_ = *waitCounter;
			}
		}
		Core::AtomicAdd(&impl_->jobCount_, numJobDesc);

		// Push jobs into pending job queue ready to be given fibers.
		impl_->pendingJobs_.insert(jobDescs, jobDescs + numJobDesc);
	}

	void Manager::WaitForCounter(Counter* counter, i32 value)
	{
		DBG_ASSERT(counter);
		// TODO: If call is coming from a fiber, push into waiting list and switch back to worker.
		while(counter->value_ > value)
		{
			Core::Yield();
		}

		// Free counter.
		// TODO: Keep a pool of these?
		if(value == 0)
		{
			delete counter;
		}
	}

} // namespace Job
