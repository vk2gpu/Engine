#include "core/debug.h"
#include "core/concurrency.h"
#include "core/os.h"
#include "core/string.h"
#include "core/vector.h"

#include "Remotery.h"

#include <cstdio>

namespace Core
{
	namespace
	{
		struct LogContext
		{
			static const int BUFFER_SIZE = 64 * 1024;
			LogContext()
			    : buffer_(BUFFER_SIZE)
			{
			}

			~LogContext() {}

			Core::Vector<char> buffer_;
		};

		LogContext* GetLogContext()
		{
			static TLS tls;
			if(tls.Get() == nullptr)
			{
				auto* context = new LogContext;
				tls.Set(context);
			}

			return (LogContext*)tls.Get();
		}

		bool enableBreakOnAssertion_ = true;
	}

	void LogV(const char* text, va_list argList)
	{
		LogContext* context = GetLogContext();
		i32 bufferSize = context->buffer_.size();

#if COMPILER_MSVC
		vsprintf_s(context->buffer_.data(), bufferSize, text, argList);
#else
		vsprintf(context, text, argList);
#endif

#if PLATFORM_WINDOWS
		::OutputDebugStringA(context->buffer_.data());
#endif

#if PLATFORM_ANDROID
		__android_log_print(ANDROID_LOG_INFO, "Engine", context);
#endif
		printf("%s", context->buffer_.data());
		rmt_LogText(context->buffer_.data());
	}

	void Log(const char* text, ...)
	{
		va_list argList;
		va_start(argList, text);
		LogV(text, argList);
		va_end(argList);
	}

	bool AssertInternal(const char* Message, const char* File, int Line, ...)
	{
#if !defined(_RELEASE)
		char context[4096];
		va_list argList;
		va_start(argList, Line);
#if COMPILER_MSVC
		vsprintf_s(context, sizeof(context), Message, argList);
#else
		vsprintf(context, Message, argList);
#endif
		va_end(argList);

		Log("\"%s\" in %s on line %u.\n\nDo you wish to break?", context, File, Line);

		return enableBreakOnAssertion_;
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

	void SetBreakOnAssertion(bool enableBreak) { enableBreakOnAssertion_ = enableBreak; }


} // namespace Core
