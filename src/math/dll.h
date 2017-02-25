#pragma once

#include "core/portability.h"

#if COMPILER_MSVC
#  if MATH_EXPORT
#    define MATH_DLL __declspec(dllexport)
#  else
#    define MATH_DLL __declspec(dllimport)
#  endif
#else
#  define MATH_DLL
#endif

#if MATH_INLINE
#define MATH_DLL_INLINE INLINE
#else
#define MATH_DLL_INLINE MATH_DLL
#endif
