#include "image/color.h"

#include "catch.hpp"

namespace
{
	f32 MAX_RGBA_ERROR = 1.0f / 4096.0f;

	bool EpsilonCompare(f32 a, f32 b, f32 error) { return std::abs(a - b) < error; }

	Image::RGBAColor testNormalizedRGB[] = {
	    Image::RGBAColor(1.0f, 0.0f, 0.0f, 1.0f), Image::RGBAColor(0.0f, 1.0f, 0.0f, 1.0f),
	    Image::RGBAColor(0.0f, 0.0f, 1.0f, 1.0f), Image::RGBAColor(0.0f, 1.0f, 1.0f, 1.0f),
	    Image::RGBAColor(1.0f, 0.0f, 1.0f, 1.0f), Image::RGBAColor(1.0f, 1.0f, 0.0f, 1.0f),
	    Image::RGBAColor(0.1f, 0.1f, 0.1f, 1.0f), Image::RGBAColor(0.2f, 0.2f, 0.2f, 1.0f),
	    Image::RGBAColor(0.2f, 0.2f, 0.2f, 1.0f), Image::RGBAColor(0.4f, 0.4f, 0.4f, 1.0f),
	    Image::RGBAColor(0.8f, 0.8f, 0.8f, 1.0f), Image::RGBAColor(1.0f, 1.0f, 1.0f, 1.0f),
	};
}

TEST_CASE("image-color-tests-hsv")
{
	for(const auto rgbA : testNormalizedRGB)
	{
		Image::HSVColor hsv = Image::ToHSV(rgbA);
		Image::RGBAColor rgbB = Image::ToRGB(hsv);

		Core::Log("RGB(%.1f, %.1f, %.1f) -> HSV(%.1f, %.1f, %.1f) -> RGB(%.1f, %.1f, %.1f)\n", rgbA.r, rgbA.g, rgbA.b,
		    hsv.h, hsv.s, hsv.v, rgbB.r, rgbB.g, rgbB.b);

		CHECK(EpsilonCompare(rgbA.r, rgbB.r, MAX_RGBA_ERROR));
		CHECK(EpsilonCompare(rgbA.g, rgbB.g, MAX_RGBA_ERROR));
		CHECK(EpsilonCompare(rgbA.b, rgbB.b, MAX_RGBA_ERROR));
	}
}

TEST_CASE("image-color-tests-ycocg")
{
	for(const auto rgbA : testNormalizedRGB)
	{
		Image::YCoCgColor ycocg = Image::ToYCoCg(rgbA);
		Image::RGBAColor rgbB = Image::ToRGB(ycocg);

		Core::Log("RGB(%.1f, %.1f, %.1f) -> YCoCg(%.1f, %.1f, %.1f) -> RGB(%.1f, %.1f, %.1f)\n", rgbA.r, rgbA.g, rgbA.b,
		    ycocg.y, ycocg.co, ycocg.cg, rgbB.r, rgbB.g, rgbB.b);


		CHECK(EpsilonCompare(rgbA.r, rgbB.r, MAX_RGBA_ERROR));
		CHECK(EpsilonCompare(rgbA.g, rgbB.g, MAX_RGBA_ERROR));
		CHECK(EpsilonCompare(rgbA.b, rgbB.b, MAX_RGBA_ERROR));
	}
}
