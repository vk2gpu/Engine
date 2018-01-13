#pragma once

#include "core/portability.h"

#if MATH_EXPORT
#define MATH_DLL EXPORT
#else
#define MATH_DLL IMPORT
#endif

#if CODE_INLINE
#define MATH_DLL_INLINE INLINE
#else
#define MATH_DLL_INLINE MATH_DLL
#endif
