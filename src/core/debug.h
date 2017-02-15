#pragma once

#include "core/types.h"

//////////////////////////////////////////////////////////////////////////
// Logging
void DbgLog(const char* Text, ...);

//////////////////////////////////////////////////////////////////////////
// DbgAssertInternal
bool DbgAssertInternal(const char* pMessage, const char* pFile, int Line, ...);

//////////////////////////////////////////////////////////////////////////
// Macros
#if defined(DEBUG) || defined(RELEASE)
#define DBG_BREAK __debugbreak()
#define DBG_ASSERT_MSG(Condition, Message, ...)                                                                        \
	if(!(Condition))                                                                                                   \
	{                                                                                                                  \
		if(DbgAssertInternal(Message, __FILE__, __LINE__, ##__VA_ARGS__))                                              \
			DBG_BREAK;                                                                                                 \
	}
#define DBG_ASSERT(Condition)                                                                                          \
	if(!(Condition))                                                                                                   \
	{                                                                                                                  \
		if(DbgAssertInternal(#Condition, __FILE__, __LINE__))                                                          \
			DBG_BREAK;                                                                                                 \
	}

#define DBG_LOG(...) DbgLog(##__VA_ARGS__)

#else
#define DBG_BREAK
#define DBG_ASSERT_MSG(Condition, Message, ...)
#define DBG_ASSERT(Condition)
#define DBG_LOG(...)
#endif
