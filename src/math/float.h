#pragma once

#include "core/types.h"
#include "math/dll.h"

//////////////////////////////////////////////////////////////////////////
// Defines
#define F32_EPSILON (1e-24f)
#define F32_PI (3.14159265358979310f)
#define F32_PIMUL2 (6.28318530717958620f)
#define F32_PIMUL4 (12.5663706143591720f)
#define F32_PIDIV2 (1.57079632679489660f)
#define F32_PIDIV4 (0.78539816339744828f)

//////////////////////////////////////////////////////////////////////////
// Utility
MATH_DLL bool CheckFloat(f32 T);
