#include "core/random.h"

namespace Core
{
	Random::Random(u32 seed)
	    : seed_(seed)
	{
	}

	i32 Random::Generate()
	{
		i32 hi = seed_ / 127773;
		i32 lo = seed_ % 127773;

		if((seed_ = 16807 * lo - 2836 * hi) <= 0)
		{
			seed_ += 2147483647;
		}
		return seed_;
	}
} // namespace Core
