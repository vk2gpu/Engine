#pragma once

#include "core/portability.h"

#if IMGUI_EXPORT
#define IMGUI_DLL EXPORT
#else
#define IMGUI_DLL IMPORT
#endif

#if CODE_INLINE
#define IMGUI_DLL_INLINE
#else
#define IMGUI_DLL_INLINE IMGUI_DLL
#endif
