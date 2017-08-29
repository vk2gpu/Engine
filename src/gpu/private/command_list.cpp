#include "gpu/command_list.h"
#include "gpu/manager.h"

#if !CODE_INLINE
#include "gpu/private/command_list.inl"
#endif

#include <cstdarg>
#include <cstdio>

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
		DBG_ASSERT(eventLabelDepth_ == 0);
		queueType_ = CommandQueueType::NONE;
		allocatedBytes_ = 0;
		commands_.clear();
		cachedDrawState_ = &drawState_;
	}

	CommandList::ScopedEvent CommandList::Event(i32 metaData, const char* text)
	{
		const i32 textLength = (i32)strlen(text) + 1;
		char* dstText = Alloc<char>(textLength);
		strcpy_s(dstText, textLength, text);
		if(InternalBeginEvent(metaData, dstText))
			return ScopedEvent(this);
		return ScopedEvent(nullptr);
	}

	CommandList::ScopedEvent CommandList::Eventf(i32 metaData, const char* format, ...)
	{
		static const i32 MAX_TEXT_LENGTH = 256;
		char* dstText = Alloc<char>(MAX_TEXT_LENGTH);
		dstText[0] = '\0';
		va_list argList;
		va_start(argList, format);
		vsprintf_s(dstText, MAX_TEXT_LENGTH, format, argList);
		va_end(argList);
		if(InternalBeginEvent(metaData, dstText))
			return ScopedEvent(this);
		return ScopedEvent(nullptr);
	}

	CommandEndEvent* CommandList::InternalEndEvent()
	{
		DBG_ASSERT(--eventLabelDepth_ >= 0);
		queueType_ |= CommandEndEvent::QUEUE_TYPE;
		auto* command = Alloc<CommandEndEvent>();
		commands_.push_back(command);
		return command;
	}

} // namespace GPU
