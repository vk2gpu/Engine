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
	}

	void CommandList::Reset()
	{
		queueType_ = CommandQueueType::NONE;
		allocatedBytes_ = 0;
		commands_.clear();
	}

} // namespace GPU
