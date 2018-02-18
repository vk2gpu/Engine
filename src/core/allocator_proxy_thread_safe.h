#pragma once

#include "core/dll.h"
#include "core/allocator.h"
#include "core/concurrency.h"

namespace Core
{
	/**
	 * Thread-safe allocator proxy.
	 * This is to wrap around an allocator and provide thread safety.
	 */
	class CORE_DLL AllocatorProxyThreadSafe : public IAllocator
	{
	public:
		AllocatorProxyThreadSafe(IAllocator& allocator);
		~AllocatorProxyThreadSafe();

		void* Allocate(i64 bytes, i64 align) override;
		void Deallocate(void* mem) override;
		bool OwnAllocation(void* mem) override;
		i64 GetAllocationSize(void* mem) override;
		AllocatorStats GetStats() const override;
		void LogStats() const override;
		void LogAllocs() const override;

	private:
		IAllocator& allocator_;
		Core::RWLock rwLock_;
	};

} // namespace Core
