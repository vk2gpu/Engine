#pragma once

#include "math/dll.h"
#include "core/types.h"
#include "core/array_view.h"

namespace Math
{
	/**
	 * Calculate power ratio from decibels.
	 */
	MATH_DLL_INLINE f32 DecibelsToPowerRatio(f32 db);

	/**
	 * Calculate decibels from power ratio.
	 */
	MATH_DLL_INLINE f32 PowerRatioToDecibels(f32 r);

	/**
	 * Calculate amplitude ratio from decibels.
	 */
	MATH_DLL_INLINE f32 DecibelsToAmplitudeRatio(f32 db);

	/**
	 * Calculate decibels from amplitude ratio.
	 */
	MATH_DLL_INLINE f32 AmplitudeRatioToDecibels(f32 r);

	/**
	 * Utility for statistics.
	 */
	class MATH_DLL Statistics
	{
	public:
		Statistics() = default;

		/**
		 * Add value.
		 */
		void Add(f32 value);

		/**
		 * Add values.
		 */
		void Add(Core::ArrayView<const f32> values);

		/**
		 * Get mean.
		 */
		f32 GetMean() const;

		/**
		 * Get root mean squared.
		 */
		f32 GetRootMeanSquared() const;

		/**
		 * Get sample variance.
		 */
		f32 GetSampleVariance() const;

		/**
		 * Get population variance.
		 */
		f32 GetPopulationVariance() const;

	private:
		f64 n_ = 0.0;
		f64 sum_ = 0.0;
		f64 sumSq_ = 0.0;
		f64 varianceMean_ = 0.0;
		f64 varianceSumSq_ = 0.0;
	};

} // namespace Math

#if CODE_INLINE
#include "math/private/utils.inl"
#endif
