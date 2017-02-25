#pragma once

#include "core/types.h"
#include "math/dll.h"

namespace Math
{
	static const f32 F32_EPSILON(1e-24f);
	static const f32 F32_PI(3.14159265358979310f);
	static const f32 F32_PIMUL2(6.28318530717958620f);
	static const f32 F32_PIMUL4(12.5663706143591720f);
	static const f32 F32_PIDIV2(1.57079632679489660f);
	static const f32 F32_PIDIV4(0.78539816339744828f);

	static const u32 F32_SIGN_MASK(0x80000000);
	static const u32 F32_EXP_MASK(0x7F800000);
	static const u32 F32_FRAC_MASK(0x007FFFFF);
	static const u32 F32_SNAN_MASK(0x00400000);

	MATH_DLL bool CheckFloat(f32 T);
}
