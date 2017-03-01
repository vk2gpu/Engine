#pragma once

#include "core/portability.h"

#if CLIENT_EXPORT
#define CLIENT_DLL EXPORT
#else
#define CLIENT_DLL IMPORT
#endif

#if CODE_INLINE
#define CLIENT_DLL_INLINE
#else
#define CLIENT_DLL_INLINE CLIENT_DLL
#endif
