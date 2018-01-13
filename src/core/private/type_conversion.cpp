#include "core/type_conversion.h"
#include "core/array.h"
#include "core/half.h"
#include "core/misc.h"

#include <limits>
#include <cmath>
#include <xmmintrin.h>
#include <memory.h>


#if defined(COMPILER_MSVC)
#pragma optimize("", on)
#endif

namespace Core
{
	namespace
	{
		using ConvertFnArray = ConvertFn**;

		template<i32 BYTE_SIZE>
		void Copy(void* outVal, const void* val, int c)
		{
			memcpy(outVal, val, c * BYTE_SIZE);
		}

		template<typename TYPE_A, typename TYPE_B>
		void AtoB(void* outVal, const void* val, int c)
		{
			auto* outValT = ((TYPE_B*)outVal);
			const auto* inValT = (const TYPE_A*)val;
			for(int i = 0; i < c; ++i)
				outValT[i] = (TYPE_B)inValT[i];
		}

		template<typename TYPE>
		void F32toUNORM(void* outVal, const void* val, int c)
		{
			auto* outValT = ((TYPE*)outVal);
			const auto* inValT = (const f32*)val;
			const f32 SCALE = (powf(2.0f, sizeof(TYPE) * 8) - 1.0f);
			for(int i = 0; i < c; ++i)
			{
				f32 v = Core::Clamp(inValT[i], 0.0f, 1.0f) * SCALE;
				v += 0.5f;
				outValT[i] = (TYPE)v;
			}
		}

		template<typename TYPE>
		void UNORMtoF32(void* outVal, const void* val, int c)
		{
			auto* outValT = (f32*)outVal;
			const auto* inValT = (const TYPE*)val;
			const f32 SCALE = (powf(2.0f, sizeof(TYPE) * 8) - 1.0f);
			for(int i = 0; i < c; ++i)
				outValT[i] = ((f32)inValT[i]) / SCALE;
		}

		template<typename TYPE>
		void F32toSNORM(void* outVal, const void* val, int c)
		{
			auto* outValT = ((TYPE*)outVal);
			const auto* inValT = (const f32*)val;
			const f32 SCALE = (powf(2.0f, sizeof(TYPE) * 8 - 1) - 1.0f);
			for(int i = 0; i < c; ++i)
			{
				f32 v = Core::Clamp(inValT[i], -1.0f, 1.0f) * SCALE;
				v = v >= 0.0f ? v + 0.5f : v - 0.5f;
				outValT[i] = (TYPE)v;
			}
		}

		template<typename TYPE>
		void SNORMtoF32(void* outVal, const void* val, int c)
		{
			auto* outValT = (f32*)outVal;
			const auto* inValT = (const TYPE*)val;
			const f32 SCALE = (powf(2.0f, sizeof(TYPE) * 8 - 1) - 1.0f);
			for(int i = 0; i < c; ++i)
				outValT[i] = Core::Clamp((f32)inValT[i] / SCALE, -1.0f, 1.0f);
		}

		void F16toF32(void* outVal, const void* val, int c) { HalfToFloat((const u16*)val, (f32*)outVal, c); }

		void F32toF16(void* outVal, const void* val, int c) { FloatToHalf((const f32*)val, (u16*)outVal, c); }

		template<typename TYPE, ConvertFn FN_A, ConvertFn FN_B>
		void Adapter(void* outVal, const void* val, int c)
		{
			DBG_ASSERT(c <= 16);
			Core::Array<TYPE, 16> intermediate;
			FN_A(intermediate.data(), val, c);
			FN_B(outVal, intermediate.data(), c);
		}

		// clang-format off
		ConvertFn* FLOATtoFLOATFns[] = {
		    // 8 -> (8, 16, 32)
		    nullptr, nullptr, nullptr,

		    // 16 -> (8, 16, 32)
		    nullptr, Copy<2>, F16toF32,

		    // 32 -> (8, 16, 32)
		    nullptr, F32toF16, Copy<4>,
		};

		ConvertFn* FLOATtoUNORMFns[] = {
		    // 8 -> (8, 16, 32)
		    nullptr, nullptr, nullptr,

		    // 16 -> (8, 16, 32)
		    Adapter<f32, F16toF32, F32toUNORM<u8>>, Adapter<f32, F16toF32, F32toUNORM<u16>>,
		    Adapter<f32, F16toF32, F32toUNORM<u32>>,

		    // 32 -> (8, 16, 32)
		    F32toUNORM<u8>, F32toUNORM<u16>, F32toUNORM<u32>,
		};

		ConvertFn* FLOATtoSNORMFns[] = {
		    // 8 -> (8, 16, 32)
		    nullptr, nullptr, nullptr,

		    // 16 -> (8, 16, 32)
		    Adapter<f32, F16toF32, F32toSNORM<i8>>, Adapter<f32, F16toF32, F32toSNORM<i16>>,
		    Adapter<f32, F16toF32, F32toSNORM<i32>>,

		    // 32 -> (8, 16, 32)
		    F32toSNORM<i8>, F32toSNORM<i16>, F32toSNORM<i32>,
		};

		ConvertFn* UNORMtoFLOATFns[] = {
		    // 8 -> (8, 16, 32)
		    nullptr, Adapter<f32, UNORMtoF32<u8>, F32toF16>, UNORMtoF32<u8>,

		    // 16 -> (8, 16, 32)
		    nullptr, Adapter<f32, UNORMtoF32<u16>, F32toF16>, UNORMtoF32<u16>,

		    // 32 -> (8, 16, 32)
		    nullptr, Adapter<f32, UNORMtoF32<u16>, F32toF16>, UNORMtoF32<u32>,
		};

		ConvertFn* SNORMtoFLOATFns[] = {
		    // 8 -> (8, 16, 32)
		    nullptr, Adapter<f32, SNORMtoF32<i8>, F32toF16>, SNORMtoF32<i8>,

		    // 16 -> (8, 16, 32)
		    nullptr, Adapter<f32, SNORMtoF32<i16>, F32toF16>, SNORMtoF32<i16>,

		    // 32 -> (8, 16, 32)
		    nullptr, Adapter<f32, SNORMtoF32<i32>, F32toF16>, SNORMtoF32<i32>,
		};

		ConvertFn* UNORMtoUNORMFns[] = {
		    // 8 -> (8, 16, 32)
		    Copy<1>, Adapter<f32, UNORMtoF32<u8>, F32toUNORM<u16>>, Adapter<f32, UNORMtoF32<u8>, F32toUNORM<u32>>,

		    // 16 -> (8, 16, 32)
		    Adapter<f32, UNORMtoF32<u16>, F32toUNORM<u8>>, Copy<2>, Adapter<f32, UNORMtoF32<u16>, F32toUNORM<u32>>,

		    // 32 -> (8, 16, 32)
		    Adapter<f32, UNORMtoF32<u32>, F32toUNORM<u8>>, Adapter<f32, UNORMtoF32<u32>, F32toUNORM<u16>>, Copy<4>,
		};

		ConvertFn* SNORMtoSNORMFns[] = {
		    // 8 -> (8, 16, 32)
		    Copy<1>, Adapter<f32, SNORMtoF32<i8>, F32toSNORM<i16>>, Adapter<f32, SNORMtoF32<i8>, F32toSNORM<i32>>,

		    // 16 -> (8, 16, 32)
		    Adapter<f32, SNORMtoF32<i16>, F32toSNORM<i8>>, Copy<2>, Adapter<f32, SNORMtoF32<i16>, F32toSNORM<i32>>,

		    // 32 -> (8, 16, 32)
		    Adapter<f32, SNORMtoF32<i32>, F32toSNORM<i8>>, Adapter<f32, SNORMtoF32<i32>, F32toSNORM<i16>>, Copy<4>,
		};

		ConvertFn* UNORMtoSNORMFns[] = {
		    // 8 -> (8, 16, 32)
		    Adapter<f32, UNORMtoF32<u8>, F32toSNORM<i8>>, Adapter<f32, UNORMtoF32<u8>, F32toSNORM<i16>>,
		    Adapter<f32, UNORMtoF32<u8>, F32toSNORM<i32>>,

		    // 16 -> (8, 16, 32)
		    Adapter<f32, UNORMtoF32<u16>, F32toSNORM<i8>>, Adapter<f32, UNORMtoF32<u16>, F32toSNORM<i16>>,
		    Adapter<f32, UNORMtoF32<u16>, F32toSNORM<i32>>,

		    // 32 -> (8, 16, 32)
		    Adapter<f32, UNORMtoF32<u32>, F32toSNORM<i8>>, Adapter<f32, UNORMtoF32<u32>, F32toSNORM<i16>>,
		    Adapter<f32, UNORMtoF32<u32>, F32toSNORM<i32>>,
		};

		ConvertFn* SNORMtoUNORMFns[] = {
		    // 8 -> (8, 16, 32)
		    Adapter<f32, SNORMtoF32<i8>, F32toUNORM<u16>>, Adapter<f32, SNORMtoF32<i8>, F32toUNORM<u16>>,
		    Adapter<f32, SNORMtoF32<i8>, F32toUNORM<u32>>,

		    // 16 -> (8, 16, 32)
		    Adapter<f32, SNORMtoF32<i16>, F32toUNORM<u8>>, Adapter<f32, SNORMtoF32<i16>, F32toUNORM<u16>>,
		    Adapter<f32, SNORMtoF32<i16>, F32toUNORM<u32>>,

		    // 32 -> (8, 16, 32)
		    Adapter<f32, SNORMtoF32<i32>, F32toUNORM<u8>>, Adapter<f32, SNORMtoF32<i32>, F32toUNORM<u16>>,
		    Adapter<f32, SNORMtoF32<i32>, F32toUNORM<u32>>,
		};

		ConvertFn* UINTtoUINTFns[] = {
		    // 8 -> (8, 16, 32)
		    AtoB<u8, u8>, AtoB<u8, u16>, AtoB<u8, u32>,

		    // 16 -> (8, 16, 32)
		    AtoB<u16, u8>, AtoB<u16, u16>, AtoB<u16, u32>,

		    // 32 -> (8, 16, 32)
		    AtoB<u32, u8>, AtoB<u32, u16>, AtoB<u32, u32>,
		};

		ConvertFn* UINTtoSINTFns[] = {
		    // 8 -> (8, 16, 32)
		    AtoB<u8, i8>, AtoB<u8, i16>, AtoB<u8, i32>,

		    // 16 -> (8, 16, 32)
		    AtoB<u16, i8>, AtoB<u16, i16>, AtoB<u16, i32>,

		    // 32 -> (8, 16, 32)
		    AtoB<u32, i8>, AtoB<u32, i16>, AtoB<u32, i32>,
		};

		ConvertFn* SINTtoUINTFns[] = {
		    // 8 -> (8, 16, 32)
		    AtoB<i8, u8>, AtoB<i8, u16>, AtoB<i8, u32>,

		    // 16 -> (8, 16, 32)
		    AtoB<i16, u8>, AtoB<i16, u16>, AtoB<i16, u32>,

		    // 32 -> (8, 16, 32)
		    AtoB<i32, u8>, AtoB<i32, u16>, AtoB<i32, u32>,
		};

		ConvertFn* SINTtoSINTFns[] = {
		    // 8 -> (8, 16, 32)
		    AtoB<i8, i8>, AtoB<i8, i16>, AtoB<i8, i32>,

		    // 16 -> (8, 16, 32)
		    AtoB<i16, i8>, AtoB<i16, i16>, AtoB<i16, i32>,

		    // 32 -> (8, 16, 32)
		    AtoB<i32, i8>, AtoB<i32, i16>, AtoB<i32, i32>,
		};

		ConvertFn* FLOATtoUINTFns[] = {
		    // 8 -> (8, 16, 32)
		    nullptr, nullptr, nullptr,

		    // 16 -> (8, 16, 32)
		    Adapter<f32, F16toF32, AtoB<f32, u8>>, Adapter<f32, F16toF32, AtoB<f32, u16>>,
		    Adapter<f32, F16toF32, AtoB<f32, u32>>,

		    // 32 -> (8, 16, 32)
		    AtoB<f32, u8>, AtoB<f32, u16>, AtoB<f32, u32>,
		};

		ConvertFn* UINTtoFLOATFns[] = {
		    // 8 -> (8, 16, 32)
		    nullptr, Adapter<f32, AtoB<u8, f32>, F32toF16>, AtoB<u8, f32>,

		    // 16 -> (8, 16, 32)
		    nullptr, Adapter<f32, AtoB<u16, f32>, F32toF16>, AtoB<u16, f32>,

		    // 32 -> (8, 16, 32)
		    nullptr, Adapter<f32, AtoB<u32, f32>, F32toF16>, AtoB<u32, f32>,
		};

		ConvertFn* FLOATtoSINTFns[] = {
		    // 8 -> (8, 16, 32)
		    nullptr, nullptr, nullptr,

		    // 16 -> (8, 16, 32)
		    Adapter<f32, F16toF32, AtoB<f32, u8>>, Adapter<f32, F16toF32, AtoB<f32, u16>>,
		    Adapter<f32, F16toF32, AtoB<f32, u32>>,

		    // 32 -> (8, 16, 32)
		    AtoB<f32, u8>, AtoB<f32, u16>, AtoB<f32, u32>,
		};

		ConvertFn* SINTtoFLOATFns[] = {
		    // 8 -> (8, 16, 32)
		    nullptr, Adapter<f32, AtoB<i8, f32>, F32toF16>, AtoB<i8, f32>,

		    // 16 -> (8, 16, 32)
		    nullptr, Adapter<f32, AtoB<i16, f32>, F32toF16>, AtoB<i16, f32>,

		    // 32 -> (8, 16, 32)
		    nullptr, Adapter<f32, AtoB<i32, f32>, F32toF16>, AtoB<i32, f32>,
		};

		ConvertFnArray TypeToTypeFns[] = {
		    // FLOAT -> (FLOAT, UNORM, SNORM, UINT, SINT)
		    FLOATtoFLOATFns, FLOATtoUNORMFns, FLOATtoSNORMFns, FLOATtoUINTFns, FLOATtoSINTFns,

		    // UNORM -> (FLOAT, UNORM, SNORM, UINT, SINT)
		    UNORMtoFLOATFns, UNORMtoUNORMFns, UNORMtoSNORMFns, nullptr, nullptr,

		    // SNORM -> (FLOAT, UNORM, SNORM, UINT, SINT)
		    SNORMtoFLOATFns, SNORMtoUNORMFns, SNORMtoSNORMFns, nullptr, nullptr,

		    // UINT -> (FLOAT, UNORM, SNORM, UINT, SINT)
		    UINTtoFLOATFns, nullptr, nullptr, UINTtoUINTFns, UINTtoSINTFns,

		    // SINT -> (FLOAT, UNORM, SNORM, UINT, SINT)
		    SINTtoFLOATFns, nullptr, nullptr, SINTtoUINTFns, SINTtoSINTFns,
		};

		// clang-format on
	}

	bool Convert(StreamDesc outStream, StreamDesc inStream, i32 num, i32 components)
	{
		static const i32 NUM_BIT_SIZES = 3;
		static const i32 NUM_TYPES = 5;

		u8* outStreamB = (u8*)outStream.data_;
		u8* inStreamB = (u8*)inStream.data_;

		switch(outStream.numBits_)
		{
		case 8:
		case 16:
		case 32:
			break;
		default:
			return false;
		}

		switch(inStream.numBits_)
		{
		case 8:
		case 16:
		case 32:
			break;
		default:
			return false;
		}

		i32 outStreamFnIdx = (32 - Core::CountLeadingZeros((u32)outStream.numBits_)) - 4;
		i32 inStreamFnIdx = (32 - Core::CountLeadingZeros((u32)inStream.numBits_)) - 4;

		if(outStreamFnIdx < 0 || outStreamFnIdx > 2 || inStreamFnIdx < 0 || inStreamFnIdx > 2)
		{
			return false;
		}

		const i32 typeToTypeIdx = (i32)outStream.dataType_ + (i32)inStream.dataType_ * NUM_TYPES;
		const i32 bitToBitIdx = outStreamFnIdx + inStreamFnIdx * NUM_BIT_SIZES;

		if(typeToTypeIdx >= (NUM_TYPES * NUM_TYPES))
			return false;
		if(bitToBitIdx >= (NUM_BIT_SIZES * NUM_BIT_SIZES))
			return false;

		auto typeConvFns = TypeToTypeFns[typeToTypeIdx];
		if(typeConvFns == nullptr)
			return false;

		auto convertFn = typeConvFns[bitToBitIdx];
		if(convertFn == nullptr)
			return false;

		for(i32 i = 0; i < num; ++i)
		{
			convertFn(outStreamB, inStreamB, components);

			outStreamB += outStream.stride_;
			inStreamB += inStream.stride_;
		}

		return true;
	}

} // namespace Core