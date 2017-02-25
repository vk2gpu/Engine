#pragma once

#include "core/types.h"
#include "math/dll.h"
#include "math/vec2.h"

#define DECLARE_SWIZZLE2(T, X, Y)                                                                                      \
	inline T X##Y() const { return T(X, Y); }

namespace Math
{
	/**
	 * 3D Vector.
	 */
	struct MATH_DLL Vec3 final
	{
	public:
		f32 x, y, z;

	public:
		Vec3() = default;
		Vec3(f32* Val);
		Vec3(f32 X, f32 Y, f32 Z);
		Vec3(const Vec2& Rhs, f32 Z);

		Vec3 operator+(const Vec3& Rhs) const { return Vec3(x + Rhs.x, y + Rhs.y, z + Rhs.z); }
		Vec3 operator-(const Vec3& Rhs) const { return Vec3(x - Rhs.x, y - Rhs.y, z - Rhs.z); }
		Vec3 operator*(f32 Rhs) const { return Vec3(x * Rhs, y * Rhs, z * Rhs); }
		Vec3 operator/(f32 Rhs) const { return Vec3(x / Rhs, y / Rhs, z / Rhs); }
		Vec3 operator*(const Vec3& Rhs) const { return Vec3(x * Rhs.x, y * Rhs.y, z * Rhs.z); }
		Vec3 operator/(const Vec3& Rhs) const { return Vec3(x / Rhs.x, y / Rhs.y, z / Rhs.z); }
		Vec3 operator-() const { return Vec3(-x, -y, -z); }
		Vec3& operator+=(const Vec3& Rhs);
		Vec3& operator-=(const Vec3& Rhs);
		Vec3& operator*=(f32 Rhs);
		Vec3& operator/=(f32 Rhs);

		bool operator==(const Vec3& Rhs) const;
		bool operator!=(const Vec3& Rhs) const;

		Vec3 Normal() const;
		void Normalise();
		f32 Magnitude() const;
		f32 MagnitudeSquared() const { return Dot(*this); }
		Vec3 Reflect(const Vec3& Normal) const { return (*this - (Normal * (2.0f * Dot(Normal)))); }
		f32 Dot(const Vec3& Rhs) const { return (x * Rhs.x) + (y * Rhs.y) + (z * Rhs.z); }
		Vec3 Cross(const Vec3& Rhs) const
		{
			return Vec3((y * Rhs.z) - (Rhs.y * z), (z * Rhs.x) - (Rhs.z * x), (x * Rhs.y) - (Rhs.x * y));
		}


		DECLARE_SWIZZLE2(Vec2, x, x);
		DECLARE_SWIZZLE2(Vec2, x, y);
		DECLARE_SWIZZLE2(Vec2, x, z);
		DECLARE_SWIZZLE2(Vec2, y, x);
		DECLARE_SWIZZLE2(Vec2, y, y);
		DECLARE_SWIZZLE2(Vec2, y, z);
		DECLARE_SWIZZLE2(Vec2, z, x);
		DECLARE_SWIZZLE2(Vec2, z, y);
		DECLARE_SWIZZLE2(Vec2, z, z);
	};

	FORCEINLINE Vec3::Vec3(f32* Val)
	    : x(Val[0])
	    , y(Val[1])
	    , z(Val[2])
	{
	}

	FORCEINLINE Vec3::Vec3(f32 X, f32 Y, f32 Z)
	    : x(X)
	    , y(Y)
	    , z(Z)
	{
	}

	FORCEINLINE Vec3::Vec3(const Vec2& V, f32 Z)
	    : x(V.x)
	    , y(V.y)
	    , z(Z)
	{
	}


	FORCEINLINE Vec3& Vec3::operator+=(const Vec3& Rhs)
	{
		x += Rhs.x;
		y += Rhs.y;
		z += Rhs.z;
		return (*this);
	}

	FORCEINLINE Vec3& Vec3::operator-=(const Vec3& Rhs)
	{
		x -= Rhs.x;
		y -= Rhs.y;
		z -= Rhs.z;
		return (*this);
	}

	FORCEINLINE Vec3& Vec3::operator*=(f32 Rhs)
	{
		x *= Rhs;
		y *= Rhs;
		z *= Rhs;
		return (*this);
	}

	FORCEINLINE Vec3& Vec3::operator/=(f32 Rhs)
	{
		const f32 InvRhs = 1.0f / Rhs;
		x *= InvRhs;
		y *= InvRhs;
		z *= InvRhs;
		return (*this);
	}


	FORCEINLINE bool Vec3::operator!=(const Vec3& Rhs) const
	{ //
		return !((*this) == Rhs);
	}

	bool CheckFloat(Vec3 T);
} // namespace Math
