#include "math/vec4.h"

#include "core/float.h"

#include <cmath>

namespace Math
{
	Vec4::Vec4(const Vec2& Rhs)
	    : x(Rhs.x)
	    , y(Rhs.y)
	    , z(0.0f)
	    , w(1.0f)
	{
	}

	Vec4::Vec4(const Vec3& Rhs, f32 W)
	    : x(Rhs.x)
	    , y(Rhs.y)
	    , z(Rhs.z)
	    , w(W)
	{
	}

	Vec3 Vec4::Normal3() const
	{
		f32 Mag = sqrtf(x * x + y * y + z * z);

		if(Mag == 0.0f)
		{
			return Vec3(0, 0, 0);
		}

		const f32 InvMag = 1.0f / Mag;
		return Vec3(x * InvMag, y * InvMag, z * InvMag);
	}

	f32 Vec4::Magnitude() const
	{ //
		return sqrtf(MagnitudeSquared());
	}

	void Vec4::Normalise()
	{
		f32 Mag = Magnitude();

		if(Mag == 0.0f)
			return;

		const f32 InvMag = 1.0f / Mag;
		x *= InvMag;
		y *= InvMag;
		z *= InvMag;
		w *= InvMag;
	}

	void Vec4::Normalise3()
	{
		f32 Mag = sqrtf(x * x + y * y + z * z);

		if(Mag == 0.0f)
			return;

		const f32 InvMag = 1.0f / Mag;
		x *= InvMag;
		y *= InvMag;
		z *= InvMag;
	}

	Vec4 Vec4::Normal() const
	{
		f32 Mag = Magnitude();

		if(Mag == 0.0f)
		{
			return Vec4(0, 0, 0, 0);
		}

		const f32 InvMag = 1.0f / Mag;
		return Vec4(x * InvMag, y * InvMag, z * InvMag, w * InvMag);
	}

	bool Vec4::operator==(const Vec4& Rhs) const
	{
		return ((abs(x - Rhs.x) < Core::F32_EPSILON) && (abs(y - Rhs.y) < Core::F32_EPSILON) &&
		        (abs(z - Rhs.z) < Core::F32_EPSILON) && (abs(w - Rhs.w) < Core::F32_EPSILON));
	}

	bool CheckFloat(Vec4 T)
	{ //
		return Core::CheckFloat(T.x) && Core::CheckFloat(T.y) && Core::CheckFloat(T.z) && Core::CheckFloat(T.w);
	}
} // namespace Math
