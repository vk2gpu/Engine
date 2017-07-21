#pragma once

#include "core/dll.h"
#include "core/types.h"

namespace Core
{
	/**
	 * Log to debug channel of the platform.
	 */
	CORE_DLL void Log(const char* Text, va_list ArgList);

	/**
	 * Log to debug channel of the platform.
	 */
	CORE_DLL void Log(const char* Text, ...);

	/**
	 * Internal assertion.
	 */
	CORE_DLL bool AssertInternal(const char* pMessage, const char* pFile, int Line, ...);

	/**
	 * @return Is debugger attached?
	 */
	CORE_DLL bool IsDebuggerAttached();

} // namespace Core

//////////////////////////////////////////////////////////////////////////
// Macros
#if defined(DEBUG) || defined(RELEASE)
#if PLATFORM_WINDOWS
#define DBG_BREAK __debugbreak()
#else
#define DBG_BREAK
#endif
#define DBG_ASSERT_MSG(Condition, Message, ...)                                                                        \
	if(!(Condition))                                                                                                   \
	{                                                                                                                  \
		if(Core::AssertInternal(Message, __FILE__, __LINE__, ##__VA_ARGS__))                                           \
			DBG_BREAK;                                                                                                 \
	}
#define DBG_ASSERT(Condition)                                                                                          \
	if(!(Condition))                                                                                                   \
	{                                                                                                                  \
		if(Core::AssertInternal(#Condition, __FILE__, __LINE__))                                                       \
			DBG_BREAK;                                                                                                 \
	}

#define DBG_LOG(...) Core::Log(##__VA_ARGS__)

#else
#define DBG_BREAK
#define DBG_ASSERT_MSG(Condition, Message, ...)
#define DBG_ASSERT(Condition)
#define DBG_LOG(...)
#endif
