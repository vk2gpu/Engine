#include "math/vec3.h"

#include "core/float.h"
#include "math/vec2.h"

#include <cmath>

namespace Math
{
	bool Vec3::operator==(const Vec3& Rhs) const
	{
		return ((fabs(x - Rhs.x) < Core::F32_EPSILON) && (fabs(y - Rhs.y) < Core::F32_EPSILON) &&
		        (fabs(z - Rhs.z) < Core::F32_EPSILON));
	}

	f32 Vec3::Magnitude() const
	{ //
		return sqrtf(MagnitudeSquared());
	}

	Vec3 Vec3::Normal() const
	{
		f32 Mag = Magnitude();

		if(Mag == 0.0f)
		{
			return Vec3(0, 0, 0);
		}

		const f32 InvMag = 1.0f / Mag;
		return Vec3(x * InvMag, y * InvMag, z * InvMag);
	}

	void Vec3::Normalise()
	{
		f32 Mag = Magnitude();

		if(Mag == 0.0f)
		{
			return;
		}

		const f32 InvMag = 1.0f / Mag;
		x *= InvMag;
		y *= InvMag;
		z *= InvMag;
	}

	bool CheckFloat(Vec3 T)
	{ //
		return Core::CheckFloat(T.x) && Core::CheckFloat(T.y) && Core::CheckFloat(T.z);
	}
} // namespace Math
