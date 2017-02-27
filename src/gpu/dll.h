#pragma once

#include "core/portability.h"

#if COMPILER_MSVC
#if GPU_EXPORT
#define GPU_DLL __declspec(dllexport)
#else
#define GPU_DLL __declspec(dllimport)
#endif
#else
#define GPU_DLL
#endif

#if CODE_INLINE
#define GPU_DLL_INLINE
#else
#define GPU_DLL_INLINE GPU_DLL
#endif
