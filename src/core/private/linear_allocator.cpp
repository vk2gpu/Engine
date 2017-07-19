#include "core/linear_allocator.h"
#include "core/concurrency.h"
#include "core/misc.h"

namespace Core
{
	LinearAllocator::LinearAllocator(i32 size, i32 alignment)
	    : size_(size)
	    , alignment_(alignment)
	    , offset_(0)
	{
		base_.resize(size);
	}

	LinearAllocator::~LinearAllocator() {}

	void LinearAllocator::Reset() { offset_ = 0; }

	void* LinearAllocator::Allocate(i32 bytes)
	{
		bytes = PotRoundUp(bytes, alignment_);
		const i32 nextOffset = Core::AtomicAdd(&offset_, bytes);
		if(nextOffset <= size_)
		{
			return &base_[nextOffset - bytes];
		}
		return nullptr;
	}
} // namespace Core
