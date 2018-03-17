#pragma once

#include "core/memory.h"
#include "core/debug.h"
#include <new>

#if 0
void* operator new(size_t count)
{
	DBG_BREAK;
	return ::new (count);
}

void* operator new[](size_t count)
{
	DBG_BREAK;
	return nullptr;
}

void operator delete(void* mem)
{
	DBG_BREAK;
}

void operator delete[](void* mem)
{
	DBG_BREAK;
}

#endif
