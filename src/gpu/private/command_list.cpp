#include "gpu/command_list.h"
#include "gpu/manager.h"

#if !CODE_INLINE
#include "gpu/private/command_list.inl"
#endif

namespace GPU
{
	CommandList::CommandList(CommandList&& other)
	    : handleAllocator_(other.handleAllocator_)
	{
		using std::swap;
		swap(queueType_, other.queueType_);
		swap(allocatedBytes_, other.allocatedBytes_);
		swap(commandData_, other.commandData_);
		swap(commands_, other.commands_);
		swap(drawState_, other.drawState_);
		swap(cachedDrawState_, other.cachedDrawState_);
	}

	CommandList::CommandList(i32 bufferSize)
	    : handleAllocator_(GPU::Manager::GetHandleAllocator())
	{
		commandData_.resize(bufferSize);
		cachedDrawState_ = &drawState_;
	}

	CommandList::CommandList(i32 bufferSize, const Core::HandleAllocator& handleAllocator)
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
