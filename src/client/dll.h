#pragma once

#include "core/portability.h"

#if COMPILER_MSVC
#  if CLIENT_EXPORT
#    define CLIENT_DLL __declspec(dllexport)
#  else
#    define CLIENT_DLL __declspec(dllimport)
#  endif
#else
#  define CLIENT_DLL
#endif

#if CODE_INLINE
#define CLIENT_DLL_INLINE
#else
#define CLIENT_DLL_INLINE CLIENT_DLL
#endif
