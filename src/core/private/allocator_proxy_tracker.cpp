#include "core/allocator_proxy_tracker.h"
#include "core/array.h"
#include "core/concurrency.h"
#include "core/debug.h"
#include "core/map.h"

namespace Core
{
	struct PointerHasher
	{
		u64 operator()(u64 input, const void* data) const { return HashFNV1a(input, &data, sizeof(void*)); }
	};

	struct AllocInfo
	{
		void* mem_ = nullptr;
		i64 requestSize_ = 0;
		i64 allocSize_ = 0;
		i64 numFrames_ = 0;
		Core::Array<void*, 32> callstack_ = {};
	};

	struct AllocatorProxyTrackerImpl
	{
		AllocatorProxyTrackerImpl(IAllocator& allocator)
		    : allocator_(allocator)
		    , allocInfos_(UntrackedVirtualAllocator())
		{
		}

		void AddAlloc(void* mem, i64 size)
		{
			const i64 totalAllocated = Core::AtomicIncAcq(&totalAllocated_);
			//DBG_LOG("%s %p Alloc (%lld) : %p\n", impl_->name_, impl_, totalAllocated, mem);
			DBG_ASSERT(totalAllocated > 0);

			AllocInfo allocInfo;
			allocInfo.mem_ = mem;
			allocInfo.requestSize_ = size;
			allocInfo.allocSize_ = allocator_.GetAllocationSize(mem);
			allocInfo.numFrames_ = Core::GetCallstack(2, allocInfo.callstack_.data(), allocInfo.callstack_.size());

			Core::AtomicAdd(&usage_, allocInfo.allocSize_);

			i64 prevMax = peakUsage_;

			const auto CmpExgRel = [](volatile i64* dest, i64& expected, i64 value) -> bool {
				i64 oldValue = expected;
				expected = Core::AtomicCmpExchgRel(dest, value, oldValue);
				return expected == oldValue;
			};

			while(peakUsage_ < usage_ && !CmpExgRel(&peakUsage_, prevMax, usage_))
				;

			{
				Core::ScopedWriteLock lock(rwLock_);
				DBG_ASSERT(allocInfos_.find(mem) == nullptr);
				auto oldAllocs = allocInfos_.size();
				allocInfos_.insert(mem, allocInfo);

				DBG_ASSERT(allocInfos_.find(mem));
				DBG_ASSERT(allocInfos_.size() == (oldAllocs + 1));
			}
		}

		void RemoveAlloc(void* mem)
		{
			const i64 totalAllocated = Core::AtomicDecAcq(&totalAllocated_);
			//DBG_LOG("%s %p Dealloc (%lld): %p\n", impl_->name_, impl_, totalAllocated, mem);
			DBG_ASSERT(totalAllocated >= 0);

			{
				Core::ScopedWriteLock lock(rwLock_);
				auto oldAllocs = allocInfos_.size();
				auto* allocInfo = allocInfos_.find(mem);
				DBG_ASSERT(allocInfo);

				Core::AtomicAdd(&usage_, -allocInfo->allocSize_);
				memset(mem, 0xfe, allocInfo->requestSize_);
				allocInfos_.erase(mem);

				DBG_ASSERT(allocInfos_.size() == (oldAllocs - 1));
			}
		}

		IAllocator& allocator_;
		Core::Array<char, 64> name_;
		Core::RWLock rwLock_;
		Core::Map<void*, AllocInfo, PointerHasher, IAllocator&> allocInfos_;
		volatile i64 totalAllocs_ = 0;
		volatile i64 totalDeallocs_ = 0;
		volatile i64 totalOwnAlloc_ = 0;
		volatile i64 totalGetAllocSize_ = 0;
		volatile i64 totalAllocated_ = 0;

		volatile i64 usage_ = 0;
		volatile i64 peakUsage_ = 0;
	};

	AllocatorProxyTracker::AllocatorProxyTracker(IAllocator& allocator, const char* name)
	{
		impl_ = UntrackedVirtualAllocator().New<AllocatorProxyTrackerImpl>(allocator);
		strcpy_s(impl_->name_.data(), impl_->name_.size(), name);
	}

	AllocatorProxyTracker::~AllocatorProxyTracker()
	{
		{
			Core::ScopedReadLock lock(impl_->rwLock_);

			if(impl_->totalAllocated_ > 0)
			{
				Core::Log("=====================================================\n");
				LogAllocs();
			}

			Core::Log("=====================================================\n");
			LogStats();

			DBG_ASSERT_MSG(impl_->totalAllocated_ == 0, "Memory leaks detected in %s allocator!", impl_->name_);
		}

		UntrackedVirtualAllocator().Delete(impl_);
	}

	void* AllocatorProxyTracker::Allocate(i64 bytes, i64 align)
	{
		Core::AtomicIncAcq(&impl_->totalAllocs_);
		auto mem = impl_->allocator_.Allocate(bytes, align);
		if(mem)
		{
			impl_->AddAlloc(mem, bytes);
		}
		return mem;
	}

	void AllocatorProxyTracker::Deallocate(void* mem)
	{
		Core::AtomicIncAcq(&impl_->totalDeallocs_);
		if(mem)
		{
			DBG_ASSERT(OwnAllocation(mem));
			impl_->RemoveAlloc(mem);
			impl_->allocator_.Deallocate(mem);
		}
	}

	bool AllocatorProxyTracker::OwnAllocation(void* mem)
	{
		Core::AtomicIncAcq(&impl_->totalOwnAlloc_);
		return impl_->allocator_.OwnAllocation(mem);
	}

	i64 AllocatorProxyTracker::GetAllocationSize(void* mem)
	{
		Core::AtomicIncAcq(&impl_->totalGetAllocSize_);
		DBG_ASSERT(impl_->allocInfos_.find(mem) != nullptr);
		return impl_->allocator_.GetAllocationSize(mem);
	}

	AllocatorStats AllocatorProxyTracker::GetStats() const
	{
		AllocatorStats retVal;
		retVal.numAllocations_ = impl_->totalAllocated_;
		retVal.peakUsage_ = impl_->peakUsage_;
		retVal.usage_ = impl_->usage_;
		return retVal;
	}

	void AllocatorProxyTracker::LogStats() const
	{
		Core::Log("%s Allocation Tracker:\n", impl_->name_.data());
		Core::Log(" - Proxy Stats:\n");
		Core::Log(" - - Allocate calls: %lld:\n", impl_->totalAllocs_);
		Core::Log(" - - Deallocate calls: %lld:\n", impl_->totalDeallocs_);
		Core::Log(" - - OwnAllocation calls: %lld:\n", impl_->totalOwnAlloc_);
		Core::Log(" - - GetAllocationSize calls: %lld:\n", impl_->totalGetAllocSize_);
		Core::Log(" - - Total Allocated: %lld:\n", impl_->totalAllocated_);
		Core::Log(" - - Usage: %lld:\n", impl_->usage_);
		Core::Log(" - - Peak Usage: %lld:\n", impl_->peakUsage_);
	}

	void AllocatorProxyTracker::LogAllocs() const
	{
		Core::Log("%s Leaks:\n", impl_->name_.data());

		for(auto it : impl_->allocInfos_)
		{
			const AllocInfo& allocInfo = it.value;
			Core::Log(" - Alloc: %p %lld bytes\n", allocInfo.mem_, allocInfo.requestSize_);
			for(i32 i = 0; i < allocInfo.numFrames_; ++i)
			{
				SymbolInfo sym = Core::GetSymbolInfo(allocInfo.callstack_[i]);
				Core::Log(" - - %p - %s\n", allocInfo.callstack_[i], sym.name_);
			}
		}
	}
} // namespace Core
