#pragma once

#include "core/dll.h"
#include "core/types.h"

namespace Core
{
	CORE_DLL void HalfToFloat(const u16* in, f32* out, i32 num);
	CORE_DLL void FloatToHalf(const f32* in, u16* out, i32 num);

} // namespace Core
