#include "core/array.h"
#include "math/mat44.h"

#include "math/ispc/mat44_ispc.h"

#include "catch.hpp"

namespace
{
}

TEST_CASE("mat44-tests-mul")
{
	Math::Mat44 a;
	Math::Mat44 b;

	a.Identity();
	b.Identity();

	Math::Mat44 c;

	u64 numIters = 10000000LL;

	{
		u64 begin;
		u64 end;
		begin = __rdtsc();
		for(int i = 0; i < numIters; ++i)
		{
			c = a * b;
		}
		end = __rdtsc();


		u64 cycles = end - begin;
		f64 avgCycles = (f64)cycles / (f64)numIters;

		Core::Log("Default Avg. cycles: %f\n", avgCycles);
	}

	{
		u64 begin;
		u64 end;
		begin = __rdtsc();
		for(int i = 0; i < (numIters); ++i)
		{
			ispc::Mat44_MulArray(1, (ispc::Mat44*)&c, (ispc::Mat44*)&a, (ispc::Mat44*)&b);
		}
		end = __rdtsc();


		u64 cycles = end - begin;
		f64 avgCycles = (f64)cycles / (f64)numIters;

		Core::Log("ISPC Avg. cycles: %f\n", avgCycles);
	}
}

TEST_CASE("mat44-tests-inverse")
{
	Math::Mat44 a;
	Math::Mat44 b;

	a.Identity();
	b.Identity();

	Math::Mat44 c;

	u64 numIters = 10000000LL;

	{
		u64 begin;
		u64 end;
		begin = __rdtsc();
		for(int i = 0; i < numIters; ++i)
		{
			a.Inverse();
		}
		end = __rdtsc();


		u64 cycles = end - begin;
		f64 avgCycles = (f64)cycles / (f64)numIters;

		Core::Log("Default Avg. cycles: %f\n", avgCycles);
	}

	{
		u64 begin;
		u64 end;
		begin = __rdtsc();
		for(int i = 0; i < (numIters); ++i)
		{
			ispc::Mat44_Inverse(1, (ispc::Mat44*)&a, (ispc::Mat44*)&a);
		}
		end = __rdtsc();


		u64 cycles = end - begin;
		f64 avgCycles = (f64)cycles / (f64)numIters;

		Core::Log("ISPC Avg. cycles: %f\n", avgCycles);
	}
}

