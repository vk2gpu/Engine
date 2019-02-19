#include "core/allocator_virtual.h"
#include "core/concurrency.h"
#include "core/misc.h"
#include "core/os.h"
#include "core/set.h"

#include <cstdio>
#include <cstdlib>
#include <new>

namespace Core
{
	namespace
	{
#if PLATFORM_WINDOWS
		void VirtualGetInfo(i64& pageSize, i64& granularity)
		{
			SYSTEM_INFO info;
			::GetSystemInfo(&info);
			pageSize = info.dwPageSize;
			granularity = info.dwAllocationGranularity;
		}

		void* VirtualAllocate(i64 size) { return ::VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_READWRITE); }

		void VirtualDeallocate(void* mem, i64 size) { ::VirtualFree(mem, 0, MEM_RELEASE); }

		void* VirtualReserve(i64 size) { return ::VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS); }

		void* VirtualCommit(void* mem, i64 size) { return ::VirtualAlloc(mem, size, MEM_COMMIT, PAGE_READWRITE); }

#elif defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID) || defined(PLATFORM_OSX) || defined(PLATFORM_IOS)
		void VirtualGetInfo(i64& pageSize, i64& granularity)
		{
			pageSize = sysconf(_SC_PAGESIZE);
			granularity = 0;
		}

		void* VirtualAllocate(i64 size) { return mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0); }

		void VirtualDeallocate(void* mem, i64 size) { munmap(mem, size); }

		void* VirtualReserve(i64 size) { return mmap(nullptr, size, PROT_NONE, MAP_PRIVATE|MAP_ANON, -1, 0); }

		void* VirtualCommit(void* mem, i64 size) { return mmap(mem, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0); }

#endif

		/**
		 * Callbacks for use with containers.
		 */
		struct AllocatorCallbacks
		{
			void* Allocate(i64 size, i64 align) { return VirtualAllocate(size); }

			void Deallocate(void* mem) { VirtualDeallocate(mem, 0 /*size*/); }
		};
	}

	struct AllocatorVirtualImpl
	{
		struct AllocInfo
		{
			void* mem_ = nullptr;
			i64 size_ = 0;

			bool operator==(const AllocInfo& a) const { return mem_ == a.mem_; }
		};

		struct AllocInfoHasher
		{
			u64 operator()(u64 input, const AllocInfo& data) const
			{
				return HashFNV1a(input, &data.mem_, sizeof(void*));
			}
		};

		volatile i64 reservedBytes_ = 0;
		i64 pageSize_ = 0;
		i64 granularity_ = 0;
		bool enableGuardPages_ = false;

		Core::RWLock rwLock_;
		Core::Set<AllocInfo, AllocInfoHasher, AllocatorCallbacks> allocInfos_;

		void AddAlloc(void* mem, i64 size)
		{
			DBG_ASSERT(mem != nullptr);
			DBG_ASSERT(size > 0);

			Core::ScopedWriteLock lock(rwLock_);
			DBG_ASSERT(!allocInfos_.find(AllocInfo{mem, 0}));
			allocInfos_.insert(AllocInfo{mem, size});
			Core::AtomicAddRel(&reservedBytes_, size);
		}

		void RemoveAlloc(void* mem)
		{
			DBG_ASSERT(mem != nullptr);

			Core::ScopedWriteLock lock(rwLock_);
			auto* alloc = allocInfos_.find(AllocInfo{mem, 0});
			DBG_ASSERT(alloc);
			allocInfos_.erase(AllocInfo{mem, 0});
			Core::AtomicAddRel(&reservedBytes_, -alloc->size_);
		}

		i64 AllocSize(void* mem)
		{
			DBG_ASSERT(mem != nullptr);

			Core::ScopedReadLock lock(rwLock_);
			if(auto* alloc = allocInfos_.find(AllocInfo{mem, 0}))
				return alloc->size_;
			return -1;
		}
	};

	AllocatorVirtual::AllocatorVirtual(bool enableGuardPages)
	{
		auto* implMem = VirtualAllocate(sizeof(AllocatorVirtualImpl));
		impl_ = new(implMem) AllocatorVirtualImpl;

		impl_->enableGuardPages_ = true;

		VirtualGetInfo(impl_->pageSize_, impl_->granularity_);
	}

	AllocatorVirtual::~AllocatorVirtual()
	{
		DBG_ASSERT(impl_->allocInfos_.size() == 0);
		DBG_ASSERT(impl_->reservedBytes_ == 0);

		impl_->~AllocatorVirtualImpl();
		VirtualDeallocate(impl_, sizeof(AllocatorVirtualImpl));
		impl_ = nullptr;
	}

	void* AllocatorVirtual::Allocate(i64 bytes, i64 align)
	{
		void* retVal = nullptr;
		bytes = Core::PotRoundUp(bytes, impl_->granularity_);
		i64 reserveBytes = bytes;

		// Add 2 extra pages to allocate.
		if(impl_->enableGuardPages_)
			reserveBytes += (impl_->granularity_ * 2);

		// Round up to required granularity.
		reserveBytes = Core::PotRoundUp(reserveBytes, impl_->granularity_);

		// Reserve.
		retVal = VirtualReserve(reserveBytes);

		// If successful, add allocation and offset into initial guard page.
		if(retVal)
		{
			impl_->AddAlloc(retVal, reserveBytes);

			if(impl_->enableGuardPages_)
				retVal = (u8*)retVal + impl_->granularity_;

			// Commit & readwrite access.
			DBG_VERIFY(VirtualCommit(retVal, bytes));
		}
		return retVal;
	}

	void AllocatorVirtual::Deallocate(void* mem)
	{
		if(mem)
		{
			if(impl_->enableGuardPages_)
				mem = (u8*)mem - impl_->granularity_;

			i64 size = impl_->AllocSize(mem);
			DBG_ASSERT(size >= 0)
			if(size >= 0)
			{
				impl_->RemoveAlloc(mem);
				VirtualDeallocate(mem, size);
			}
		}
	}

	bool AllocatorVirtual::OwnAllocation(void* mem)
	{
		if(impl_->enableGuardPages_)
			mem = (u8*)mem - impl_->granularity_;

		return impl_->AllocSize(mem) >= 0;
	}

	i64 AllocatorVirtual::GetAllocationSize(void* mem)
	{
		if(impl_->enableGuardPages_)
			mem = (u8*)mem - impl_->granularity_;

		return impl_->AllocSize(mem);
	}

} // namespace Core
