#pragma once

#include "core/portability.h"

#if COMPILER_MSVC
#  if PLUGIN_EXPORT
#    define PLUGIN_DLL __declspec(dllexport)
#  else
#    define PLUGIN_DLL __declspec(dllimport)
#  endif
#else
#  define PLUGIN_DLL
#endif

#if CODE_INLINE
#define PLUGIN_DLL_INLINE
#else
#define PLUGIN_DLL_INLINE PLUGIN_DLL
#endif
