#pragma once

#include "core/debug.h"
#include "core/misc.h"

#include <utility>

namespace GPU
{
	INLINE void* CommandList::Alloc(i32 bytes)
	{
		const i32 requiredMinSize = Core::PotRoundUp(allocatedBytes_ + bytes, sizeof(size_t));
		if(commandData_.size() < requiredMinSize)
			return nullptr; // TODO: Look at adding resizing later.
		void* data = &commandData_[allocatedBytes_];
		allocatedBytes_ += Core::PotRoundUp(bytes, sizeof(size_t));
		return data;
	}

	INLINE CommandDraw* CommandList::Draw(Handle pipelineBinding, Handle drawBinding, PrimitiveTopology primitive,
	    i32 indexOffset, i32 vertexOffset, i32 noofVertices, i32 firstInstance, i32 noofInstances)
	{
		DBG_ASSERT(handleAllocator_.IsValid(pipelineBinding));
		DBG_ASSERT(pipelineBinding.GetType() == ResourceType::PIPELINE_BINDING_SET);
		DBG_ASSERT(handleAllocator_.IsValid(drawBinding));
		DBG_ASSERT(drawBinding.GetType() == ResourceType::DRAW_BINDING_SET);
		DBG_ASSERT(primitive != PrimitiveTopology::INVALID);
		DBG_ASSERT(indexOffset >= 0);
		DBG_ASSERT(vertexOffset >= 0);
		DBG_ASSERT(noofVertices > 0);
		DBG_ASSERT(firstInstance >= 0);
		DBG_ASSERT(noofInstances > 0);

		auto* command = Alloc<CommandDraw>();
		command->pipelineBinding_ = pipelineBinding;
		command->drawBinding_ = drawBinding;
		command->primitive_ = primitive;
		command->indexOffset_ = indexOffset;
		command->vertexOffset_ = vertexOffset;
		command->noofVertices_ = noofVertices;
		command->firstInstance_ = firstInstance;
		command->noofInstances_ = noofInstances;
		commands_.push_back(command);
		return command;
	}

	INLINE CommandDrawIndirect* CommandList::DrawIndirect(
	    Handle pipelineBinding, Handle drawBinding, Handle indirectBuffer, i32 argByteOffset)
	{
		DBG_ASSERT(handleAllocator_.IsValid(pipelineBinding));
		DBG_ASSERT(pipelineBinding.GetType() == ResourceType::PIPELINE_BINDING_SET);
		DBG_ASSERT(handleAllocator_.IsValid(drawBinding));
		DBG_ASSERT(drawBinding.GetType() == ResourceType::DRAW_BINDING_SET);
		DBG_ASSERT(handleAllocator_.IsValid(indirectBuffer));
		DBG_ASSERT(indirectBuffer.GetType() == ResourceType::BUFFER);
		DBG_ASSERT(argByteOffset >= 0);

		auto* command = Alloc<CommandDrawIndirect>();
		command->pipelineBinding_ = pipelineBinding;
		command->drawBinding_ = drawBinding;
		command->indirectBuffer_ = indirectBuffer;
		command->argByteOffset = argByteOffset;
		commands_.push_back(command);
		return command;
	}

	INLINE CommandDispatch* CommandList::Dispatch(Handle pipelineBinding, i32 xGroups, i32 yGroups, i32 zGroups)
	{
		DBG_ASSERT(handleAllocator_.IsValid(pipelineBinding));
		DBG_ASSERT(pipelineBinding.GetType() == ResourceType::PIPELINE_BINDING_SET);
		DBG_ASSERT(xGroups >= 1);
		DBG_ASSERT(yGroups >= 1);
		DBG_ASSERT(zGroups >= 1);

		auto* command = Alloc<CommandDispatch>();
		command->pipelineBinding_ = pipelineBinding;
		command->xGroups_ = xGroups;
		command->yGroups_ = yGroups;
		command->zGroups_ = zGroups;
		commands_.push_back(command);
		return command;
	}

	INLINE CommandDispatchIndirect* CommandList::DispatchIndirect(
	    Handle pipelineBinding, Handle indirectBuffer, i32 argByteOffset)
	{
		DBG_ASSERT(handleAllocator_.IsValid(pipelineBinding));
		DBG_ASSERT(pipelineBinding.GetType() == ResourceType::PIPELINE_BINDING_SET);
		DBG_ASSERT(handleAllocator_.IsValid(indirectBuffer));
		DBG_ASSERT(indirectBuffer.GetType() == ResourceType::BUFFER);
		DBG_ASSERT(argByteOffset >= 0);

		auto* command = Alloc<CommandDispatchIndirect>();
		command->pipelineBinding_ = pipelineBinding;
		command->indirectBuffer_ = indirectBuffer;
		command->argByteOffset = argByteOffset;
		commands_.push_back(command);
		return command;
	}

	INLINE CommandClearRTV* CommandList::ClearRTV(Handle frameBinding, i32 rtvIdx, f32 color[4])
	{
		DBG_ASSERT(handleAllocator_.IsValid(frameBinding));
		DBG_ASSERT(frameBinding.GetType() == ResourceType::FRAME_BINDING_SET);
		DBG_ASSERT(rtvIdx >= 0);

		auto* command = Alloc<CommandClearRTV>();
		command->frameBinding_ = frameBinding;
		command->rtvIdx_ = rtvIdx;
		command->color_[0] = color[0];
		command->color_[1] = color[1];
		command->color_[2] = color[2];
		command->color_[3] = color[3];
		commands_.push_back(command);
		return command;
	}

	INLINE CommandClearDSV* CommandList::ClearDSV(Handle frameBinding, float depth, u8 stencil)
	{
		DBG_ASSERT(handleAllocator_.IsValid(frameBinding));
		DBG_ASSERT(frameBinding.GetType() == ResourceType::FRAME_BINDING_SET);

		auto* command = Alloc<CommandClearDSV>();
		command->frameBinding_ = frameBinding;
		command->depth_ = depth;
		command->stencil_ = stencil;
		commands_.push_back(command);
		return command;
	}

	INLINE CommandClearUAV* CommandList::ClearUAV(Handle pipelineBinding, i32 uavIdx, f32 values[4])
	{
		DBG_ASSERT(handleAllocator_.IsValid(pipelineBinding));
		DBG_ASSERT(pipelineBinding.GetType() == ResourceType::PIPELINE_BINDING_SET);
		DBG_ASSERT(uavIdx >= 0);

		auto* command = Alloc<CommandClearUAV>();
		command->pipelineBinding_ = pipelineBinding;
		command->uavIdx_ = (i16)uavIdx;
		command->f_[0] = values[0];
		command->f_[1] = values[1];
		command->f_[2] = values[2];
		command->f_[3] = values[3];
		commands_.push_back(command);
		return command;
	}

	INLINE CommandClearUAV* CommandList::ClearUAV(Handle pipelineBinding, i32 uavIdx, u32 values[4])
	{
		DBG_ASSERT(handleAllocator_.IsValid(pipelineBinding));
		DBG_ASSERT(pipelineBinding.GetType() == ResourceType::PIPELINE_BINDING_SET);
		DBG_ASSERT(uavIdx >= 0);

		auto* command = Alloc<CommandClearUAV>();
		command->pipelineBinding_ = pipelineBinding;
		command->uavIdx_ = (i16)uavIdx;
		command->u_[0] = values[0];
		command->u_[1] = values[1];
		command->u_[2] = values[2];
		command->u_[3] = values[3];
		commands_.push_back(command);
		return command;
	}

	INLINE CommandUpdateBuffer* CommandList::UpdateBuffer(Handle buffer, i32 offset, i32 size, const void* data)
	{
		DBG_ASSERT(handleAllocator_.IsValid(buffer));
		DBG_ASSERT(buffer.GetType() == ResourceType::BUFFER);
		DBG_ASSERT(offset >= 0);
		DBG_ASSERT(size > 0);
		DBG_ASSERT(data != nullptr);

		auto* command = Alloc<CommandUpdateBuffer>();
		command->buffer_ = buffer;
		command->offset_ = offset;
		command->size_ = size;
		command->data_ = data;
		commands_.push_back(command);
		return command;
	}

	INLINE CommandUpdateTextureSubresource* CommandList::UpdateTextureSubresource(
	    Handle texture, i32 subResourceIdx, const TextureSubResourceData& data)
	{
		DBG_ASSERT(handleAllocator_.IsValid(texture));
		DBG_ASSERT(texture.GetType() == ResourceType::TEXTURE);
		DBG_ASSERT(subResourceIdx >= 0);
		DBG_ASSERT(data.data_ != nullptr);
		DBG_ASSERT(data.rowPitch_ > 0);
		DBG_ASSERT(data.slicePitch_ > 0);

		auto* command = Alloc<CommandUpdateTextureSubresource>();
		command->texture_ = texture;
		command->subResourceIdx_ = (i16)subResourceIdx;
		command->data_ = data;
		commands_.push_back(command);
		return command;
	}

	INLINE CommandCopyBuffer* CommandList::CopyBuffer(
	    Handle srcBuffer, i32 srcOffset, i32 srcSize, Handle dstBuffer, i32 dstOffset)
	{
		DBG_ASSERT(handleAllocator_.IsValid(srcBuffer));
		DBG_ASSERT(srcBuffer.GetType() == ResourceType::BUFFER);
		DBG_ASSERT(srcOffset >= 0);
		DBG_ASSERT(srcSize >= 0);
		DBG_ASSERT(dstBuffer.GetType() == ResourceType::BUFFER);
		DBG_ASSERT(dstOffset >= 0);
		DBG_ASSERT(srcBuffer != dstBuffer);

		auto* command = Alloc<CommandCopyBuffer>();
		command->srcBuffer_ = srcBuffer;
		command->srcOffset_ = srcOffset;
		command->srcSize_ = srcSize;
		command->dstBuffer_ = dstBuffer;
		command->dstOffset_ = dstOffset;
		commands_.push_back(command);
		return command;
	}

	INLINE CommandCopyTextureSubResource* CommandList::CopyTextureSubResource(Handle srcTexture, i32 srcSubResourceIdx,
	    const Box& srcBox, Handle dstTexture, i32 dstSubResourceIdx, const Point& dstPoint)
	{
		DBG_ASSERT(handleAllocator_.IsValid(srcTexture));
		DBG_ASSERT(srcTexture.GetType() == ResourceType::TEXTURE);
		DBG_ASSERT(srcSubResourceIdx >= 0);
		DBG_ASSERT(srcBox.x_ >= 0);
		DBG_ASSERT(srcBox.y_ >= 0);
		DBG_ASSERT(srcBox.z_ >= 0);
		DBG_ASSERT(srcBox.w_ > 0);
		DBG_ASSERT(srcBox.h_ > 0);
		DBG_ASSERT(srcBox.d_ > 0);
		DBG_ASSERT(dstTexture.GetType() == ResourceType::TEXTURE);
		DBG_ASSERT(dstSubResourceIdx >= 0);
		DBG_ASSERT(dstPoint.x_ >= 0);
		DBG_ASSERT(dstPoint.y_ >= 0);
		DBG_ASSERT(srcTexture != dstTexture || srcSubResourceIdx != dstSubResourceIdx);


		auto* command = Alloc<CommandCopyTextureSubResource>();
		command->srcTexture_ = srcTexture;
		command->srcSubResourceIdx_ = (i16)srcSubResourceIdx;
		command->srcBox_ = srcBox;
		command->dstTexture_ = dstTexture;
		command->dstSubResourceIdx_ = (i16)dstSubResourceIdx;
		command->dstPoint_ = dstPoint;
		commands_.push_back(command);
		return command;
	}

} // namespace GPU
