#pragma once

#include "core/portability.h"

#if CORE_EXPORT
#define CORE_DLL EXPORT
#else
#define CORE_DLL IMPORT
#endif

#if CODE_INLINE
#define CORE_DLL_INLINE INLINE
#else
#define CORE_DLL_INLINE CORE_DLL
#endif
