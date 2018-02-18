#pragma once

#include "core/types.h"
#include "core/dll.h"
#include "core/allocator_overrides.h"

#include <new>
#include <utility>

namespace Core
{
	class IAllocator;
}

namespace Core
{
	/**
	 * Untracked virtual memory allocator.
	 */
	CORE_DLL IAllocator& UntrackedVirtualAllocator();

	/**
	 * Virtual memory allocator.
	 */
	CORE_DLL IAllocator& VirtualAllocator();

	/**
	 * General purpose allocator.
	 */
	CORE_DLL IAllocator& GeneralAllocator();

	/**
	 * Create allocation tracker.
	 * @param Allocator to use.
	 * @param name Name to assign to the tracker.
	 */
	CORE_DLL IAllocator& CreateAllocationTracker(IAllocator& allocator, const char* name);

	/**
	 * Allocator stats.
	 */
	struct AllocatorStats
	{
		i64 numAllocations_ = 0;
		i64 peakUsage_ = 0;
		i64 usage_ = 0;
	};

	/**
	 * Allocator interface.
	 * Used to implement custom allocators.
	 */
	class CORE_DLL IAllocator
	{
	public:
		IAllocator() {}
		virtual ~IAllocator() {}

		virtual void* Allocate(i64 bytes, i64 align) = 0;
		virtual void Deallocate(void* mem) = 0;

		virtual bool OwnAllocation(void* mem) = 0;
		virtual i64 GetAllocationSize(void* mem) = 0;
		virtual AllocatorStats GetStats() const { return AllocatorStats(); };
		virtual void LogStats() const {}
		virtual void LogAllocs() const {}

		template<typename TYPE, typename... ARGS>
		TYPE* New(ARGS&&... args)
		{
			void* mem = Allocate(sizeof(TYPE), alignof(TYPE));
			return new(mem) TYPE(std::forward<ARGS>(args)...);
		}

		template<typename TYPE>
		void Delete(TYPE* obj)
		{
			if(obj)
			{
				obj->~TYPE();
				Deallocate(obj);
			}
		}
	};

	/**
	 * Allocator for use with containers.
	 */
	class CORE_DLL ContainerAllocator
	{
	public:
		ContainerAllocator() = default;
		ContainerAllocator(const ContainerAllocator&) = default;
		~ContainerAllocator() = default;

		void* Allocate(i64 size, i64 align) { return GeneralAllocator().Allocate(size, align); }
		void Deallocate(void* mem) { GeneralAllocator().Deallocate(mem); }
	};

} // namespace Core

inline void* operator new(size_t count, Core::IAllocator& allocator) noexcept
{
	return allocator.Allocate(count, PLATFORM_ALIGNMENT);
}

inline void* operator new[](size_t count, Core::IAllocator& allocator) noexcept
{
	return allocator.Allocate(count, PLATFORM_ALIGNMENT);
}

inline void operator delete(void* mem, Core::IAllocator& allocator) noexcept
{
	return allocator.Deallocate(mem);
}

inline void operator delete[](void* mem, Core::IAllocator& allocator) noexcept
{
	return allocator.Deallocate(mem);
}
