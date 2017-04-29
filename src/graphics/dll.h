#pragma once

#include "core/portability.h"

#if GRAPHICS_EXPORT
#define GRAPHICS_DLL EXPORT
#else
#define GRAPHICS_DLL IMPORT
#endif

#if CODE_INLINE
#define GRAPHICS_DLL_INLINE
#else
#define GRAPHICS_DLL_INLINE GRAPHICS_DLL
#endif
