#include "core/circular_allocator.h"
#include "core/concurrency.h"
#include "core/misc.h"

namespace Core
{
	CircularAllocator::CircularAllocator(i32 size, i32 alignment)
	    : size_(size)
	    , mask_(size - 1)
	    , alignment_(alignment)
	    , offset_(0)
	    , released_(0)
	{
		DBG_ASSERT(Pot(size));
		base_.resize(size);
	}

	CircularAllocator::~CircularAllocator() {}

	void CircularAllocator::Release(i32 bytes)
	{
		ScopedSpinLock lock(spinLock_);

		Core::AtomicAdd((i64*)&released_, bytes);
	}

	void* CircularAllocator::Allocate(i32 bytes)
	{
		bytes = PotRoundUp(bytes, alignment_);

		ScopedSpinLock lock(spinLock_);

		// Can't allocate more than we have.
		if(bytes > size_)
			return nullptr;

		// Loop until we have a valid offset.
		// NOTE: This could potentially enter into an infinite loop if
		// a large size is being allocated. To mitigate this there is a maximum
		// number of tries imposed to help catch when this occurs.
		i32 maskedOffset = 0;
		i32 numTries = 0;
		do
		{
			maskedOffset = (Core::AtomicAdd((i64*)&offset_, bytes) - bytes) & mask_;
			++numTries;
		} while((maskedOffset + bytes) > size_ || numTries < 3);

		DBG_ASSERT((maskedOffset + bytes) <= size_);
		if((maskedOffset + bytes) > size_)
		{
			return nullptr;
		}

		return &base_[maskedOffset - bytes];
	}
} // namespace Core
