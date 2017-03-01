#pragma once

#include "core/portability.h"

#if PLUGIN_EXPORT
#define PLUGIN_DLL EXPORT
#else
#define PLUGIN_DLL IMPORT
#endif

#if CODE_INLINE
#define PLUGIN_DLL_INLINE
#else
#define PLUGIN_DLL_INLINE PLUGIN_DLL
#endif
