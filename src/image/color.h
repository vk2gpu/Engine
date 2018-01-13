#pragma once

#include "image/types.h"

namespace Image
{
	/**
	 * HSV color.
	 */
	struct IMAGE_DLL HSVColor
	{
	public:
		f32 h, s, v;

	public:
		HSVColor();
		HSVColor(f32* val);
		HSVColor(f32 h, f32 s, f32 v);
	};

	/**
	 * YCoCg color.
	 */
	struct IMAGE_DLL YCoCgColor
	{
	public:
		f32 y, co, cg;

	public:
		YCoCgColor();
		YCoCgColor(f32* val);
		YCoCgColor(f32 y, f32 co, f32 cg);
	};

	/**
	 * Linear space RGBA color.
	 */
	struct IMAGE_DLL RGBAColor
	{
	public:
		f32 r, g, b, a;

	public:
		RGBAColor();
		RGBAColor(f32* val);
		RGBAColor(f32 r, f32 g, f32 b, f32 a);

		RGBAColor operator+(const RGBAColor& Rhs) const
		{
			return RGBAColor(r + Rhs.r, g + Rhs.g, b + Rhs.b, a + Rhs.a);
		}
		RGBAColor operator-(const RGBAColor& Rhs) const
		{
			return RGBAColor(r - Rhs.r, g - Rhs.g, b - Rhs.b, a - Rhs.a);
		}
		RGBAColor operator*(f32 Rhs) const { return RGBAColor(r * Rhs, g * Rhs, b * Rhs, a * Rhs); }
		RGBAColor operator/(const RGBAColor& Rhs) const
		{
			return RGBAColor(r / Rhs.r, g / Rhs.g, b / Rhs.b, a / Rhs.a);
		}
		RGBAColor operator*(const RGBAColor& Rhs) const
		{
			return RGBAColor(r * Rhs.r, g * Rhs.g, b * Rhs.b, a * Rhs.a);
		}
		RGBAColor operator/(f32 Rhs) const { return RGBAColor(r / Rhs, g / Rhs, b / Rhs, a / Rhs); }
		RGBAColor& operator+=(const RGBAColor& Rhs);
		RGBAColor& operator-=(const RGBAColor& Rhs);
		RGBAColor& operator*=(f32 Rhs);
		RGBAColor& operator/=(f32 Rhs);
	};

	FORCEINLINE RGBAColor& RGBAColor::operator+=(const RGBAColor& Rhs)
	{
		r += Rhs.r;
		g += Rhs.g;
		b += Rhs.b;
		a += Rhs.a;
		return (*this);
	}

	FORCEINLINE RGBAColor& RGBAColor::operator-=(const RGBAColor& Rhs)
	{
		r -= Rhs.r;
		g -= Rhs.g;
		b -= Rhs.b;
		a -= Rhs.a;
		return (*this);
	}

	FORCEINLINE RGBAColor& RGBAColor::operator*=(f32 Rhs)
	{
		r *= Rhs;
		g *= Rhs;
		b *= Rhs;
		a *= Rhs;
		return (*this);
	}

	FORCEINLINE RGBAColor& RGBAColor::operator/=(f32 Rhs)
	{
		const f32 InvRhs = 1.0f / Rhs;
		r *= InvRhs;
		g *= InvRhs;
		b *= InvRhs;
		a *= InvRhs;
		return (*this);
	}

	/**
	 * Convert RGB to HSV.
	 */
	IMAGE_DLL HSVColor ToHSV(RGBAColor rgb);

	/**
	 * Convert RGB to YCoCg.
	 */
	IMAGE_DLL YCoCgColor ToYCoCg(RGBAColor rgb);

	/**
	 * Convert HSV to RGB.
	 */
	IMAGE_DLL RGBAColor ToRGB(HSVColor hsv);

	/**
	 * Convert YCoCg to RGB.
	 */
	IMAGE_DLL RGBAColor ToRGB(YCoCgColor ycocg);

} // namespace Image
