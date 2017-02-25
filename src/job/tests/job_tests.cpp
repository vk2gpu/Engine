#include "client/window.h"

#include "catch.hpp"

#include "core/timer.h"
#include "core/vector.h"
#include "job/manager.h"

using namespace Core;

namespace
{
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

	void RunJobTest(Job::Manager& jobManager, i32 numJobs, const char* name)
	{
		Core::Vector<i32> jobDatas;
		jobDatas.resize(numJobs, 0);

		Core::Vector<Job::JobDesc> jobDescs;
		jobDescs.reserve(numJobs);
		for(i32 i = 0; i < numJobs; ++i)
		{
			Job::JobDesc jobDesc;
			jobDesc.func_ = [](i32 param, void* data)
			{
				CalculatePrimes(100);
				*(i32*)data = param;
			};
			jobDesc.param_ = i + 1;
			jobDesc.data_ = &jobDatas[i];
			jobDesc.name_ = "testJob";
			jobDescs.push_back(jobDesc);
		}
	
		Job::Counter* counter = nullptr;

		Timer timer;
		timer.Mark();
		jobManager.RunJobs(jobDescs.data(), jobDescs.size(), &counter);
		jobManager.WaitForCounter(counter, 0);
		double time = timer.GetTime();
		Core::Log("\"%s\" Time taken: %f ms (%f ms. avg)\n", name,
			time * 1000.0, time * 1000.0 / (double)numJobs);

		for(i32 i = 0; i < numJobs; ++i)
			REQUIRE(jobDatas[i] == jobDescs[i].param_);
	}
}

TEST_CASE("job-tests-create-st-1")
{
	Job::Manager jobManager(1, 128, 16 * 1024);
}

TEST_CASE("job-tests-create-mt-4")
{
	Job::Manager jobManager(4, 128, 16 * 1024);
}

TEST_CASE("job-tests-run-job-1-st-1")
{
	Job::Manager jobManager(1, 128, 16 * 1024);
	RunJobTest(jobManager, 1, "job-tests-run-job-1-st-1");
}

TEST_CASE("job-tests-run-job-1-mt-4")
{
	Job::Manager jobManager(4, 128, 16 * 1024);
	RunJobTest(jobManager, 1, "job-tests-run-job-1-mt-4");
}

TEST_CASE("job-tests-run-job-1-mt-8")
{
	Job::Manager jobManager(8, 128, 16 * 1024);
	RunJobTest(jobManager, 1, "job-tests-run-job-1-mt-8");
}

TEST_CASE("job-tests-run-job-100-st-1")
{
	Job::Manager jobManager(1, 1, 16 * 1024);
	RunJobTest(jobManager, 100, "job-tests-run-job-100-st-1");
}

TEST_CASE("job-tests-run-job-100-mt-4")
{
	Job::Manager jobManager(4, 128, 16 * 1024);
	RunJobTest(jobManager, 100, "job-tests-run-job-100-mt-4");
}

TEST_CASE("job-tests-run-job-100-mt-8")
{
	Job::Manager jobManager(4, 128, 16 * 1024);
	RunJobTest(jobManager, 100, "job-tests-run-job-100-mt-8");
}

TEST_CASE("job-tests-run-job-10000-st-1")
{
	Job::Manager jobManager(1, 1, 16 * 1024);
	RunJobTest(jobManager, 10000, "job-tests-run-job-10000-st-1");
}

TEST_CASE("job-tests-run-job-10000-mt-4")
{
	Job::Manager jobManager(4, 128, 16 * 1024);
	RunJobTest(jobManager, 10000, "job-tests-run-job-10000-mt-4");
}

TEST_CASE("job-tests-run-job-10000-mt-8")
{
	Job::Manager jobManager(8, 128, 16 * 1024);
	RunJobTest(jobManager, 10000, "job-tests-run-job-10000-mt-8");
}
