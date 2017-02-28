#pragma once

#include "gpu/dll.h"
#include "gpu/types.h"
#include "gpu/resources.h"

namespace GPU
{
	enum class CommandType : i16
	{
		INVALID = -1,

		// Draws.
		DRAW,
		DRAW_INDIRECT,

		// Dispatches.
		DISPATCH,
		DISPATCH_INDIRECT,

		// Clears.
		CLEAR_RTV,
		CLEAR_DSV,
		CLEAR_UAV,

		// Updates.
		UPDATE_BUFFER,
		UPDATE_TEXTURE_SUBRESOURCE,

		// Transfers.
		COPY_BUFFER,
		COPY_TEXTURE_SUBRESOURCE,

		// Binding updates.
		UPDATE_RTV,
		UPDATE_DSV,
		UPDATE_SRV,
		UPDATE_UAV,
		UPDATE_CBV,
	};

	struct Command
	{
		CommandType type_ = CommandType::INVALID;
	};

	struct CommandDraw : Command
	{
		Handle pipelineBinding_;
		Handle drawBinding_;
		PrimitiveTopology primitive_ = PrimitiveTopology::INVALID;
		i32 indexOffset_ = 0;
		i32 noofIndices_ = 0;
		i32 vertexOffset_ = 0;
		i32 firstInstance_ = 0;
		i32 noofInstances_ = 0;
	};

	struct CommandDrawIndirect : Command
	{
		Handle pipelineBinding_;
		Handle drawBinding_;
		Handle indirectBuffer_;
		i32 argByteOffset = 0;
	};

	struct CommandDispatch : Command
	{
		Handle pipelineBinding_;
		i32 xGroups_ = 0;
		i32 yGroups_ = 0;
		i32 zGroups_ = 0;
	};

	struct CommandDispatchIndirect : Command
	{
		Handle pipelineBinding_;
		Handle indirectBuffer_;
		i32 argByteOffset = 0;
	};

	struct CommandClearRTV : Command
	{
		Handle frameBinding_;
		i32 rtvIdx_ = 0;
		f32 color_[4];
	};

	struct CommandClearDSV : Command
	{
		Handle frameBinding_;
		i32 dsvIdx_ = 0;
		f32 depth_;
		u8 stencil_;
	};

	struct CommandClearUAV : Command
	{
		Handle pipelineBinding_;
		i32 uavIdx_ = 0;
		union
		{
			f32 f_[4];
			u32 u_[4];
		};
	};

	struct CommandUpdateBuffer : Command
	{
		Handle buffer_;
		i32 offset_ = 0;
		i32 size_ = 0;
		const void* data_ = nullptr;
	};

	struct CommandUpdateTextureSubresource : Command
	{
		Handle texture_;
		i32 subResourceIdx_ = 0;
		const TextureSubResourceData data_;
	};

	struct CommandCopyBuffer : Command
	{
		Handle srcBuffer_;
		i32 srcOffset_ = 0;
		i32 srcSize_ = 0;
		Handle dstBuffer_;
		i32 dstOffset_ = 0;
	};

	struct CommandCopyTextureSubResource : Command
	{
		Handle srcTexture_;
		i32 srcSubResourceIdx_ = 0;
		Box srcBox_;
		Handle dstTexture_;
		i32 dstSubResourceIdx_ = 0;
		Point dstPoint_;
	};

	struct CommandUpdateRTV : Command
	{
		Handle frameBinding_;
		i16 numRTVs_ = 0;
		i16 firstRTV_ = 0;
		BindingRTV* rtvs_ = nullptr;
	};

	struct CommandUpdateDSV : Command
	{
		Handle frameBinding_;
		i16 numDSVs_ = 0;
		i16 firstDSV_ = 0;
		BindingDSV* dsvs_ = nullptr;
	};

	struct CommandUpdateSRV : Command
	{
		Handle pipelineBinding_;
		i16 numSRVs_ = 0;
		i16 firstSRV_ = 0;
		BindingSRV* srvs_ = nullptr;
	};

	struct CommandUpdateUAV : Command
	{
		Handle pipelineBinding_;
		i16 numUAVs_ = 0;
		i16 firstUAV_ = 0;
		BindingUAV* uavs_ = nullptr;
	};

	struct CommandUpdateCBV : Command
	{
		Handle pipelineBinding_;
		i16 numRTVs_ = 0;
		i16 firstRTV_ = 0;
		BindingBuffer* cbvs_ = nullptr;
	};


} // namespace GPU
