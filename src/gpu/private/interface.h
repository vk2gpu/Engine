#pragma once

#include "gpu/dll.h"
#include "gpu/types.h"
#include "gpu/resources.h"
#include "core/types.h"
#include "core/handle.h"

namespace GPU
{
	class Interface
	{
	public:
		virtual ~Interface() {}

		/**
		 * Device operations.
		 */
		virtual i32 EnumerateAdapters(AdapterInfo* outAdapters, i32 maxAdapters) = 0;

		/**
		 * Resource creation/destruction.
		 */
		virtual ErrorCode CreateSwapChain(Handle handle, const SwapChainDesc& desc, const char* debugName) = 0;
		virtual ErrorCode CreateBuffer(
		    Handle handle, const BufferDesc& desc, const void* initialData, const char* debugName) = 0;
		virtual ErrorCode CreateTexture(Handle handle, const TextureDesc& desc,
		    const TextureSubResourceData* initialData, const char* debugName) = 0;
		virtual ErrorCode CreateSamplerState(Handle handle, const SamplerState& state, const char* debugName) = 0;
		virtual ErrorCode CreateShader(Handle handle, const ShaderDesc& desc, const char* debugName) = 0;
		virtual ErrorCode CreateGraphicsPipelineState(
		    Handle handle, const GraphicsPipelineStateDesc& desc, const char* debugName) = 0;
		virtual ErrorCode CreateComputePipelineState(
		    Handle handle, const ComputePipelineStateDesc& desc, const char* debugName) = 0;
		virtual ErrorCode CreatePipelineBindingSet(
		    Handle handle, const PipelineBindingSetDesc& desc, const char* debugName) = 0;
		virtual ErrorCode CreateDrawBindingSet(
		    Handle handle, const DrawBindingSetDesc& desc, const char* debugName) = 0;
		virtual ErrorCode CreateCommandList(Handle handle, const char* debugName) = 0;
		virtual ErrorCode CreateFence(Handle handle, const char* debugName) = 0;
		virtual ErrorCode DestroyResource(Handle handle) = 0;
	};

} // namespace GPU
