#pragma once

#include "core/portability.h"

#if GPU_D3D12_DLL_EXPORT
#define GPU_D3D12_DLL EXPORT
#else
#define GPU_D3D12_DLL IMPORT
#endif

#if CODE_INLINE
#define GPU_D3D12_DLL_INLINE
#else
#define GPU_D3D12_DLL_INLINE GPU_DLL
#endif
