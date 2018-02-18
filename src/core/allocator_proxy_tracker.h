#pragma once

#include "core/dll.h"
#include "core/allocator.h"

namespace Core
{
	/**
	 * Tracker allocator proxy.
	 * This is used to track allocations and their callstacks.
	 */
	class CORE_DLL AllocatorProxyTracker final : public IAllocator
	{
	public:
		AllocatorProxyTracker(IAllocator& allocator, const char* name);
		~AllocatorProxyTracker();

		void* Allocate(i64 bytes, i64 align) override;
		void Deallocate(void* mem) override;
		bool OwnAllocation(void* mem) override;
		i64 GetAllocationSize(void* mem) override;
		AllocatorStats GetStats() const override;
		void LogStats() const override;
		void LogAllocs() const override;

	private:
		struct AllocatorProxyTrackerImpl* impl_;
	};

} // namespace Core
#pragma once
