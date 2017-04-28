#include "client/window.h"

#include "catch.hpp"

#include "core/concurrency.h"
#include "core/timer.h"
#include "core/vector.h"
#include "job/manager.h"

using namespace Core;

namespace
{
	Core::Mutex loggingMutex_;

	void CalculatePrimes(i64 max)
	{
		Vector<i64> primesFound;
		i64 numToTest = 2;
		while(primesFound.size() < max)
		{
			bool isPrime = true;
			for(i64 i = 2; i < numToTest; ++i)
			{
				i64 div = numToTest / i;
				if((div * i) == numToTest)
				{
					isPrime = false;
					break;
				}
			}
			if(isPrime)
				primesFound.push_back(numToTest);
			numToTest++;
		}
	}

	void RunJobTest(i32 numJobs, const char* name, bool log = true)
	{
		Core::Vector<i32> jobDatas;
		jobDatas.resize(numJobs, 0);

		Core::Vector<Job::JobDesc> jobDescs;
		jobDescs.reserve(numJobs);
		for(i32 i = 0; i < numJobs; ++i)
		{
			Job::JobDesc jobDesc;
			jobDesc.func_ = [](i32 param, void* data) {
				CalculatePrimes(100);
				*(i32*)data = param;
			};
			jobDesc.param_ = i + 1;
			jobDesc.data_ = &jobDatas[i];
			jobDesc.name_ = "primeCalculateJob";
			jobDescs.push_back(jobDesc);
		}

		Job::Counter* counter = nullptr;

		Timer timer;
		timer.Mark();
		Job::Manager::RunJobs(jobDescs.data(), jobDescs.size(), &counter);
		double runTime = timer.GetTime();
		Job::Manager::WaitForCounter(counter, 0);
		double time = timer.GetTime();
		if(log)
		{
			Core::ScopedMutex lock(loggingMutex_);
			Core::Log("\"%s\"\n", name);
			Core::Log("\tRubJobs: %f ms (%f ms. avg)\n", runTime * 1000.0, runTime * 1000.0 / (double)numJobs);
			Core::Log("\tWaitForCounter: %f ms (%f ms. avg)\n", (time - runTime) * 1000.0, (time - runTime) * 1000.0 / (double)numJobs);
			Core::Log("\tTotal: %f ms (%f ms. avg)\n", time * 1000.0, time * 1000.0 / (double)numJobs);
			for(i32 i = 0; i < numJobs; ++i)
				REQUIRE(jobDatas[i] == jobDescs[i].param_);
		}
	}

	void RunJobTest2(i32 numJobs, const char* name)
	{
		struct JobData
		{
			i32 jobsToLaunch = 1;
		};
		Core::Vector<JobData> jobDatas;
		jobDatas.resize(numJobs);
		for(i32 i = 0; i < numJobs; ++i)
		{
			jobDatas[i].jobsToLaunch = (i / 8) + 1;
		}

		Core::Vector<Job::JobDesc> jobDescs;
		jobDescs.reserve(numJobs);
		for(i32 i = 0; i < numJobs; ++i)
		{
			Job::JobDesc jobDesc;
			jobDesc.func_ = [](i32 param, void* data) {
				auto jobData = reinterpret_cast<JobData*>(data);
				RunJobTest(jobData->jobsToLaunch, "testJobRecursive");
			};
			jobDesc.param_ = i + 1;
			jobDesc.data_ = &jobDatas[i];
			jobDesc.name_ = "testJob2";
			jobDescs.push_back(jobDesc);
		}

		Job::Counter* counter = nullptr;

		Timer timer;
		timer.Mark();
		Job::Manager::RunJobs(jobDescs.data(), jobDescs.size(), &counter);
		double runTime = timer.GetTime();
		Job::Manager::WaitForCounter(counter, 0);
		double time = timer.GetTime();
		Core::Log("\"%s\"\n", name);
		Core::Log("\tRubJobs: %f ms (%f ms. avg)\n", runTime * 1000.0, runTime * 1000.0 / (double)numJobs);
		Core::Log("\tWaitForCounter: %f ms (%f ms. avg)\n", (time - runTime) * 1000.0, (time - runTime) * 1000.0 / (double)numJobs);
		Core::Log("\tTotal: %f ms (%f ms. avg)\n", time * 1000.0, time * 1000.0 / (double)numJobs);
	}


	static const i32 MAX_FIBERS = 128;
	static const i32 MAX_JOBS = 512;
	static const i32 FIBER_STACK_SIZE = 16 * 1024;
}

TEST_CASE("job-tests-create-st-1")
{
	Job::Manager::Scoped manager(1, MAX_FIBERS, FIBER_STACK_SIZE);
}

TEST_CASE("job-tests-create-mt-4")
{
	Job::Manager::Scoped manager(4, MAX_FIBERS, FIBER_STACK_SIZE);
}

TEST_CASE("job-tests-run-job-1-st-1")
{
	Job::Manager::Scoped manager(1, MAX_FIBERS, FIBER_STACK_SIZE);
	RunJobTest( 1, "job-tests-run-job-1-st-1");
}

TEST_CASE("job-tests-run-job-1-mt-4")
{
	Job::Manager::Scoped manager(4, MAX_FIBERS, FIBER_STACK_SIZE);
	RunJobTest(1, "job-tests-run-job-1-mt-4");
}

TEST_CASE("job-tests-run-job-1-mt-8")
{
	Job::Manager::Scoped manager(8, MAX_FIBERS, FIBER_STACK_SIZE);
	RunJobTest(1, "job-tests-run-job-1-mt-8");
}

TEST_CASE("job-tests-run-job-100-st-1")
{
	Job::Manager::Scoped manager(1, MAX_FIBERS, FIBER_STACK_SIZE);
	RunJobTest(100, "job-tests-run-job-100-st-1");
}

TEST_CASE("job-tests-run-job-100-mt-4")
{
	Job::Manager::Scoped manager(4, MAX_FIBERS, FIBER_STACK_SIZE);
	RunJobTest(100, "job-tests-run-job-100-mt-4");
}

TEST_CASE("job-tests-run-job-100-mt-8")
{
	Job::Manager::Scoped manager(4, MAX_FIBERS, FIBER_STACK_SIZE);
	RunJobTest(100, "job-tests-run-job-100-mt-8");
}

TEST_CASE("job-tests-run-job-1000-st-1")
{
	Job::Manager::Scoped manager(1, MAX_FIBERS, FIBER_STACK_SIZE);
	RunJobTest(1000, "job-tests-run-job-1000-st-1");
}

TEST_CASE("job-tests-run-job-1000-mt-4")
{
	Job::Manager::Scoped manager(4, MAX_FIBERS, FIBER_STACK_SIZE);
	RunJobTest(1000, "job-tests-run-job-1000-mt-4");
}

TEST_CASE("job-tests-run-job-1000-mt-8")
{
	Job::Manager::Scoped manager(8, MAX_FIBERS, FIBER_STACK_SIZE);
	RunJobTest(1000, "job-tests-run-job-1000-mt-8");
}

TEST_CASE("job-tests-run-job-1000-mt-4-fiber-blocked")
{
	Job::Manager::Scoped manager(4, 2, FIBER_STACK_SIZE);
	RunJobTest(1000, "job-tests-run-job-100-mt-4-fiber-blocked");
}

TEST_CASE("job-tests-run-job-1000-mt-8-fiber-blocked")
{
	Job::Manager::Scoped manager(8, 4, FIBER_STACK_SIZE);
	RunJobTest(1000, "job-tests-run-job-100-mt-8-fiber-blocked");
}

TEST_CASE("job-tests-run-job-recursive-1-mt-1")
{
	Job::Manager::Scoped manager(1, MAX_FIBERS, FIBER_STACK_SIZE);
	RunJobTest2(1, "job-tests-run-job-recursive-1-mt-1");
}

TEST_CASE("job-tests-run-job-recursive-10-mt-1")
{
	Job::Manager::Scoped manager(1, MAX_FIBERS, FIBER_STACK_SIZE);
	RunJobTest2(10, "job-tests-run-job-recursive-10-mt-1");
}

TEST_CASE("job-tests-run-job-recursive-100-mt-1")
{
	Job::Manager::Scoped manager(1, MAX_FIBERS, FIBER_STACK_SIZE);
	RunJobTest2(100, "job-tests-run-job-recursive-100-mt-1");
}

TEST_CASE("job-tests-run-job-recursive-1-mt-4")
{
	Job::Manager::Scoped manager(4, MAX_FIBERS, FIBER_STACK_SIZE);
	RunJobTest2(1, "job-tests-run-job-recursive-1-mt-4");
}

TEST_CASE("job-tests-run-job-recursive-2-mt-4")
{
	Job::Manager::Scoped manager(4, MAX_FIBERS, FIBER_STACK_SIZE);
	RunJobTest2(10, "job-tests-run-job-recursive-10-mt-4");
}

TEST_CASE("job-tests-run-job-recursive-3-mt-4")
{
	Job::Manager::Scoped manager(4, MAX_FIBERS, FIBER_STACK_SIZE);
	RunJobTest2(100, "job-tests-run-job-recursive-100-mt-4");
}

TEST_CASE("job-tests-run-job-recursive-1-mt-8")
{
	Job::Manager::Scoped manager(8, MAX_FIBERS, FIBER_STACK_SIZE);
	RunJobTest2(1, "job-tests-run-job-recursive-1-mt-8");
}

TEST_CASE("job-tests-run-job-recursive-2-mt-8")
{
	Job::Manager::Scoped manager(8, MAX_FIBERS, FIBER_STACK_SIZE);
	RunJobTest2(10, "job-tests-run-job-recursive-10-mt-8");
}

TEST_CASE("job-tests-run-job-recursive-3-mt-8")
{
	Job::Manager::Scoped manager(8, MAX_FIBERS, FIBER_STACK_SIZE);
	RunJobTest2(100, "job-tests-run-job-recursive-100-mt-8");
}
