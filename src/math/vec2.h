#pragma once

#include "core/types.h"
#include "math/dll.h"

namespace Math
{
	//////////////////////////////////////////////////////////////////////////
	// Definition
	struct MATH_DLL Vec2
	{
	public:
		f32 x, y;

	public:
		Vec2() = default;
		Vec2(f32* Val);
		Vec2(f32 lX, f32 lY);

		Vec2 operator+(const Vec2& Rhs) const { return Vec2(x + Rhs.x, y + Rhs.y); }
		Vec2 operator-(const Vec2& Rhs) const { return Vec2(x - Rhs.x, y - Rhs.y); }
		Vec2 operator*(const Vec2& Rhs) const { return Vec2(x * Rhs.x, y * Rhs.y); }
		Vec2 operator/(const Vec2& Rhs) const { return Vec2(x / Rhs.x, y / Rhs.y); }
		Vec2 operator*(f32 Rhs) const { return Vec2(x * Rhs, y * Rhs); }
		Vec2 operator/(f32 Rhs) const { return Vec2(x / Rhs, y / Rhs); }
		Vec2& operator+=(const Vec2& Rhs);
		Vec2& operator-=(const Vec2& Rhs);
		Vec2& operator*=(f32 Rhs);
		Vec2& operator/=(f32 Rhs);
		bool operator==(const Vec2& Rhs) const;
		bool operator!=(const Vec2& Rhs) const;

		f32 Magnitude() const;
		f32 MagnitudeSquared() const { return Dot(*this); }
		f32 Dot(const Vec2& Rhs) const { return (x * Rhs.x) + (y * Rhs.y); }
		Vec2 Cross() const { return Vec2(-y, x); }
		Vec2 Normal() const;
		void Normalise();
	};

	//////////////////////////////////////////////////////////////////////////
	// Inlines
	FORCEINLINE Vec2::Vec2(f32* Val)
	    : x(Val[0])
	    , y(Val[1])
	{
	}

	FORCEINLINE Vec2::Vec2(f32 X, f32 Y)
	    : x(X)
	    , y(Y)
	{
	}

	FORCEINLINE Vec2& Vec2::operator+=(const Vec2& Rhs)
	{
		x += Rhs.x;
		y += Rhs.y;
		return (*this);
	}

	FORCEINLINE Vec2& Vec2::operator-=(const Vec2& Rhs)
	{
		x -= Rhs.x;
		y -= Rhs.y;
		return (*this);
	}

	FORCEINLINE Vec2& Vec2::operator*=(f32 Rhs)
	{
		x *= Rhs;
		y *= Rhs;
		return (*this);
	}

	FORCEINLINE Vec2& Vec2::operator/=(f32 Rhs)
	{
		const f32 InvRhs = 1.0f / Rhs;
		x *= InvRhs;
		y *= InvRhs;
		return (*this);
	}

	FORCEINLINE bool Vec2::operator!=(const Vec2& Rhs) const
	{ //
		return !((*this) == Rhs);
	}

	bool CheckFloat(Vec2 T);
} // namespace Math
