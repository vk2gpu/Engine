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
#undef TRANSPARENT

#elif defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID) || defined(PLATFORM_OSX) || defined(PLATFORM_IOS)
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>

#define _snprintf snprintf
#define _vsnprintf vsnprintf
#define _stricmp strcasecmp
#define _strnicmp strncasecmp

#endif
