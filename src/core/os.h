#pragma once

#if PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#undef DrawState
#undef MessageBox
#undef ERROR
#undef DELETE
#undef LoadImage

#endif
