#pragma once

#include "core/types.h"
#include "core/dll.h"

namespace Core
{
	static const f32 F32_EPSILON(1e-24f);
	static const f32 F32_PI(3.14159265358979310f);
	static const f32 F32_PIMUL2(6.28318530717958620f);
	static const f32 F32_PIMUL4(12.5663706143591720f);
	static const f32 F32_PIDIV2(1.57079632679489660f);
	static const f32 F32_PIDIV4(0.78539816339744828f);

	static const f32 F32_MIN(1.175494351e-38F);
	static const f32 F32_MAX(3.402823466e+38f);

	static const u32 F32_SIGN_MASK(0x80000000);
	static const u32 F32_EXP_MASK(0x7F800000);
	static const u32 F32_FRAC_MASK(0x007FFFFF);
	static const u32 F32_SNAN_MASK(0x00400000);

	inline bool CompareFloat(f32 a, f32 b, f32 e = F32_EPSILON)
	{
		f32 c = (a - b);
		return c > 0.0f ? (c > e) : (c < -e);
	}

	CORE_DLL bool CheckFloat(f32 T);
}
