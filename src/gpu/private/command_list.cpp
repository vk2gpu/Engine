#include "gpu/command_list.h"

#if !CODE_INLINE
#include "gpu/private/command_list.inl"
#endif

namespace GPU
{
	CommandList::CommandList(const Core::HandleAllocator& handleAllocator, i32 bufferSize)
	    : handleAllocator_(handleAllocator)
	{
		commandData_.resize(bufferSize);
		cachedDrawState_ = &drawState_;
	}

	void CommandList::Reset()
	{
		queueType_ = CommandQueueType::NONE;
		allocatedBytes_ = 0;
		commands_.clear();
		cachedDrawState_ = &drawState_;
	}

} // namespace GPU
