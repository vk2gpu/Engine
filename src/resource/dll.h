#pragma once

#include "core/portability.h"

#if RESOURCE_EXPORT
#define RESOURCE_DLL EXPORT
#else
#define RESOURCE_DLL IMPORT
#endif

#if CODE_INLINE
#define RESOURCE_DLL_INLINE
#else
#define RESOURCE_DLL_INLINE RESOURCE_DLL
#endif
