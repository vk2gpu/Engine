#include "math/utils.h"

#if !CODE_INLINE
#include "math/private/utils.inl"
#endif

namespace Math
{
	void Statistics::Add(f32 value)
	{
		const f64 oldVarianceMean = varianceMean_;
		n_ += 1.0;
		sum_ += value;
		sumSq_ += value * value;
		varianceMean_ += (value - varianceMean_) / n_;
		varianceSumSq_ += (value - varianceMean_) * (value - oldVarianceMean);
	}

	void Statistics::Add(Core::ArrayView<const f32> values)
	{
		for(f32 value : values)
			Add(value);
	}

	f32 Statistics::GetMean() const { return (f32)(sum_ / n_); }

	f32 Statistics::GetRootMeanSquared() const { return (f32)std::sqrt(sumSq_ / n_); }

	f32 Statistics::GetSampleVariance() const { return (f32)(varianceSumSq_ / (n_ - 1.0)); }

	f32 Statistics::GetPopulationVariance() const { return (f32)(varianceSumSq_ / n_); }

} // namespace Math
