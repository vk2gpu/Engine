#pragma once

#include "core/portability.h"

#if JOB_EXPORT
#define JOB_DLL EXPORT
#else
#define JOB_DLL IMPORT
#endif

#if CODE_INLINE
#define JOB_DLL_INLINE INLINE
#else
#define JOB_DLL_INLINE JOB_DLL
#endif
