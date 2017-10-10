#include "core/debug.h"
#include "core/concurrency.h"
#include "core/os.h"

#include "Remotery.h"

#include <cstdio>

namespace Core
{
	namespace
	{
		char* GetMessageBuffer(i32& size)
		{
			static TLS tls;
			size = 64 * 1024;
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
		vsprintf_s(MessageBuffer, MessageBufferSize, Text, ArgList);
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
		char MessageBuffer[4096];
		va_list ArgList;
		va_start(ArgList, Line);
#if COMPILER_MSVC
		vsprintf_s(MessageBuffer, sizeof(MessageBuffer), Message, ArgList);
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

	namespace
	{
		Core::Mutex mbMutex_;
	}

	MessageBoxReturn MessageBox(const char* title, const char* message, MessageBoxType type, MessageBoxIcon icon)
	{
		Core::ScopedMutex lock(mbMutex_);

#if PLATFORM_WINDOWS
		UINT mbType = MB_TASKMODAL | MB_SETFOREGROUND | MB_TOPMOST;
		switch(type)
		{
		case MessageBoxType::OK:
			mbType |= MB_OK;
			break;
		case MessageBoxType::OK_CANCEL:
			mbType |= MB_OKCANCEL;
			break;
		case MessageBoxType::YES_NO:
			mbType |= MB_YESNO;
			break;
		case MessageBoxType::YES_NO_CANCEL:
			mbType |= MB_YESNOCANCEL;
			break;
		}

		switch(icon)
		{
		case MessageBoxIcon::WARNING:
			mbType |= MB_ICONWARNING;
			break;
		case MessageBoxIcon::ERROR:
			mbType |= MB_ICONERROR;
			break;
		case MessageBoxIcon::QUESTION:
			mbType |= MB_ICONQUESTION;
			break;
		default:
			mbType |= MB_ICONWARNING;
			break;
		}

		// Log.
		Log("MB: %s: %s\n", title, message);

		// TODO: HWND!
		int RetVal = ::MessageBoxA(NULL, message, title, mbType);

		switch(RetVal)
		{
		case IDOK:
			return MessageBoxReturn::OK;
			break;
		case IDYES:
			return MessageBoxReturn::YES;
			break;
		case IDNO:
			return MessageBoxReturn::NO;
			break;
		case IDCANCEL:
			return MessageBoxReturn::CANCEL;
			break;
		default:
			break;
		};

#endif
		return MessageBoxReturn::OK;
	}

	i32 GetCallstack(i32 skipFrames, void** addresses, i32 maxAddresses, i32* stackHash)
	{
#if PLATFORM_WINDOWS
		return ::CaptureStackBackTrace(skipFrames + 1, maxAddresses, addresses, (DWORD*)stackHash);
#else
		return 0;
#endif
	}


} // namespace Core
