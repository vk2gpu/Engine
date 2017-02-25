#include "math/quat.h"

#include "math/float.h"

#include <cmath>

#define SLERP_EPSILON 0.001f

namespace Math
{
	Quat Quat::Slerp(const Quat& a, const Quat& b, f32 t)
	{
		// The following code is based on what Dunn & Parberry do.
		f32 lCosOmega = (a.w * b.w) + (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
		f32 lK0;
		f32 lK1;
		Quat a2 = a;

		if(lCosOmega < 0.0f)
		{
			a2.x = -a2.x;
			a2.y = -a2.y;
			a2.z = -a2.z;
			a2.w = -a2.w;
			lCosOmega = -lCosOmega;
		}

		if(lCosOmega > (1.0f - SLERP_EPSILON))
		{
			lK0 = 1.0f - t;
			lK1 = t;
		}
		else
		{
			// Do Slerp.
			f32 lSinOmega = sqrtf(1.0f - (lCosOmega * lCosOmega));
			f32 lOmega = atan2f(lSinOmega, lCosOmega);
			f32 lInvSinOmega = (1.0f / lSinOmega);

			lK0 = sinf((1.0f - t) * lOmega) * lInvSinOmega;
			lK1 = sinf(t * lOmega) * lInvSinOmega;
		}

		return Quat((a2.x * lK0) + (b.x * lK1), (a2.y * lK0) + (b.y * lK1), (a2.z * lK0) + (b.z * lK1),
		    (a2.w * lK0) + (b.w * lK1));
	}

	void Quat::FromAxis(const Vec3& X, const Vec3& Y, const Vec3& Z)
	{
		f32 FourWSqMinus1 = X.x + Y.y + Z.z;
		f32 FourXSqMinus1 = X.x - Y.y - Z.z;
		f32 FourYSqMinus1 = Y.y - X.x - Z.z;
		f32 FourZSqMinus1 = Z.z - X.x - Y.y;

		u32 BiggestIndex = 0;
		f32 FourBiggestSqMinus1 = FourWSqMinus1;
		if(FourXSqMinus1 > FourBiggestSqMinus1)
		{
			FourBiggestSqMinus1 = FourXSqMinus1;
			BiggestIndex = 1;
		}
		if(FourYSqMinus1 > FourBiggestSqMinus1)
		{
			FourBiggestSqMinus1 = FourYSqMinus1;
			BiggestIndex = 2;
		}
		if(FourZSqMinus1 > FourBiggestSqMinus1)
		{
			FourBiggestSqMinus1 = FourZSqMinus1;
			BiggestIndex = 3;
		}

		f32 BiggestVal = sqrtf(FourBiggestSqMinus1 + 1.0f) * 0.5f;
		f32 Mult = 0.25f / BiggestVal;

		switch(BiggestIndex)
		{
		case 0:
		{
			w = BiggestVal;
			x = (Y.z - Z.y) * Mult;
			y = (Z.x - X.z) * Mult;
			z = (X.y - Y.x) * Mult;
		}
		break;

		case 1:
		{
			w = BiggestVal;
			x = (Y.z - Z.y) * Mult;
			y = (Z.x - X.z) * Mult;
			z = (X.y - Y.x) * Mult;
		}
		break;

		case 2:
		{
			y = BiggestVal;
			w = (Z.x - Y.z) * Mult;
			x = (X.y + Y.x) * Mult;
			z = (Y.z + Z.y) * Mult;
		}
		break;

		case 3:
		{
			z = BiggestVal;
			w = (X.y - Y.x) * Mult;
			x = (Z.x + X.z) * Mult;
			y = (Y.z + Z.y) * Mult;
		}
		break;
		}
	}

	void Quat::AsMatrix4d(Mat44& Matrix) const
	{
		// Multiply out the values and store in a variable
		// since storing in variables is quicker than
		// multiplying floating point numbers again and again.
		// This should make this function a touch faster.

		// Set of w multiplications required
		const f32 lWX2 = 2.0f * w * x;
		const f32 lWY2 = 2.0f * w * y;
		const f32 lWZ2 = 2.0f * w * z;

		// Set of x multiplications required
		const f32 lXX2 = 2.0f * x * x;
		const f32 lXY2 = 2.0f * x * y;
		const f32 lXZ2 = 2.0f * x * z;

		// Remainder of y multiplications
		const f32 lYY2 = 2.0f * y * y;
		const f32 lYZ2 = 2.0f * y * z;

		// Remainder of z multiplications
		const f32 lZZ2 = 2.0f * z * z;

		Matrix[0][0] = (1.0f - (lYY2 + lZZ2));
		Matrix[0][1] = (lXY2 + lWZ2);
		Matrix[0][2] = (lXZ2 - lWY2);
		Matrix[0][3] = (0.0f);

		Matrix[1][0] = (lXY2 - lWZ2);
		Matrix[1][1] = (1.0f - (lXX2 + lZZ2));
		Matrix[1][2] = (lYZ2 + lWX2);
		Matrix[1][3] = (0.0f);

		Matrix[2][0] = (lXZ2 + lWY2);
		Matrix[2][1] = (lYZ2 - lWX2);
		Matrix[2][2] = (1.0f - (lXX2 + lYY2));
		Matrix[2][3] = (0.0f);

		Matrix[3][0] = (0.0f);
		Matrix[3][1] = (0.0f);
		Matrix[3][2] = (0.0f);
		Matrix[3][3] = (1.0f);
	}

	void Quat::CalcFromXYZ()
	{
		f32 t = 1.0f - (x * x) - (y * y) - (z * z);

		if(t < 0.0f)
		{
			w = 0.0f;
		}
		else
		{
			w = -sqrtf(t);
		}
	}

	void Quat::FromEuler(f32 Yaw, f32 Pitch, f32 Roll)
	{
		const f32 Sin2Y = sinf(Yaw / 2.0f);
		const f32 Cos2Y = cosf(Yaw / 2.0f);
		const f32 Sin2P = sinf(Pitch / 2.0f);
		const f32 Cos2P = cosf(Pitch / 2.0f);
		const f32 Sin2R = sinf(Roll / 2.0f);
		const f32 Cos2R = cosf(Roll / 2.0f);

		w = (Cos2Y * Cos2P * Cos2R) + (Sin2Y * Sin2P * Sin2R);
		x = -(Cos2Y * Sin2P * Cos2R) - (Sin2Y * Cos2P * Sin2R);
		y = (Cos2Y * Sin2P * Sin2R) - (Sin2Y * Cos2P * Cos2R);
		z = (Sin2Y * Sin2P * Cos2R) - (Cos2Y * Cos2P * Sin2R);
	}

	Vec3 Quat::AsEuler() const
	{
		//
		f32 Sp = -2.0f * (y * z - w * x);

		if(std::abs(Sp) > 0.9999f)
		{
			f32 Pitch = (F32_PI * 0.5f) * Sp;
			f32 Yaw = atan2f(-x * z + w * y, 0.5f - y * y - z * z);
			f32 Roll = 0.0f;

			return Vec3(Pitch, Yaw, Roll);
		}
		else
		{
			f32 Pitch = asinf(Sp);
			f32 Yaw = atan2f(x * z + w * y, 0.5f - x * x - y * y);
			f32 Roll = atan2f(x * y + w * z, 0.5f - x * x - z * z);

			return Vec3(Pitch, Yaw, Roll);
		}
	}

	void Quat::RotateTo(const Vec3& From, const Vec3& To)
	{
		const Vec3 FromNormalised(From.Normal());
		const Vec3 ToNormalised(To.Normal());
		const Vec3 Axis(From.Cross(To));
		const f32 RadSin = sqrtf((1.0f - (FromNormalised.Dot(To))) * 0.5f);

		x = RadSin * Axis.x;
		y = RadSin * Axis.y;
		z = RadSin * Axis.z;
		w = sqrtf((1.0f + (FromNormalised.Dot(To))) * 0.5f);
	}

	void Quat::AxisAngle(const Vec3& Axis, f32 Angle)
	{
		const Vec3 AxisNormalised(Axis.Normal());
		const f32 RadSin = sinf(Angle * 0.5f);

		x = RadSin * AxisNormalised.x;
		y = RadSin * AxisNormalised.y;
		z = RadSin * AxisNormalised.z;
		w = cosf(Angle * 0.5f);
	}

	// Arithmetic

	// Cross Product
	Quat Quat::operator*(const Quat& rhs) const
	{
		return Quat((w * rhs.x) + (x * rhs.w) + (y * rhs.z) - (z * rhs.y),
		    (w * rhs.y) + (y * rhs.w) + (z * rhs.x) - (x * rhs.z),
		    (w * rhs.z) + (z * rhs.w) + (x * rhs.y) - (y * rhs.x),
		    (w * rhs.w) - (x * rhs.x) - (y * rhs.y) - (z * rhs.z));
	}

	// Make an identity quaternion 1.0(0.0, 0.0, 0.0)
	void Quat::MakeIdentity()
	{
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
		w = 1.0f;
	}

	// Return the magnitude
	f32 Quat::Magnitude()
	{ //
		return sqrtf(MagnitudeSquared());
	}

	void Quat::Inverse()
	{
		Quat Q = ~(*this);
		auto DotThis = Dot(*this);
		x = Q.x / DotThis;
		y = Q.y / DotThis;
		z = Q.z / DotThis;
		w = Q.w / DotThis;
	}


	// Quick speed up - needs optimising
	Vec3 Quat::RotateVector(const Vec3& vec) const
	{
		const Quat& This = (*this);
		const Quat OutVec = This * Quat(vec.x, vec.y, vec.z, 1.0f) * ~This;

		return Vec3(OutVec.x, OutVec.y, OutVec.z);
	}
} // namespace Math
