#include "core/library.h"

#if PLATFORM_WINDOWS
#include "core/os.h"
#endif

#if PLATFORM_LINUX
#include <dlfcn.h>
#endif

namespace Core
{
	LibHandle LibraryOpen(const char* libName)
	{
#if PLATFORM_WINDOWS
		HMODULE handle = ::LoadLibraryA(libName);
		return (LibHandle)handle;
#elif PLATFORM_LINUX
		return (LibHandle)dlopen(libName, RTLD_NOW);
#else
#error "Unimplemented."
#endif
	}

	void LibraryClose(LibHandle handle)
	{
#if PLATFORM_WINDOWS
		::FreeLibrary((HMODULE)handle);
#elif PLATFORM_LINUX
		dlclose((void*)handle);
#else
#error "Unimplemented."
#endif
	}

	void* LibrarySymbol(LibHandle handle, const char* symbolName)
	{
#if PLATFORM_WINDOWS
		return ::GetProcAddress((HMODULE)handle, symbolName);
#elif PLATFORM_LINUX
		return dlsym((void*)handle, symbolName);
#else
#error "Unimplemented."
#endif
	}
} // namespace Core