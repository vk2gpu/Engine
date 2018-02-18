#include "core/debug.h"
#include "core/random.h"
#include "core/vector.h"
#include "math/utils.h"
#include "math/vec2.h"
#include "catch.hpp"

namespace
{
	const Math::Vec2 POWER_VALUES[] = {
	    {50.0f, 100000.0f}, {40.0f, 10000.0f}, {30.0f, 1000.0f}, {20.0f, 100.0f}, {10.0f, 10.0f}, {0.0f, 1.0f},
	    {-10.0f, 0.1f}, {-20.0f, 0.01f}, {-30.0f, 0.001f}, {-40.0f, 0.0001f}, {-50.0f, 0.00001f},
	};

	const Math::Vec2 AMPLITUDE_VALUES[] = {
	    {100.0f, 100000.0f}, {80.0f, 10000.0f}, {60.0f, 1000.0f}, {40.0f, 100.0f}, {20.0f, 10.0f}, {0.0f, 1.0f},
	    {-20.0f, 0.1f}, {-40.0f, 0.01f}, {-60.0f, 0.001f}, {-80.0f, 0.0001f}, {-100.0f, 0.00001f},
	};

	const f32 DATASET_0[] = {
	    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	};

	const f32 DATASET_0_MEAN = 1.0f;
	const f32 DATASET_0_RMS = 1.0f;
	const f32 DATASET_0_VARIANCE = 0.0f;
	const f32 DATASET_0_VARIANCEP = 0.0f;
	const f32 DATASET_0_VARIANCE_8 = 0.0f;
	const f32 DATASET_0_VARIANCEP_8 = 0.0f;

	const f32 DATASET_1[] = {
	    1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f,
	};

	const f32 DATASET_1_MEAN = 5.5f;
	const f32 DATASET_1_RMS = 6.204836823f;
	const f32 DATASET_1_VARIANCE = 9.166666666076f;
	const f32 DATASET_1_VARIANCEP = 8.250000000000f;
	const f32 DATASET_1_VARIANCE_8 = 8.354430198669f;
	const f32 DATASET_1_VARIANCEP_8 = 8.250000000000f;

	const f32 MAX_ERROR_PCT = 0.001f / 100.0f;

	bool EpsilonTest(const char* label, f32 a, f32 b)
	{
		Core::Log("%s: %.12f, expected: %.12f\n", label, a, b);

		f32 d = std::abs(a - b);
		return d <= (std::abs(a) * MAX_ERROR_PCT);
	}
}

TEST_CASE("utils-tests-decibels")
{
	SECTION("power-ratio")
	{
		for(auto pair : POWER_VALUES)
		{
			Math::Vec2 value = {Math::PowerRatioToDecibels(pair.y), Math::DecibelsToPowerRatio(pair.x)};

			CHECK(EpsilonTest("dB", value.x, pair.x));
			CHECK(EpsilonTest("PR", value.y, pair.y));
		}
	}

	SECTION("amplitude-ratio")
	{
		for(auto pair : AMPLITUDE_VALUES)
		{
			Math::Vec2 value = {Math::AmplitudeRatioToDecibels(pair.y), Math::DecibelsToAmplitudeRatio(pair.x)};

			CHECK(EpsilonTest("dB", value.x, pair.x));
			CHECK(EpsilonTest("AR", value.y, pair.y));
		}
	}
}

TEST_CASE("utils-tests-statistics")
{
	Math::Statistics stat0;
	Math::Statistics stat1;

	for(f32 value : DATASET_0)
		stat0.Add(value);

	for(f32 value : DATASET_1)
		stat1.Add(value);

	CHECK(EpsilonTest("RM0", stat0.GetMean(), DATASET_0_MEAN));
	CHECK(EpsilonTest("RR0", stat0.GetRootMeanSquared(), DATASET_0_RMS));
	CHECK(EpsilonTest("RSV0", stat0.GetSampleVariance(), DATASET_0_VARIANCE));
	CHECK(EpsilonTest("RPV0", stat0.GetPopulationVariance(), DATASET_0_VARIANCEP));

	CHECK(EpsilonTest("RM1", stat1.GetMean(), DATASET_1_MEAN));
	CHECK(EpsilonTest("RR1", stat1.GetRootMeanSquared(), DATASET_1_RMS));
	CHECK(EpsilonTest("RSV1", stat1.GetSampleVariance(), DATASET_1_VARIANCE));
	CHECK(EpsilonTest("RPV1", stat1.GetPopulationVariance(), DATASET_1_VARIANCEP));

	for(i32 i = 1; i < 8; ++i)
		stat0.Add(DATASET_0);

	for(i32 i = 1; i < 8; ++i)
		stat1.Add(DATASET_1);

	CHECK(EpsilonTest("RM0", stat0.GetMean(), DATASET_0_MEAN));
	CHECK(EpsilonTest("RR0", stat0.GetRootMeanSquared(), DATASET_0_RMS));
	CHECK(EpsilonTest("RSV0", stat0.GetSampleVariance(), DATASET_0_VARIANCE_8));
	CHECK(EpsilonTest("RPV0", stat0.GetPopulationVariance(), DATASET_0_VARIANCEP_8));

	CHECK(EpsilonTest("RM1", stat1.GetMean(), DATASET_1_MEAN));
	CHECK(EpsilonTest("RR1", stat1.GetRootMeanSquared(), DATASET_1_RMS));
	CHECK(EpsilonTest("RSV1", stat1.GetSampleVariance(), DATASET_1_VARIANCE_8));
	CHECK(EpsilonTest("RPV1", stat1.GetPopulationVariance(), DATASET_1_VARIANCEP_8));
}
