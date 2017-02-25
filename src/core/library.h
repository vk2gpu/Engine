#pragma once

#include "core/types.h"
#include "core/dll.h"

namespace Core
{
	typedef void* LibHandle;

	/**
	 * Open library (.so, .dll, .dylib).
	 * @param libName Name of libtary.
	 */
	CORE_DLL LibHandle LibraryOpen(const char* libName);

	/**
	 * Close library.
	 */
	CORE_DLL void LibraryClose(LibHandle handle);

	/**
	 * Get library symbol.
	 */
	CORE_DLL void* LibrarySymbol(LibHandle handle, const char* symbolName);

} // namespace Core
