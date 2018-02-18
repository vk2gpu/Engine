#pragma once

#include "core/dll.h"
#include "core/allocator.h"
#include "core/array.h"

namespace Core
{
	/**
	 * Virtual memory allocator.
	 * Will allocate/deallocate virtual memory directly from the operating system.
	 * This is not considered to be a cheap or general purpose allocator.
	 * It is thread safe.
	 */
	class CORE_DLL AllocatorVirtual : public IAllocator
	{
	public:
		/**
		 * @param enableGuardPages Enable guard pages around allocations.
		 */
		AllocatorVirtual(bool enableGuardPages);
		~AllocatorVirtual();

		void* Allocate(i64 bytes, i64 align) override;
		void Deallocate(void* mem) override;
		bool OwnAllocation(void* mem) override;
		i64 GetAllocationSize(void* mem) override;

	private:
		struct AllocatorVirtualImpl* impl_ = nullptr;
	};

} // namespace Core
