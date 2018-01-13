#include "image/color.h"
#include "image/image.h"
#include "image/load.h"
#include "image/process.h"
#include "image/save.h"

#include "core/file.h"
#include "core/function.h"

#include "catch.hpp"

namespace
{
	static const i32 SIZE = 256;

	enum class PatternType : i32
	{
		SOLID = 0,
		RG_GRADIENT,
		HUE_GRADIENT,

		MAX
	};

	Image::Image CreateTestImage(PatternType patternType, Image::RGBAColor color)
	{
		auto image =
		    Image::Image(GPU::TextureType::TEX2D, GPU::Format::R8G8B8A8_UNORM, SIZE, SIZE, 1, 1, nullptr, nullptr);

		if(image)
		{
			Core::Array<Core::Function<u32(i32, i32, Image::RGBAColor)>, (i32)PatternType::MAX> patternFns = {
			    [](i32 x, i32 y, Image::RGBAColor color) -> u32 { return Image::ToSRGBA(color); },
			    [](i32 x, i32 y, Image::RGBAColor color) -> u32 {
				    const f32 xf = (f32)x / (f32)(SIZE - 1);
				    const f32 yf = (f32)y / (f32)(SIZE - 1);
				    return Image::ToSRGBA(Image::RGBAColor(xf, yf, 0.0f, 1.0f));
			    },
			    [](i32 x, i32 y, Image::RGBAColor color) -> u32 {
				    const f32 xf = (f32)x / (f32)(SIZE - 1);
				    const f32 yf = (f32)y / (f32)(SIZE - 1);
				    auto hsv = Image::HSVColor(xf, 1.0f, yf);
				    auto rgba = Image::ToRGB(hsv) * color;
				    auto srgb = Image::ToSRGBA(rgba);
				    return srgb;
			    }};

			auto patternFn = patternFns[(i32)patternType];
			u32* data = image.GetMipData<u32>(0);
			for(i32 y = 0; y < image.GetHeight(); ++y)
			{
				i32 idx = y * image.GetWidth();
				for(i32 x = 0; x < image.GetWidth(); ++x)
				{
					data[idx] = patternFn(x, y, color);
					++idx;
				}
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

	f32 psnrAA = Image::CalculatePSNR(imageA, imageA);
	f32 psnrBB = Image::CalculatePSNR(imageB, imageB);
	f32 psnrCC = Image::CalculatePSNR(imageC, imageC);
	f32 psnrDD = Image::CalculatePSNR(imageD, imageD);

	REQUIRE(psnrAA == Image::INFINITE_PSNR);
	REQUIRE(psnrBB == Image::INFINITE_PSNR);
	REQUIRE(psnrCC == Image::INFINITE_PSNR);
	REQUIRE(psnrDD == Image::INFINITE_PSNR);

	f32 psnrAB = Image::CalculatePSNR(imageA, imageB);
	f32 psnrAC = Image::CalculatePSNR(imageA, imageC);
	f32 psnrAD = Image::CalculatePSNR(imageA, imageD);

	Core::Log("AB: PSNR %.2f dB\n", psnrAB);
	Core::Log("AC: PSNR %.2f dB\n", psnrAC);
	Core::Log("AD: PSNR %.2f dB\n", psnrAD);

	REQUIRE(psnrAB > psnrAC);
	REQUIRE(psnrAC > psnrAD);
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

		f32 psnr = Image::CalculatePSNR(expectedImage, image);
		if(psnr < Image::INFINITE_PSNR)
			Core::Log("%s: PSNR %.2f dB\n", fileName, psnr);
		else
			Core::Log("%s: PSNR inf dB\n", fileName, psnr);

		// Should be an exact match.
		REQUIRE(psnr == Image::INFINITE_PSNR);
	}
}
