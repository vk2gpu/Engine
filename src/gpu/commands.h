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

	/**
	 * Base command.
	 */
	struct GPU_DLL Command
	{
		CommandType type_ = CommandType::INVALID;
	};

	/**
	 * Templated command to autoset command type.
	 */
	template<CommandType TYPE>
	struct GPU_DLL CommandTyped : Command
	{
		CommandTyped() { type_ = TYPE; }
	};

	/**
	 * Draw.
	 * This handles indexed & non-indexed draw depending on the drawBinding_.
	 */
	struct GPU_DLL CommandDraw : CommandTyped<CommandType::DRAW>
	{
		/// Pipeline state binding to use.
		Handle pipelineBinding_;
		/// Draw binding to use. Determines if indexed or non-indexed draw.
		Handle drawBinding_;
		/// Primitive type to rasterize.
		PrimitiveTopology primitive_ = PrimitiveTopology::INVALID;
		/// Index offset. Ignored for non-instanced draw.
		i32 indexOffset_ = 0;
		/// Vertex offset to start at.
		i32 vertexOffset_ = 0;
		/// Number of vertices to draw.
		i32 noofVertices_ = 0;
		/// First instance ID to draw from.
		i32 firstInstance_ = 0;
		/// Number of instances to draw.
		i32 noofInstances_ = 0;
	};

	/**
	 * Draw indirect.
	 * This handles indexed & non-indexed draw depending on the drawBinding_.
	 */
	struct GPU_DLL CommandDrawIndirect : CommandTyped<CommandType::DRAW_INDIRECT>
	{
		/// Pipeline state binding to use.
		Handle pipelineBinding_;
		/// Draw binding to use. Determines if indexed or non-indexed draw.
		Handle drawBinding_;
		/// Indirect buffer with draw parameters.
		Handle indirectBuffer_;
		/// Byte offset in indirect buffer to start reading arguments from.
		i32 argByteOffset = 0;
	};

	/**
	 * Dispatch.
	 */
	struct GPU_DLL CommandDispatch : CommandTyped<CommandType::DISPATCH>
	{
		/// Pipeline state binding to use.
		Handle pipelineBinding_;
		/// X groups to dispatch.
		i32 xGroups_ = 0;
		/// Y groups to dispatch.
		i32 yGroups_ = 0;
		/// Z groups to dispatch.
		i32 zGroups_ = 0;
	};

	/**
	 * Dispatch indirect.
	 */
	struct GPU_DLL CommandDispatchIndirect : CommandTyped<CommandType::DISPATCH_INDIRECT>
	{
		/// Pipeline state binding to use.
		Handle pipelineBinding_;
		/// Indirect buffer with dispatch parameters.
		Handle indirectBuffer_;
		/// Byte offset in indirect buffer to start reading arguments from.
		i32 argByteOffset = 0;
	};

	/**
	 * Clear a render target view.
	 */
	struct GPU_DLL CommandClearRTV : CommandTyped<CommandType::CLEAR_RTV>
	{
		/// Frame binding which contains the RTV we wish to clear.
		Handle frameBinding_;
		/// RTV index in frame binding.
		i32 rtvIdx_ = 0;
		/// Colour to clear to.
		f32 color_[4];
	};

	/**
	 * Clear a depth stencil view.
	 */
	struct GPU_DLL CommandClearDSV : CommandTyped<CommandType::CLEAR_DSV>
	{
		/// Frame binding which contains the DSV we wish to clear.
		Handle frameBinding_;
		/// Depth value to clear to.
		f32 depth_;
		/// Stencil value to clear to.
		u8 stencil_;
	};

	/**
	 * Clear an unordered access view.
	 */
	struct GPU_DLL CommandClearUAV : CommandTyped<CommandType::CLEAR_UAV>
	{
		/// Pipeline binding which contains the UAV we wish to clear.
		Handle pipelineBinding_;
		/// Index of UAV.
		i16 uavIdx_ = 0;
		/// Clear data.
		union
		{
			f32 f_[4];
			u32 u_[4];
		};
	};

	/**
	 * Update buffer data.
	 */
	struct GPU_DLL CommandUpdateBuffer : CommandTyped<CommandType::UPDATE_BUFFER>
	{
		/// Buffer to update.
		Handle buffer_;
		/// Offset within buffer to update.
		i32 offset_ = 0;
		/// Size of update.
		i32 size_ = 0;
		/// Pointer to data.
		const void* data_ = nullptr;
	};

	/**
	 * Update texture subresource.
	 */
	struct GPU_DLL CommandUpdateTextureSubresource : CommandTyped<CommandType::UPDATE_TEXTURE_SUBRESOURCE>
	{
		/// Texture to update.
		Handle texture_;
		/// Subresource index.
		i16 subResourceIdx_ = 0;
		/// Subresource data.
		TextureSubResourceData data_;
	};

	/**
	 * Copy are of buffer from one to another.
	 */
	struct GPU_DLL CommandCopyBuffer : CommandTyped<CommandType::COPY_BUFFER>
	{
		/// Source buffer.
		Handle srcBuffer_;
		/// Offset in source buffer to copy from.
		i32 srcOffset_ = 0;
		/// Size of copy.
		i32 srcSize_ = 0;
		/// Destination buffer.
		Handle dstBuffer_;
		/// Destination offset to copy into.
		i32 dstOffset_ = 0;
	};

	/**
	 * Copy texture subresource from one to another.
	 */
	struct GPU_DLL CommandCopyTextureSubResource : CommandTyped<CommandType::COPY_TEXTURE_SUBRESOURCE>
	{
		/// Source texture.
		Handle srcTexture_;
		/// Source subresource index.
		i16 srcSubResourceIdx_ = 0;
		/// Source box.
		Box srcBox_;
		/// Destination texture.
		Handle dstTexture_;
		/// Destination subresource index.
		i16 dstSubResourceIdx_ = 0;
		/// Destination point.
		Point dstPoint_;
	};

	/**
	 * Update RTVs within a frame binding.
	 */
	struct GPU_DLL CommandUpdateRTV : CommandTyped<CommandType::UPDATE_RTV>
	{
		/// Frame binding to update.
		Handle frameBinding_;
		/// First RTV to update.
		i16 firstRTV_ = 0;
		/// Number of RTVs to update.
		i16 numRTVs_ = 0;
		/// Binding data for RTVs.
		BindingRTV* rtvs_ = nullptr;
	};

	/**
	 * Update DSV within a frame binding.
	 */
	struct GPU_DLL CommandUpdateDSV : CommandTyped<CommandType::UPDATE_DSV>
	{
		/// Frame binding to update.
		Handle frameBinding_;
		/// Binding data for DSV.
		BindingDSV dsv_;
	};

	/**
	 * Update SRVs within pipeline binding.
	 */
	struct GPU_DLL CommandUpdateSRV : CommandTyped<CommandType::UPDATE_SRV>
	{
		/// Pipeline binding to update.
		Handle pipelineBinding_;
		/// First SRV to update.
		i16 firstSRV_ = 0;
		/// Number of SRVs to update.
		i16 numSRVs_ = 0;
		/// Binding data for SRVs.
		BindingSRV* srvs_ = nullptr;
	};

	/**
	 * Update UAVs within pipeline binding.
	 */
	struct GPU_DLL CommandUpdateUAV : CommandTyped<CommandType::UPDATE_UAV>
	{
		/// Pipeline binding to update.
		Handle pipelineBinding_;
		/// First UAV to update.
		i16 firstUAV_ = 0;
		/// Number of UAVs to update.
		i16 numUAVs_ = 0;
		/// Binding data for UAVs.
		BindingUAV* uavs_ = nullptr;
	};

	/**
	 * Update CBVs within pipeline binding.
	 */
	struct GPU_DLL CommandUpdateCBV : CommandTyped<CommandType::UPDATE_CBV>
	{
		/// Pipeline binding to update.
		Handle pipelineBinding_;
		/// First CBV to update.
		i16 firstCBV_ = 0;
		/// Number of CBVs to update.
		i16 numCBVs_ = 0;
		/// Binding data for CBVs.
		BindingBuffer* cbvs_ = nullptr;
	};


} // namespace GPU
