#include "core/allocator_tlsf.h"
#include "core/misc.h"

#include "tlsf.h"

#include <cstdio>
#include <cstdlib>
#include <new>

namespace Core
{
	AllocatorTLSF::AllocatorTLSF(IAllocator& parent, i64 minPoolSize)
	    : parent_(parent)
	    , minPoolSize_(minPoolSize)
	{
		DBG_ASSERT(minPoolSize > 0);
		tlsf_ = parent_.Allocate(tlsf_size(), PLATFORM_ALIGNMENT);
		tlsf_ = tlsf_create(tlsf_);
	}

	AllocatorTLSF::~AllocatorTLSF()
	{
		Pool* pool = pool_;
		while(pool)
		{
			Pool* next = pool->next_;
			parent_.Deallocate(pool);
			pool = next;
		}
		parent_.Deallocate(tlsf_);
	}

	void* AllocatorTLSF::Allocate(i64 bytes, i64 align)
	{
		void* retVal = nullptr;
		bool addedPool = false;
		while(retVal == nullptr)
		{
			retVal = tlsf_memalign(tlsf_, align, bytes);
			if(retVal == nullptr)
			{
				DBG_ASSERT(!addedPool);
				if(!AddPool(bytes, align))
					return nullptr;
				addedPool = true;
			}
		}

		return retVal;
	}

	void AllocatorTLSF::Deallocate(void* mem) { tlsf_free(tlsf_, mem); }

	bool AllocatorTLSF::OwnAllocation(void* mem)
	{
		for(auto* pool = pool_; pool; pool = pool->next_)
		{
			if(mem >= pool->mem_ && mem < (pool->mem_ + pool->size_))
			{
				return true;
			}
		}
		return false;
	}

	i64 AllocatorTLSF::GetAllocationSize(void* mem)
	{
		if(OwnAllocation(mem))
		{
			return tlsf_block_size(mem);
		}
		return -1;
	}

	void AllocatorTLSF::LogStats() const
	{
		Core::Log(" - TLSF Heap:\n");
		Core::Log(" - - Integrity: %s\n", tlsf_check(tlsf_) == 0 ? "GOOD" : "BAD");

		const auto Walker = [](void* ptr, size_t size, int used, void* user) {
			Core::Log(" - - - Alloc: %p, %lld bytes, %s\n", ptr, (i64)size, used ? "USED" : "FREE");
		};

		for(auto* pool = pool_; pool; pool = pool->next_)
		{
			Core::Log(" - - Pool: %p, %lld bytes\n", pool->mem_, pool->size_);
			Core::Log(" - - - Integrity: %s\n", tlsf_check_pool(pool->pool_) == 0 ? "GOOD" : "BAD");
			tlsf_walk_pool(pool->pool_, Walker, (void*)this);
		}

		parent_.LogStats();
	}

	bool AllocatorTLSF::CheckIntegrity() { return tlsf_check(tlsf_) == 0; }

	bool AllocatorTLSF::AddPool(i64 minSize, i64 minAlign)
	{
		// @see tlsf.c, mapping_search.
		auto RoundUpBlockSize = [](i64 size) {
			if(size > SMALL_BLOCK_SIZE)
			{
				u64 round = (1 << ((64 - Core::CountLeadingZeros((u64)size)) - SL_INDEX_COUNT_LOG2)) - 1;
				size += round;
			}
			return size;
		};

		i64 overhead = Core::PotRoundUp(sizeof(Pool) + tlsf_pool_overhead(), minAlign) + minAlign;
		i64 reqSize = overhead + RoundUpBlockSize(minSize);
		i64 poolSize = Core::PotRoundUp(reqSize, minPoolSize_);
		i64 usableSize = poolSize - sizeof(Pool);

		minAlign = Core::Max(tlsf_align_size(), minAlign);
		DBG_ASSERT(usableSize > Core::PotRoundUp(minSize, minAlign));
		u8* poolMem = (u8*)parent_.Allocate(poolSize, PLATFORM_ALIGNMENT);
		if(poolMem)
		{
			Pool* pool = new(poolMem) Pool();
			pool->mem_ = (u8*)(pool + 1);
			pool->size_ = usableSize;
			pool->pool_ = tlsf_add_pool(tlsf_, pool->mem_, pool->size_);
			pool->next_ = pool_;
			pool_ = pool;
			return true;
		}
		return false;
	}

} // namespace Core
