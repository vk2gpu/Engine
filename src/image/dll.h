#pragma once

#include "core/portability.h"

#if IMAGE_EXPORT
#define IMAGE_DLL EXPORT
#else
#define IMAGE_DLL IMPORT
#endif

#if IMAGE_INLINE
#define IMAGE_DLL_INLINE INLINE
#else
#define IMAGE_DLL_INLINE IMAGE_DLL
#endif
