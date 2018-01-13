#pragma once

#include <cmath>

namespace Math
{

	MATH_DLL_INLINE f32 DecibelsToPowerRatio(f32 db) { return std::powf(10.0f, db / 10.0f); }

	MATH_DLL_INLINE f32 PowerRatioToDecibels(f32 r) { return 10.0f * std::log10f(r); }

	MATH_DLL_INLINE f32 DecibelsToAmplitudeRatio(f32 db) { return std::powf(10.0f, db / 20.0f); }

	MATH_DLL_INLINE f32 AmplitudeRatioToDecibels(f32 r) { return 20.0f * std::log10f(r); }


} // namespace Math
