#pragma once

#include "core/portability.h"

#if COMPILER_MSVC
#  if CORE_EXPORT
#    define CORE_DLL __declspec(dllexport)
#  else
#    define CORE_DLL __declspec(dllimport)
#  endif
#else
#  define CORE_DLL
#endif

#if CODE_INLINE
#define CORE_DLL_INLINE INLINE
#else
#define CORE_DLL_INLINE CORE_DLL
#endif
