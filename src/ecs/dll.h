#pragma once

#include "core/portability.h"

#if ECS_EXPORT
#define ECS_DLL EXPORT
#else
#define ECS_DLL IMPORT
#endif

#if ECS_INLINE
#define ECS_DLL_INLINE INLINE
#else
#define ECS_DLL_INLINE ECS_DLL
#endif
