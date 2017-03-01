#pragma once

#include "core/portability.h"

#if GPU_EXPORT
#define GPU_DLL EXPORT
#else
#define GPU_DLL IMPORT
#endif

#if CODE_INLINE
#define GPU_DLL_INLINE
#else
#define GPU_DLL_INLINE GPU_DLL
#endif
