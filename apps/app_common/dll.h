#pragma once

#include "core/portability.h"

#if APP_COMMON_EXPORT
#define APP_COMMON_DLL EXPORT
#else
#define APP_COMMON_DLL IMPORT
#endif

#if CODE_INLINE
#define APP_COMMON_DLL_INLINE
#else
#define APP_COMMON_DLL_INLINE APP_COMMON_DLL
#endif
