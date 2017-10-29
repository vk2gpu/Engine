#include "core/array.h"
#include "core/debug.h"
#include "core/misc.h"
#include "core/type_conversion.h"
#include "core/vector.h"

#include "catch.hpp"

namespace
{
	Core::Array<f32, 11> floatArray_UNORM = {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f};

	Core::Array<f32, 21> floatArray_SNORM = {-1.0f, -0.9f, -0.8f, -0.7f, -0.6f, -0.5f, -0.4f, -0.3f, -0.2f, -0.1f, 0.0f,
	    0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f};

	Core::Array<f32, 11> floatArray_UINT = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};

	Core::Array<f32, 21> floatArray_SINT = {-10.0f, -9.0f, -8.0f, -7.0f, -6.0f, -5.0f, -4.0f, -3.0f, -2.0f, -1.0f, 0.0f,
	    1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};

	struct InterleavedDataIn
	{
		f32 x_, y_, z_;
		f32 u_, v_;
		f32 r_, g_, b_, a_;
	};

	struct InterleavedDataOut
	{
		u16 x_, y_, z_;
		u16 u_, v_;
		u8 r_, g_, b_, a_;
	};

	Core::Array<InterleavedDataIn, 8> interleavedArray;

	void SetupInterleavedArray()
	{
		interleavedArray[0] = {/* xyz */ 0.0f, 0.0f, 0.0f, /* uv */ 0.0f, 0.0f, /* rgba */ 1.0f, 1.0f, 1.0f, 1.0f};
		interleavedArray[1] = {/* xyz */ 1.0f, 0.0f, 0.0f, /* uv */ 0.5f, 0.0f, /* rgba */ 1.0f, 1.0f, 1.0f, 1.0f};
		interleavedArray[2] = {/* xyz */ 1.0f, 1.0f, 0.0f, /* uv */ 0.5f, 0.5f, /* rgba */ 1.0f, 1.0f, 1.0f, 1.0f};
		interleavedArray[3] = {/* xyz */ 0.0f, 1.0f, 0.0f, /* uv */ 0.0f, 0.5f, /* rgba */ 1.0f, 1.0f, 1.0f, 1.0f};
		interleavedArray[4] = {/* xyz */ 1.0f, 1.0f, 0.0f, /* uv */ 0.0f, 0.0f, /* rgba */ 0.1f, 0.2f, 0.3f, 0.7f};
		interleavedArray[5] = {/* xyz */ 2.0f, 1.0f, 0.0f, /* uv */ 0.5f, 0.0f, /* rgba */ 0.1f, 0.2f, 0.3f, 0.7f};
		interleavedArray[6] = {/* xyz */ 2.0f, 2.0f, 0.0f, /* uv */ 0.5f, 0.5f, /* rgba */ 0.1f, 0.2f, 0.3f, 0.7f};
		interleavedArray[7] = {/* xyz */ 1.0f, 2.0f, 0.0f, /* uv */ 0.0f, 0.5f, /* rgba */ 0.1f, 0.2f, 0.3f, 0.7f};
	}

	f32 CalculateMaxError(const void* baseB, const void* compB, i32 num, i32 stride)
	{
		f32 error = 0.0f;
		for(i32 i = 0; i < num; ++i)
		{
			auto base = *(f32*)baseB;
			auto comp = *(f32*)compB;
			error = Core::Max(error, std::abs((base - comp)));
			(u8*&)baseB += stride;
			(u8*&)compB += stride;
		}
		return error;
	}
}

TEST_CASE("type-conversion-tests-f32-to-f16")
{
	auto doConversionTest = [](const f32* data, i32 num) -> f32 {
		Core::Array<u16, 32> output;
		Core::Array<f32, 32> compare;
		num = std::min(num, output.size());

		{
			Core::StreamDesc inStream((void*)data, Core::DataType::FLOAT, 32, sizeof(f32));
			Core::StreamDesc outStream(output.data(), Core::DataType::FLOAT, 16, sizeof(u16));
			REQUIRE(Core::Convert(outStream, inStream, num, 1));
		}

		{
			Core::StreamDesc inStream(output.data(), Core::DataType::FLOAT, 16, sizeof(u16));
			Core::StreamDesc outStream(compare.data(), Core::DataType::FLOAT, 32, sizeof(f32));

			REQUIRE(Core::Convert(outStream, inStream, num, 1));
		}

		return CalculateMaxError(data, compare.data(), num, sizeof(f32));
	};

	const f32 MAX_ERROR = 0.001f;
	CHECK(doConversionTest(floatArray_UNORM.data(), floatArray_UNORM.size()) < MAX_ERROR);
	CHECK(doConversionTest(floatArray_SNORM.data(), floatArray_SNORM.size()) < MAX_ERROR);
	CHECK(doConversionTest(floatArray_UINT.data(), floatArray_UINT.size()) < MAX_ERROR);
	CHECK(doConversionTest(floatArray_SINT.data(), floatArray_SINT.size()) < MAX_ERROR);
}

TEST_CASE("type-conversion-tests-f32-to-u8-unorm")
{
	auto doConversionTest = [](const f32* data, i32 num) -> f32 {
		Core::Array<u8, 32> output;
		Core::Array<f32, 32> compare;
		num = std::min(num, output.size());

		{
			Core::StreamDesc inStream((void*)data, Core::DataType::FLOAT, 32, sizeof(f32));
			Core::StreamDesc outStream(output.data(), Core::DataType::UNORM, 8, sizeof(u8));
			REQUIRE(Core::Convert(outStream, inStream, num, 1));
		}

		{
			Core::StreamDesc inStream(output.data(), Core::DataType::UNORM, 8, sizeof(u8));
			Core::StreamDesc outStream(compare.data(), Core::DataType::FLOAT, 32, sizeof(f32));

			REQUIRE(Core::Convert(outStream, inStream, num, 1));
		}

		return CalculateMaxError(data, compare.data(), num, sizeof(f32));
	};

	const f32 MAX_ERROR = 1.0f / (f32)0xff;
	CHECK(doConversionTest(floatArray_UNORM.data(), floatArray_UNORM.size()) < MAX_ERROR);
}

TEST_CASE("type-conversion-tests-f32-to-s8-snorm")
{
	auto doConversionTest = [](const f32* data, i32 num) -> f32 {
		Core::Array<u8, 32> output;
		Core::Array<f32, 32> compare;
		num = std::min(num, output.size());

		{
			Core::StreamDesc inStream((void*)data, Core::DataType::FLOAT, 32, sizeof(f32));
			Core::StreamDesc outStream(output.data(), Core::DataType::SNORM, 8, sizeof(i8));
			REQUIRE(Core::Convert(outStream, inStream, num, 1));
		}

		{
			Core::StreamDesc inStream(output.data(), Core::DataType::SNORM, 8, sizeof(i8));
			Core::StreamDesc outStream(compare.data(), Core::DataType::FLOAT, 32, sizeof(f32));
			REQUIRE(Core::Convert(outStream, inStream, num, 1));
		}

		return CalculateMaxError(data, compare.data(), num, sizeof(f32));
	};

	const f32 MAX_ERROR = 1.0f / (f32)0x7f;
	CHECK(doConversionTest(floatArray_SNORM.data(), floatArray_SNORM.size()) < MAX_ERROR);
}

TEST_CASE("type-conversion-tests-f32-to-u16-unorm")
{
	auto doConversionTest = [](const f32* data, i32 num) -> f32 {
		Core::Array<u16, 32> output;
		Core::Array<f32, 32> compare;
		num = std::min(num, output.size());

		{
			Core::StreamDesc inStream((void*)data, Core::DataType::FLOAT, 32, sizeof(f32));
			Core::StreamDesc outStream(output.data(), Core::DataType::UNORM, 16, sizeof(u16));
			REQUIRE(Core::Convert(outStream, inStream, num, 1));
		}

		{
			Core::StreamDesc inStream(output.data(), Core::DataType::UNORM, 16, sizeof(u16));
			Core::StreamDesc outStream(compare.data(), Core::DataType::FLOAT, 32, sizeof(f32));
			REQUIRE(Core::Convert(outStream, inStream, num, 1));
		}

		return CalculateMaxError(data, compare.data(), num, sizeof(f32));
	};

	const f32 MAX_ERROR = 1.0f / (f32)0xffff;
	CHECK(doConversionTest(floatArray_UNORM.data(), floatArray_UNORM.size()) < MAX_ERROR);
}

TEST_CASE("type-conversion-tests-f32-to-s16-snorm")
{
	auto doConversionTest = [](const f32* data, i32 num) -> f32 {
		Core::Array<u16, 32> output;
		Core::Array<f32, 32> compare;
		num = std::min(num, output.size());

		{
			Core::StreamDesc inStream((void*)data, Core::DataType::FLOAT, 32, sizeof(f32));
			Core::StreamDesc outStream(output.data(), Core::DataType::SNORM, 16, sizeof(i16));
			REQUIRE(Core::Convert(outStream, inStream, num, 1));
		}

		{
			Core::StreamDesc inStream(output.data(), Core::DataType::SNORM, 16, sizeof(i16));
			Core::StreamDesc outStream(compare.data(), Core::DataType::FLOAT, 32, sizeof(f32));
			REQUIRE(Core::Convert(outStream, inStream, num, 1));
		}

		return CalculateMaxError(data, compare.data(), num, sizeof(f32));
	};

	const f32 MAX_ERROR = 1.0f / (f32)0x7fff;
	CHECK(doConversionTest(floatArray_SNORM.data(), floatArray_SNORM.size()) < MAX_ERROR);
}

TEST_CASE("type-conversion-tests-f32-to-u8-uint")
{
	auto doConversionTest = [](const f32* data, i32 num) -> f32 {
		Core::Array<u8, 32> output;
		Core::Array<f32, 32> compare;
		num = std::min(num, output.size());

		{
			Core::StreamDesc inStream((void*)data, Core::DataType::FLOAT, 32, sizeof(f32));
			Core::StreamDesc outStream(output.data(), Core::DataType::UINT, 8, sizeof(u8));
			REQUIRE(Core::Convert(outStream, inStream, num, 1));
		}

		{
			Core::StreamDesc inStream(output.data(), Core::DataType::UINT, 8, sizeof(u8));
			Core::StreamDesc outStream(compare.data(), Core::DataType::FLOAT, 32, sizeof(f32));
			REQUIRE(Core::Convert(outStream, inStream, num, 1));
		}

		return CalculateMaxError(data, compare.data(), num, sizeof(f32));
	};

	const f32 MAX_ERROR = 0.0f;
	CHECK(doConversionTest(floatArray_UINT.data(), floatArray_UINT.size()) <= MAX_ERROR);
}

TEST_CASE("type-conversion-tests-interleaved-data")
{
	auto doConversionTest = [](const InterleavedDataIn* data, i32 num) -> f32 {
		SetupInterleavedArray();

		Core::StreamDesc inStream_F32(nullptr, Core::DataType::FLOAT, 32, sizeof(InterleavedDataIn));

		Core::StreamDesc outStream_F16(nullptr, Core::DataType::FLOAT, 16, sizeof(InterleavedDataOut));
		Core::StreamDesc outStream_UNORM16(nullptr, Core::DataType::UNORM, 16, sizeof(InterleavedDataOut));
		Core::StreamDesc outStream_UNORM8(nullptr, Core::DataType::UNORM, 8, sizeof(InterleavedDataOut));

		Core::Array<InterleavedDataOut, 32> output;
		Core::Array<InterleavedDataIn, 32> compare;
		num = std::min(num, output.size());

		{
			REQUIRE(Core::Convert(outStream_F16((void*)&output[0].x_), inStream_F32((void*)&data[0].x_), num, 3));
			REQUIRE(Core::Convert(outStream_UNORM16((void*)&output[0].u_), inStream_F32((void*)&data[0].u_), num, 2));
			REQUIRE(Core::Convert(outStream_UNORM8((void*)&output[0].r_), inStream_F32((void*)&data[0].r_), num, 4));
		}

		{
			REQUIRE(Core::Convert(inStream_F32((void*)&compare[0].x_), outStream_F16((void*)&output[0].x_), num, 3));
			REQUIRE(
			    Core::Convert(inStream_F32((void*)&compare[0].u_), outStream_UNORM16((void*)&output[0].u_), num, 2));
			REQUIRE(Core::Convert(inStream_F32((void*)&compare[0].r_), outStream_UNORM8((void*)&output[0].r_), num, 4));
		}

		return CalculateMaxError(data, compare.data(), (sizeof(InterleavedDataIn) / sizeof(f32)) * num, sizeof(f32));
	};

	const f32 MAX_ERROR = 1.0f / (f32)0xff;
	CHECK(doConversionTest(interleavedArray.data(), interleavedArray.size()) < MAX_ERROR);
}
