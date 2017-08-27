#include "core/debug.h"
#include "core/concurrency.h"

#include "Remotery.h"

#include <cstdio>

#if PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace Core
{
	namespace
	{
		char* GetMessageBuffer(i32& size)
		{
			static TLS tls;
			size = 4096;
			if(tls.Get() == nullptr)
			{
				char* buffer = new char[size];
				tls.Set(buffer);
			}
			return (char*)tls.Get();
		}
	}

	void LogV(const char* Text, va_list ArgList)
	{
		i32 MessageBufferSize = 0;
		char* MessageBuffer = GetMessageBuffer(MessageBufferSize);

#if COMPILER_MSVC
		vsprintf_s(MessageBuffer, 4096, Text, ArgList);
#else
		vsprintf(MessageBuffer, Text, ArgList);
#endif

#if PLATFORM_WINDOWS
		::OutputDebugStringA(MessageBuffer);
#endif

#if PLATFORM_ANDROID
		__android_log_print(ANDROID_LOG_INFO, "Engine", MessageBuffer);
#endif
		printf("%s", MessageBuffer);
		rmt_LogText(MessageBuffer);
	}

	void Log(const char* Text, ...)
	{
		va_list ArgList;
		va_start(ArgList, Text);
		LogV(Text, ArgList);
		va_end(ArgList);
	}

	bool AssertInternal(const char* Message, const char* File, int Line, ...)
	{
#if defined(DEBUG) || defined(RELEASE)
		i32 MessageBufferSize = 0;
		char* MessageBuffer = GetMessageBuffer(MessageBufferSize);
		va_list ArgList;
		va_start(ArgList, Line);
#if COMPILER_MSVC
		vsprintf_s(MessageBuffer, MessageBufferSize, Message, ArgList);
#else
		vsprintf(MessageBuffer, Message, ArgList);
#endif
		va_end(ArgList);

		Log("\"%s\" in %s on line %u.\n\nDo you wish to break?", MessageBuffer, File, Line);

		return true;
#else
		return false;
#endif
	}

	bool IsDebuggerAttached()
	{
#if PLATFORM_WINDOWS
		return !!::IsDebuggerPresent();
#else
		return false;
#endif
	}
} // namespace Core
