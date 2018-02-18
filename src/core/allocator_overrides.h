#include "core/allocator.h"
#include <new>

#define DECLARE_MODULE_ALLOCATOR(_name)                                                                                \
	inline static Core::IAllocator& GetAllocator()                                                                     \
	{                                                                                                                  \
		static Core::IAllocator& alloc = Core::CreateAllocationTracker(Core::GeneralAllocator(), "General/" _name);    \
		return alloc;                                                                                                  \
	}                                                                                                                  \
	void* operator new(size_t count) { return GetAllocator().Allocate(count, PLATFORM_ALIGNMENT); }                    \
	void* operator new[](size_t count) { return GetAllocator().Allocate(count, PLATFORM_ALIGNMENT); }                  \
	void operator delete(void* mem) { return GetAllocator().Deallocate(mem); }                                         \
	void operator delete[](void* mem) { return GetAllocator().Deallocate(mem); }
