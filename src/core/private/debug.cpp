#include "core/debug.h"

#include <cstdio>

#if PLATFORM_WINDOWS
#include <windows.h>
#endif


//////////////////////////////////////////////////////////////////////////
// DbgLog
void DbgLog( const char* Text, ... )
{
	static char MessageBuffer[ 4096 ];
	va_list ArgList;
	va_start( ArgList, Text );
#if COMPILER_MSVC
	vsprintf_s( MessageBuffer, Text, ArgList );
#else
	vsprintf( MessageBuffer, Text, ArgList );
#endif

#if PLATFORM_WINDOWS
	::OutputDebugStringA( MessageBuffer );
#endif

#if PLATFORM_ANDROID
	__android_log_print( ANDROID_LOG_INFO, "Engine", MessageBuffer );
#endif
	printf( "%s", MessageBuffer );
}

//////////////////////////////////////////////////////////////////////////
// DbgAssertInternal
bool DbgAssertInternal(const char* Message, const char* File, int Line, ...)
{
#if defined(DEBUG) || defined(RELEASE)
	static char MessageBuffer[ 4096 ] = { 0 };
	va_list ArgList;
	va_start( ArgList, Line );
#if COMPILER_MSVC
	vsprintf_s(MessageBuffer, Message, ArgList);
#else
	vsprintf(MessageBuffer, Message, ArgList);
#endif
	va_end(ArgList);

	DbgLog("\"%s\" in %s on line %u.\n\nDo you wish to break?", MessageBuffer, File, Line);

	return true;
#else
	return false;
#endif
}
