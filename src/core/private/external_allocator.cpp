#include "core/external_allocator.h"

#if !CODE_INLINE
#include "core/private/external_allocator.inl"
#endif

#include "core/debug.h"
#include "etlsf.h"

namespace Core
{
	ExternalAllocator::ExternalAllocator(i32 size, i32 maxAllocations)
	{
		DBG_ASSERT(size > 0);
		DBG_ASSERT(maxAllocations > 0 && maxAllocations <= 0xffff)
		arena_ = etlsf_create((u32)size, (u16)maxAllocations);
	}

	ExternalAllocator::~ExternalAllocator() { etlsf_destroy(arena_); }

	u16 ExternalAllocator::AllocRange(i32 size)
	{
		DBG_ASSERT(size > 0);
		return etlsf_alloc_range(arena_, size).value;
	}

	void ExternalAllocator::FreeRange(u16 id)
	{
		const etlsf_alloc_t alloc = {id};
		etlsf_free_range(arena_, alloc);
	}

	ExternalAlloc ExternalAllocator::GetAlloc(u16 id) const
	{
		const etlsf_alloc_t alloc = {id};
		ExternalAlloc retVal;
		retVal.offset_ = (i32)etlsf_alloc_offset(arena_, alloc);
		retVal.size_ = (i32)etlsf_alloc_size(arena_, alloc);
		if(retVal.offset_ == 0 && retVal.size_ == 0)
		{
			retVal.offset_ = -1;
			retVal.size_ = -1;
		}
		return retVal;
	}
}
