#pragma once

#include "core/types.h"

typedef void* LibHandle;

/**
 * Open library (.so, .dll, .dylib).
 * @param libName Name of libtary.
 */
LibHandle LibraryOpen(const char* libName);

/**
 * Close library.
 */
void LibraryClose(LibHandle handle);

/**
 * Get library symbol.
 */
void* LibrarySymbol(LibHandle handle, const char* symbolName);
