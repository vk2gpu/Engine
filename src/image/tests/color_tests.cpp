#include "image/color.h"
#include "core/random.h"
#include "core/vector.h"

#include "catch.hpp"

namespace
{
	f32 MAX_RGBA_ERROR = 1.0f / 4096.0f;
	f32 MAX_SRGB_ERROR = 1.0f / 64.0f;

	bool EpsilonCompare(f32 a, f32 b, f32 error) { return std::abs(a - b) < error; }

	Image::RGBAColor testNormalizedRGB[] = {
	    Image::RGBAColor(1.0f, 0.0f, 0.0f, 1.0f), Image::RGBAColor(0.0f, 1.0f, 0.0f, 1.0f),
	    Image::RGBAColor(0.0f, 0.0f, 1.0f, 1.0f), Image::RGBAColor(0.0f, 1.0f, 1.0f, 1.0f),
	    Image::RGBAColor(1.0f, 0.0f, 1.0f, 1.0f), Image::RGBAColor(1.0f, 1.0f, 0.0f, 1.0f),
	    Image::RGBAColor(0.1f, 0.1f, 0.1f, 1.0f), Image::RGBAColor(0.2f, 0.2f, 0.2f, 1.0f),
	    Image::RGBAColor(0.2f, 0.2f, 0.2f, 1.0f), Image::RGBAColor(0.4f, 0.4f, 0.4f, 1.0f),
	    Image::RGBAColor(0.8f, 0.8f, 0.8f, 1.0f), Image::RGBAColor(1.0f, 1.0f, 1.0f, 1.0f),
	};

	Core::Vector<Image::RGBAColor> GetTestColors()
	{
		Core::Vector<Image::RGBAColor> outColors;
		Core::Random rng;

		for(const auto rgba : testNormalizedRGB)
		{
			outColors.push_back(rgba);
		}

		for(i32 i = 0; i < 128; ++i)
		{
			f32 r = ((u32)rng.Generate() & 0xff) / 255.0f;
			f32 g = ((u32)rng.Generate() & 0xff) / 255.0f;
			f32 b = ((u32)rng.Generate() & 0xff) / 255.0f;
			f32 a = ((u32)rng.Generate() & 0xff) / 255.0f;
			outColors.push_back(Image::RGBAColor(r, g, b, a));
		}

		return outColors;
	}
}

TEST_CASE("image-color-tests-hsv")
{
	Core::Vector<Image::RGBAColor> outColors = GetTestColors();
	for(const auto rgbA : outColors)
	{
		Image::HSVColor hsv = Image::ToHSV(rgbA);
		Image::RGBAColor rgbB = Image::ToRGB(hsv);

		Core::Log("RGB(%.4f, %.4f, %.4f) -> HSV(%.4f, %.4f, %.4f) -> RGB(%.4f, %.4f, %.4f)\n", rgbA.r, rgbA.g, rgbA.b,
		    hsv.h, hsv.s, hsv.v, rgbB.r, rgbB.g, rgbB.b);

		CHECK(EpsilonCompare(rgbA.r, rgbB.r, MAX_RGBA_ERROR));
		CHECK(EpsilonCompare(rgbA.g, rgbB.g, MAX_RGBA_ERROR));
		CHECK(EpsilonCompare(rgbA.b, rgbB.b, MAX_RGBA_ERROR));
	}
}

TEST_CASE("image-color-tests-ycocg")
{
	Core::Vector<Image::RGBAColor> outColors = GetTestColors();
	for(const auto rgbA : outColors)
	{
		Image::YCoCgColor ycocg = Image::ToYCoCg(rgbA);
		Image::RGBAColor rgbB = Image::ToRGB(ycocg);

		Core::Log("RGB(%.4f, %.4f, %.4f) -> YCoCg(%.4f, %.4f, %.4f) -> RGB(%.4f, %.4f, %.4f)\n", rgbA.r, rgbA.g, rgbA.b,
		    ycocg.y, ycocg.co, ycocg.cg, rgbB.r, rgbB.g, rgbB.b);


		CHECK(EpsilonCompare(rgbA.r, rgbB.r, MAX_RGBA_ERROR));
		CHECK(EpsilonCompare(rgbA.g, rgbB.g, MAX_RGBA_ERROR));
		CHECK(EpsilonCompare(rgbA.b, rgbB.b, MAX_RGBA_ERROR));
	}
}

TEST_CASE("image-color-tests-srgb")
{
	Core::Vector<Image::RGBAColor> outColors = GetTestColors();
	for(const auto rgbA : outColors)
	{
		Image::SRGBAColor srgb = Image::ToSRGBA(rgbA);
		Image::RGBAColor rgbB = Image::ToRGBA(srgb);

		Core::Log("RGB(%.4f, %.4f, %.4f) -> SRGB(%.4f, %.4f, %.4f) -> RGB(%.4f, %.4f, %.4f)\n", rgbA.r, rgbA.g, rgbA.b,
		    srgb.r / 255.0f, srgb.g / 255.0f, srgb.b / 255.0f, rgbB.r, rgbB.g, rgbB.b);


		CHECK(EpsilonCompare(rgbA.r, rgbB.r, MAX_SRGB_ERROR));
		CHECK(EpsilonCompare(rgbA.g, rgbB.g, MAX_SRGB_ERROR));
		CHECK(EpsilonCompare(rgbA.b, rgbB.b, MAX_SRGB_ERROR));
	}
}
