#include "image/color.h"
#include "image/image.h"
#include "image/load.h"
#include "image/process.h"
#include "image/save.h"

#include "core/file.h"
#include "core/function.h"
#include "core/misc.h"

#include "catch.hpp"

#include <cmath>

namespace
{
	static const i32 TEST_SIZE = 256;

	bool operator==(Image::RGBAColor a, Image::RGBAColor b)
	{
		return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
	}

	enum class PatternType : i32
	{
		SOLID = 0,
		RG_GRADIENT,
		HUE_GRADIENT,

		MAX
	};

	Image::Image CreateTestImage(PatternType patternType, Image::RGBAColor color, i32 numLevels = 1)
	{
		auto image = Image::Image(
		    GPU::TextureType::TEX2D, GPU::Format::R8G8B8A8_UNORM, TEST_SIZE, TEST_SIZE, 1, numLevels, nullptr, nullptr);

		if(image)
		{
			Core::Array<Core::Function<u32(i32, i32, i32, Image::RGBAColor)>, (i32)PatternType::MAX> patternFns = {
			    [](i32 x, i32 y, i32 l, Image::RGBAColor color) -> u32 { return Image::ToSRGBA(color); },
			    [](i32 x, i32 y, i32 l, Image::RGBAColor color) -> u32 {
				    const f32 xf = (f32)(x << l) / (f32)(TEST_SIZE - 1);
				    const f32 yf = (f32)(y << l) / (f32)(TEST_SIZE - 1);
				    return Image::ToSRGBA(Image::RGBAColor(xf, yf, 0.0f, 1.0f));
			    },
			    [](i32 x, i32 y, i32 l, Image::RGBAColor color) -> u32 {
				    const f32 xf = (f32)(x << l) / (f32)(TEST_SIZE - 1);
				    const f32 yf = (f32)(y << l) / (f32)(TEST_SIZE - 1);
				    auto hsv = Image::HSVColor(xf, 1.0f, yf);
				    auto rgba = Image::ToRGB(hsv) * color;
				    auto srgb = Image::ToSRGBA(rgba);
				    return srgb;
			    }};

			auto patternFn = patternFns[(i32)patternType];
			i32 w = image.GetWidth();
			i32 h = image.GetHeight();
			for(i32 l = 0; l < image.GetLevels(); ++l)
			{
				u32* data = image.GetMipData<u32>(l);
				for(i32 y = 0; y < h; ++y)
				{
					i32 idx = y * w;
					for(i32 x = 0; x < w; ++x)
					{
						data[idx] = patternFn(x, y, l, color);
						++idx;
					}
				}

				w = Core::Max(w >> 1, 1);
				h = Core::Max(h >> 1, 1);
			}
		}
		return image;
	}
}


TEST_CASE("image-tests-create")
{
	auto image = CreateTestImage(PatternType::SOLID, Image::RGBAColor(1.0f, 1.0f, 1.0f, 1.0f));
	REQUIRE(image);
}


TEST_CASE("image-tests-convert")
{
	const f32 MINIMUM_PSNR = 30.0f;
	i32 levels = 32 - Core::CountLeadingZeros((u32)TEST_SIZE);
	auto image = CreateTestImage(PatternType::HUE_GRADIENT, Image::RGBAColor(1.0f, 1.0f, 1.0f, 1.0f), levels);
	REQUIRE(image);
	auto compareImage = CreateTestImage(PatternType::SOLID, Image::RGBAColor(0.0f, 0.0f, 0.0f, 0.0f), 1);
	REQUIRE(compareImage);

	Image::Image bc1;
	CHECK(Image::Convert(bc1, image, Image::ImageFormat::BC1_UNORM));
	Image::Convert(compareImage, bc1, Image::ImageFormat::R8G8B8A8_UNORM);
	Image::RGBAColor bc1PSNR = Image::CalculatePSNR(image, compareImage);
	CHECK(bc1PSNR.r > MINIMUM_PSNR);
	CHECK(bc1PSNR.g > MINIMUM_PSNR);
	CHECK(bc1PSNR.b > MINIMUM_PSNR);
	CHECK(bc1PSNR.a == Image::INFINITE_PSNR);

	Image::Image bc3;
	CHECK(Image::Convert(bc3, image, Image::ImageFormat::BC3_UNORM));
	Image::Convert(compareImage, bc3, Image::ImageFormat::R8G8B8A8_UNORM);
	Image::RGBAColor bc3PSNR = Image::CalculatePSNR(image, compareImage);
	CHECK(bc3PSNR.r > MINIMUM_PSNR);
	CHECK(bc3PSNR.g > MINIMUM_PSNR);
	CHECK(bc3PSNR.b > MINIMUM_PSNR);
	CHECK(bc3PSNR.a > MINIMUM_PSNR);

	Image::Image bc4;
	CHECK(Image::Convert(bc4, image, Image::ImageFormat::BC4_UNORM));
	Image::Convert(compareImage, bc4, Image::ImageFormat::R8G8B8A8_UNORM);
	Image::RGBAColor bc4PSNR = Image::CalculatePSNR(image, compareImage);
	CHECK(bc4PSNR.r > MINIMUM_PSNR);

	Image::Image bc5;
	CHECK(Image::Convert(bc5, image, Image::ImageFormat::BC5_UNORM));
	Image::Convert(compareImage, bc5, Image::ImageFormat::R8G8B8A8_UNORM);
	Image::RGBAColor bc5PSNR = Image::CalculatePSNR(image, compareImage);
	CHECK(bc5PSNR.r > MINIMUM_PSNR);
	CHECK(bc5PSNR.g > MINIMUM_PSNR);

#if 0
	CHECK(Image::Convert(bc1, image, Image::ImageFormat::BC1_UNORM));
	Image::Convert(compareImage, bc1, Image::ImageFormat::R8G8B8A8_UNORM);
	Image::RGBAColor bc1PSNR = Image::CalculatePSNR(image, compareImage);
	{
		auto inFile = Core::File("test_input_bc1.png", Core::FileFlags::CREATE | Core::FileFlags::WRITE);
		auto outFile = Core::File("test_output_bc1.png", Core::FileFlags::CREATE | Core::FileFlags::WRITE);
		CHECK(Image::Save(inFile, image, Image::FileType::PNG));
		CHECK(Image::Save(outFile, compareImage, Image::FileType::PNG));

		Image::Image bc1Squish(bc1.GetType(), bc1.GetFormat(), bc1.GetWidth(), bc1.GetHeight(), bc1.GetDepth(), bc1.GetLevels(), nullptr, nullptr);

		squish::CompressImage(image.GetMipData<u8>(0), bc1Squish.GetWidth(), bc1Squish.GetHeight(), bc1Squish.GetMipData<void>(0), squish::kBc1 | squish::kColourIterativeClusterFit);
		Image::Convert(compare2Image, bc1Squish, Image::ImageFormat::R8G8B8A8_UNORM);
		Image::RGBAColor bc1SquishPSNR = Image::CalculatePSNR(image, compare2Image);
		auto out2File = Core::File("test_output_bc1squish.png", Core::FileFlags::CREATE | Core::FileFlags::WRITE);
		CHECK(Image::Save(out2File, compare2Image, Image::FileType::PNG));
	}
#endif
}

TEST_CASE("image-tests-process")
{
	const f32 MINIMUM_PSNR = 30.0f;

	i32 levels = 32 - Core::CountLeadingZeros((u32)TEST_SIZE);
	auto image = CreateTestImage(PatternType::HUE_GRADIENT, Image::RGBAColor(1.0f, 1.0f, 1.0f, 1.0f), levels);
	REQUIRE(image);

	Image::Image ycocgImage;
	Image::Image rgbaImage;

	Image::SRGBAColor i(255, 128, 64, 255);
	Image::RGBAColor c = Image::ToRGBA(i);
	Image::YCoCgColor y = Image::ToYCoCg(c);

	auto ToYCoCg = [](u32* output, const u32* input) {
		Image::SRGBAColor i((const u8*)input);
		Image::RGBAColor c = Image::ToRGBA(i);
		Image::YCoCgColor y = Image::ToYCoCg(c);

		DBG_ASSERT(y.y >= 0.0f && y.y <= 1.0f);
		DBG_ASSERT(y.co >= -0.5f && y.co <= 0.5f);
		DBG_ASSERT(y.cg >= -0.5f && y.cg <= 0.5f);

		y.y = Core::Clamp(y.y, 0.0f, 1.0f) * 255.0f;
		y.co = Core::Clamp(y.co + 0.5f, 0.0f, 1.0f) * 255.0f;
		y.cg = Core::Clamp(y.cg + 0.5f, 0.0f, 1.0f) * 255.0f;

		u32 iy = (u8)(y.y);
		u32 ico = (u8)(y.co);
		u32 icg = (u8)(y.cg);

		u32 o = 0;
		o |= iy;
		o |= ico << 8;
		o |= icg << 16;
		o |= 0xff << 24;
		*output = o;
	};

	auto FromYCoCg = [](u32* output, const u32* input) {
		u32 i = *input;
		u8 iy = i & 0xff;
		u8 ico = (i >> 8) & 0xff;
		u8 icg = (i >> 16) & 0xff;

		Image::YCoCgColor y;
		y.y = Core::Clamp((f32)iy / 255.0f, 0.0f, 1.0f);
		y.co = Core::Clamp((f32)ico / 255.0f, 0.0f, 1.0f) - 0.5f;
		y.cg = Core::Clamp((f32)icg / 255.0f, 0.0f, 1.0f) - 0.5f;

		DBG_ASSERT(y.y >= 0.0f && y.y <= 1.0f);
		DBG_ASSERT(y.co >= -0.5f && y.co <= 0.5f);
		DBG_ASSERT(y.cg >= -0.5f && y.cg <= 0.5f);

		Image::RGBAColor c = Image::ToRGB(y);
		c.r = Core::Clamp(c.r, 0.0f, 1.0f);
		c.g = Core::Clamp(c.g, 0.0f, 1.0f);
		c.b = Core::Clamp(c.b, 0.0f, 1.0f);
		c.a = Core::Clamp(c.a, 0.0f, 1.0f);
		Image::SRGBAColor o = Image::ToSRGBA(c);

		*output = o;
	};

	REQUIRE(Image::ProcessTexels<u32>(ycocgImage, image, ToYCoCg));
	REQUIRE(Image::ProcessTexels<u32>(rgbaImage, ycocgImage, FromYCoCg));


	auto psnr = Image::CalculatePSNR(rgbaImage, image);

	CHECK(psnr.r > MINIMUM_PSNR);
	CHECK(psnr.g > MINIMUM_PSNR);
	CHECK(psnr.b > MINIMUM_PSNR);
	CHECK(psnr.a == Image::INFINITE_PSNR);
}


TEST_CASE("image-tests-compare")
{
	auto imageA = CreateTestImage(PatternType::SOLID, Image::RGBAColor(1.0f, 1.0f, 1.0f, 1.0f));
	REQUIRE(imageA);
	auto imageB = CreateTestImage(PatternType::SOLID, Image::RGBAColor(0.9f, 0.9f, 0.9f, 0.9f));
	REQUIRE(imageB);
	auto imageC = CreateTestImage(PatternType::SOLID, Image::RGBAColor(0.5f, 0.5f, 0.5f, 0.5f));
	REQUIRE(imageC);
	auto imageD = CreateTestImage(PatternType::SOLID, Image::RGBAColor(0.0f, 0.0f, 0.0f, 0.0f));
	REQUIRE(imageD);

	Image::RGBAColor psnrAA = Image::CalculatePSNR(imageA, imageA);
	Image::RGBAColor psnrBB = Image::CalculatePSNR(imageB, imageB);
	Image::RGBAColor psnrCC = Image::CalculatePSNR(imageC, imageC);
	Image::RGBAColor psnrDD = Image::CalculatePSNR(imageD, imageD);

	CHECK(psnrAA == Image::INFINITE_PSNR_RGBA);
	CHECK(psnrBB == Image::INFINITE_PSNR_RGBA);
	CHECK(psnrCC == Image::INFINITE_PSNR_RGBA);
	CHECK(psnrDD == Image::INFINITE_PSNR_RGBA);

	Image::RGBAColor psnrAB = Image::CalculatePSNR(imageA, imageB);
	Image::RGBAColor psnrAC = Image::CalculatePSNR(imageA, imageC);
	Image::RGBAColor psnrAD = Image::CalculatePSNR(imageA, imageD);

	Core::Log("AB: PSNR %.2f dB\n", psnrAB.r);
	Core::Log("AC: PSNR %.2f dB\n", psnrAC.r);
	Core::Log("AD: PSNR %.2f dB\n", psnrAD.r);

	CHECK(psnrAB.r > psnrAC.r);
	CHECK(psnrAC.r > psnrAD.r);
}


TEST_CASE("image-tests-save")
{
	const char* fileName = "image-tests-save.png";
	if(Core::FileExists(fileName))
		Core::FileRemove(fileName);

	auto image = CreateTestImage(PatternType::HUE_GRADIENT, Image::RGBAColor(1.0f, 1.0f, 1.0f, 1.0f));
	REQUIRE(image);

	auto file = Core::File(fileName, Core::FileFlags::CREATE | Core::FileFlags::WRITE);
	REQUIRE(file);
	REQUIRE(Image::Save(file, image, Image::FileType::PNG));
}


TEST_CASE("image-tests-load")
{
	const char* fileName = "image-tests-load.png";
	if(Core::FileExists(fileName))
		Core::FileRemove(fileName);

	// Create an image to save out.
	auto expectedImage = CreateTestImage(PatternType::HUE_GRADIENT, Image::RGBAColor(1.0f, 1.0f, 1.0f, 1.0f));
	REQUIRE(expectedImage);

	// Save.
	{
		if(Core::FileExists(fileName))
			Core::FileRemove(fileName);

		auto file = Core::File(fileName, Core::FileFlags::CREATE | Core::FileFlags::WRITE);
		REQUIRE(file);
		REQUIRE(Image::Save(file, expectedImage, Image::FileType::PNG));
	}

	// Load.
	{
		auto file = Core::File(fileName, Core::FileFlags::READ);
		auto image = Image::Load(file, nullptr);
		REQUIRE(image);

		Image::RGBAColor psnr = Image::CalculatePSNR(expectedImage, image);
		REQUIRE(psnr == Image::INFINITE_PSNR_RGBA);
	}
}
