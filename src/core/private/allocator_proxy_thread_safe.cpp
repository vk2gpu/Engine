#include "core/allocator_proxy_thread_safe.h"

namespace Core
{
	AllocatorProxyThreadSafe::AllocatorProxyThreadSafe(IAllocator& allocator)
	    : allocator_(allocator)
	{
	}

	AllocatorProxyThreadSafe::~AllocatorProxyThreadSafe() {}

	void* AllocatorProxyThreadSafe::Allocate(i64 bytes, i64 align)
	{
		Core::ScopedWriteLock lock(rwLock_);
		return allocator_.Allocate(bytes, align);
	}

	void AllocatorProxyThreadSafe::Deallocate(void* mem)
	{
		Core::ScopedWriteLock lock(rwLock_);
		allocator_.Deallocate(mem);
	}

	bool AllocatorProxyThreadSafe::OwnAllocation(void* mem)
	{
		Core::ScopedReadLock lock(rwLock_);
		return allocator_.OwnAllocation(mem);
	}

	i64 AllocatorProxyThreadSafe::GetAllocationSize(void* mem)
	{
		Core::ScopedReadLock lock(rwLock_);
		return allocator_.GetAllocationSize(mem);
	}

	AllocatorStats AllocatorProxyThreadSafe::GetStats() const
	{
		Core::ScopedReadLock lock(rwLock_);
		return allocator_.GetStats();
	}

	void AllocatorProxyThreadSafe::LogStats() const
	{
		Core::ScopedReadLock lock(rwLock_);
		allocator_.LogStats();
	}

	void AllocatorProxyThreadSafe::LogAllocs() const
	{
		Core::ScopedReadLock lock(rwLock_);
		allocator_.LogAllocs();
	}


} // namespace Core
