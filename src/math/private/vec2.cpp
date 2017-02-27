#include "math/vec2.h"

#include "core/float.h"

#include <cmath>

namespace Math
{
	bool Vec2::operator==(const Vec2& Rhs) const
	{
		return ((fabs(x - Rhs.x) < Core::F32_EPSILON) && (fabs(y - Rhs.y) < Core::F32_EPSILON));
	}

	f32 Vec2::Magnitude() const { return sqrtf(MagnitudeSquared()); }

	Vec2 Vec2::Normal() const
	{
		f32 Mag = Magnitude();

		if(Mag == 0.0f)
		{
			return Vec2(0, 0);
		}

		const f32 InvMag = 1.0f / Mag;
		return Vec2(x * InvMag, y * InvMag);
	}

	void Vec2::Normalise()
	{
		f32 Mag = Magnitude();

		if(Mag == 0.0f)
		{
			return;
		}

		const f32 InvMag = 1.0f / Mag;
		x *= InvMag;
		y *= InvMag;
	}

	bool CheckFloat(Vec2 T)
	{ //
		return Core::CheckFloat(T.x) && Core::CheckFloat(T.x);
	}
} // namespace Math
