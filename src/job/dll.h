#pragma once

#include "core/portability.h"

#if COMPILER_MSVC
#  if JOB_EXPORT
#    define JOB_DLL __declspec(dllexport)
#  else
#    define JOB_DLL __declspec(dllimport)
#  endif
#else
#  define JOB_DLL
#endif

#if CODE_INLINE
#define JOB_DLL_INLINE INLINE
#else
#define JOB_DLL_INLINE JOB_DLL
#endif
