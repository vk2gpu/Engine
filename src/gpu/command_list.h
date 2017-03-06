#pragma once

#include "gpu/dll.h"
#include "gpu/commands.h"
#include "core/vector.h"

namespace GPU
{
	/**
	 * Software side command list.
	 * These should be built and compiled by jobs prior to submission
	 * to a GPU queue.
	 * These should be built from a single thread.
	 */
	class CommandList
	{
	public:
		/**
		 * @param handleAllocator Used to validate handles passed in.
		 * @param bufferSize Buffer size to allocate.
		 */
		GPU_DLL CommandList(Core::HandleAllocator& handleAllocator, i32 bufferSize = 1024 * 1024);

		/**
		 * Allocate from command list.
		 * @param bytes Number of bytes to allocate.
		 * @return Allocated memory. Only valid until submission.
		 * @pre @a bytes > 0.
		 */
		GPU_DLL void* Alloc(i32 bytes);

		/**
		 * Templated alloc. See above.
		 */
		template<typename TYPE>
		TYPE* Alloc(i32 num = 1)
		{
			return new(Alloc(sizeof(TYPE) * num)) TYPE[num]();
		}

		/**
		 * Reset command list.
		 */
		GPU_DLL void Reset();

		/**
		 * See @a CommandDraw.
		 * @pre @a pipelineBinding is valid.
		 * @pre @a drawBinding is valid.
		 * @pre @a primitive is valid.
		 * @pre @a indexOffset >= 0.
		 * @pre @a vertexOffset >= 0.
		 * @pre @a noofVertices > 0.
		 * @pre @a firstInstance >= 0.
		 * @pre @a noofInstances > 0.
		 * @return Draw command. nullptr if failure.
		 */
		GPU_DLL CommandDraw* Draw(Handle pipelineBinding, Handle drawBinding, PrimitiveTopology primitive,
		    i32 indexOffset, i32 vertexOffset, i32 noofVertices, i32 firstInstance, i32 noofInstances);

		/**
		 * See @a CommandDrawIndirect.
		 * @pre @a pipelineBinding is valid.
		 * @pre @a drawBinding is valid.
		 * @pre @a indirectBuffer is valid.
		 * @pre @a argByteOffset >= 0.
		 * @return Dispatch command. nullptr if failure.
		 */
		GPU_DLL CommandDrawIndirect* DrawIndirect(
		    Handle pipelineBinding, Handle drawBinding, Handle indirectBuffer, i32 argByteOffset);

		/**
		 * See @a CommandDispatch.
		 * @pre @a pipelineBinding is valid.
		 * @pre @a xGroups > 0.
		 * @pre @a yGroups > 0.
		 * @pre @a zGroups > 0.
		 * @return Dispatch command. nullptr if failure.
		 */
		GPU_DLL CommandDispatch* Dispatch(Handle pipelineBinding, i32 xGroups, i32 yGroups, i32 zGroups);

		/**
		 * See @a CommandDispatchIndirect.
		 * @pre @a pipelineBinding is valid.
		 * @pre @a indirectBuffer is valid.
		 * @pre @a argByteOffset >= 0.
		 * @return Dispatch command. nullptr if failure.
		 */
		GPU_DLL CommandDispatchIndirect* DispatchIndirect(
		    Handle pipelineBinding, Handle indirectBuffer, i32 argByteOffset);

		/**
		 * See @a CommandClearRTV.
		 * @pre @a frameBinding is valid.
		 * @pre @a rtvIdx >= 0.
		 * @return Clear command. nullptr if failure.
		 */
		GPU_DLL CommandClearRTV* ClearRTV(Handle frameBinding, i32 rtvIdx, f32 color_[4]);

		/**
		 * See @a CommandClearDSV.
		 * @pre @a frameBinding is valid.
		 * @return Clear command. nullptr if failure.
		 */
		GPU_DLL CommandClearDSV* ClearDSV(Handle frameBinding, float depth, u8 stencil);

		/**
		 * See @a CommandClearUAV.
		 * @pre @a pipelineBinding is valid.
		 * @pre @a uavIdx >= 0.
		 */
		GPU_DLL CommandClearUAV* ClearUAV(Handle pipelineBinding, i32 uavIdx, f32 values[4]);

		/**
		 * See @a CommandClearUAV.
		 * @pre @a pipelineBinding is valid.
		 * @pre @a uavIdx >= 0.
		 * @return Clear command. nullptr if failure.
		 */
		GPU_DLL CommandClearUAV* ClearUAV(Handle pipelineBinding, i32 uavIdx, u32 values[4]);

		/**
		 * See @a CommandUpdateBuffer.
		 * @pre @a buffer is valid.
		 * @pre @a offset >= 0.
		 * @pre @a size > 0.
		 * @pre @a data != nullptr.
		 * @return Update command. nullptr if failure.
		 */
		GPU_DLL CommandUpdateBuffer* UpdateBuffer(Handle buffer, i32 offset, i32 size, const void* data);

		/**
		 * See @a CommandUpdateTextureSubresource.
		 * @pre @a texture is valid.
		 * @pre @a subResourceIdx >= 0.
		 * @pre @a data is valid.
		 * @return Update command. nullptr if failure.
		 */
		GPU_DLL CommandUpdateTextureSubresource* UpdateTextureSubresource(
		    Handle texture, i32 subResourceIdx, const TextureSubResourceData& data);

		/**
		 * See @a CommandCopyBuffer.
		 * @pre @a srcBuffer is valid.
		 * @pre @a srcOffset >= 0.
		 * @pre @a srcSize > 0.
		 * @pre @a dstBuffer is valid.
		 * @pre @a dstOffset >= 0.
		 * @pre @a srcBuffer != @a dstBuffer.
		 * @return Copy command. nullptr if failure.
		 */
		GPU_DLL CommandCopyBuffer* CopyBuffer(
		    Handle srcBuffer, i32 srcOffset, i32 srcSize, Handle dstBuffer, i32 dstOffset);

		/**
		 * See @a CommandCopyTextureSubResource.
		 * @pre @a srcTexture is valid.
		 * @pre @a srcSubResourceIdx >= 0.
		 * @pre @a srcBox is within @a srcTexture sub resource bounds.
		 * @pre @a dstTexture is valid.
		 * @pre @a dstSubResourceIdx >= 0.
		 * @pre @a dstPoint is within @a dstTexture sub resource bounds.
		 * @pre @a srcTexture != @a dstTexture or @a srcSubResourceIdx != @a dstSubResourceIdx.
		 * @return Copy command. nullptr if failure.
		 */
		GPU_DLL CommandCopyTextureSubResource* CopyTextureSubResource(Handle srcTexture, i32 srcSubResourceIdx,
		    const Box& srcBox, Handle dstTexture, i32 dstSubResourceIdx, const Point& dstPoint);

		/**
		 * See @a CommandUpdateRTV.
		 * @pre @a frameBinding is valid.
		 * @pre @a firstRTV >= 0.
		 * @pre @a numRTVs > 0.
		 * @return Update command to be filled in with valid state information. nullptr if failure.
		 */
		GPU_DLL CommandUpdateRTV* UpdateRTV(Handle frameBinding, i32 firstRTV, i32 numRTVs);

		/**
		 * See @a CommandUpdateDSV.
		 * @pre @a frameBinding is valid.
		 * @pre @a dsv is valid.
		 * @return Update command. nullptr if failure.
		 */
		GPU_DLL CommandUpdateDSV* UpdateDSV(Handle frameBinding, const BindingDSV& dsv);

		/**
		 * See @a CommandUpdateSRV.
		 * @pre @a pipelineBinding is valid.
		 * @pre @a firstSRV >= 0.
		 * @pre @a numSRVs > 0.
		 * @return Update command to be filled in with valid state information. nullptr if failure.
		 */
		GPU_DLL CommandUpdateSRV* UpdateSRV(Handle pipelineBinding, i32 firstSRV, i32 numSRVs);

		/**
		 * See @a CommandUpdateUAV.
		 * @pre @a pipelineBinding is valid.
		 * @pre @a firstUAV >= 0.
		 * @pre @a numUAVs > 0.
		 * @return Update command to be filled in with valid state information. nullptr if failure.
		 */
		GPU_DLL CommandUpdateUAV* UpdateUAV(Handle pipelineBinding, i32 firstUAV, i32 numUAVs);

		/**
		 * See @a CommandUpdateCBV.
		 * @pre @a pipelineBinding is valid.
		 * @pre @a firstCBV >= 0.
		 * @pre @a numCBVs > 0.
		 * @return Update command to be filled in with valid state information. nullptr if failure.
		 */
		GPU_DLL CommandUpdateCBV* UpdateCBV(Handle pipelineBinding, i32 firstCBV, i32 numCBVs);

		/// for iterator support.
		typedef Core::Vector<Command*> CommandVector;
		typedef CommandVector::iterator iterator;
		typedef CommandVector::const_iterator const_iterator;

		iterator begin() { return commands_.begin(); }
		const_iterator begin() const { return commands_.begin(); }
		iterator end() { return commands_.end(); }
		const_iterator end() const { return commands_.end(); }

	private:
		CommandList(const CommandList&) = delete;

		/// Used to validate handles.
		Core::HandleAllocator& handleAllocator_;

		i32 allocatedBytes_ = 0;
		Core::Vector<u8> commandData_;
		CommandVector commands_;
	};
} // namespace GPU
#if CODE_INLINE
#include "gpu/private/command_list.inl"
#endif
