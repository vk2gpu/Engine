#pragma once

#include "core/types.h"
#include "gpu/dll.h"
#include "gpu/command_list.h"
#include "gpu/resources.h"

namespace GPU
{
	class GPU_DLL Manager final
	{
	public:
		/**
		 * Create GPU manager
		 * @param deviceWindow Window we use for device creation (if required).
		 * @param debuggerIntegration Which debugger(s) to enable integration of.
		 */
		Manager(void* deviceWindow, DebuggerIntegrationFlags debuggerIntegration = DebuggerIntegrationFlags::NONE);
		~Manager();

		/**
		 * Enumerate adapters.
		 * @param outAdapters Pointer to adapter info structure array.
		 * @param maxAdapters Maximum number to enumerate.
		 * @return Total number of adapters.
		 */
		i32 EnumerateAdapters(AdapterInfo* outAdapters, i32 maxAdapters);

		/**
		 * Is initialized?
		 */
		bool IsInitialized() const;

		/**
		 * Initialize.
		 * @param adapterIdx Adapter index.
		 * @pre !IsInitialized()
		 */
		ErrorCode Initialize(i32 adapterIdx);

		/**
		 * Create swapchain.
		 * @param desc Swapchain descriptor.
		 * @param debugName Debug name.
		 */
		Handle CreateSwapChain(const SwapChainDesc& desc, const char* debugName);

		/**
		 * Create buffer.
		 * @param desc Buffer descriptor.
		 * @param initialData Data to create buffer with.
		 * @param debugName Debug name.
		 * @pre initialData == nullptr, or allocated size matches one in @a desc
		 */
		Handle CreateBuffer(const BufferDesc& desc, const void* initialData, const char* debugName);

		/**
		 * Create texture.
		 * @param desc Texture descriptor.
		 * @param initialData Data to create texture with.
		 * @param debugName Debug name.
		 * @pre initialData == nullptr, or has enough for (levels * elements).
		 */
		Handle CreateTexture(const TextureDesc& desc, const TextureSubResourceData* initialData, const char* debugName);

		/**
		 * Create sample state.
		 * @param samplerState Sampler state to create.
		 */
		Handle CreateSamplerState(const SamplerState& state, const char* debugName);

		/**
		 * Create shader.
		 * @param desc Shader desc, contains byte code and type.
		 * @param debugName Debug name.
		 */
		Handle CreateShader(const ShaderDesc& desc, const char* debugName);

		/**
		 * Create graphics pipeline state.
		 */
		Handle CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc, const char* debugName);

		/**
		 * Create compute pipeline state.
		 */
		Handle CreateComputePipelineState(const ComputePipelineStateDesc& desc, const char* debugName);

		/**
		 * Create pipeline binding set.
		 */
		Handle CreatePipelineBindingSet(const PipelineBindingSetDesc& desc, const char* debugName);

		/**
		 * Create draw binding set.
		 */
		Handle CreateDrawBindingSet(const DrawBindingSetDesc& desc, const char* debugName);

		/**
		 * Create frame binding set.
		 */
		Handle CreateFrameBindingSet(const FrameBindingSetDesc& desc, const char* debugName);

		/**
		 * Create command list.
		 */
		Handle CreateCommandList(const char* debugName);

		/**
		 * Create fence.
		 * Used for synchronisation in and around queues.
		 */
		Handle CreateFence(const char* debugName);

		/**
		 * Destroy resource.
		 */
		void DestroyResource(Handle handle);

		/**
		 * Compile command list.
		 * @param handle Handle to command list.
		 * @param commandList Input software command list.
		 * @return Success.
		 */
		bool CompileCommandList(Handle handle, const CommandList& commandList);

		/**
		 * Submit command list.
		 * @param handle Handle to command list.
		 * @return Success.
		 */
		bool SubmitCommandList(Handle handle);

		/**
		 * Present swapchain.
		 */
		bool PresentSwapChain(Handle handle);

		/**
		 * Resize swapchain.
		 */
		bool ResizeSwapChain(Handle handle, i32 width, i32 height);

		/**
		 * Is valid handle?
		 */
		bool IsValidHandle(Handle handle) const;

		/**
		 * Get handle allocator.
		 */
		const Core::HandleAllocator& GetHandleAllocator() const;


		/**
		 * Begin debug capture.
		 */
		void BeginDebugCapture(const char* name);

		/**
		 * End debug capture.
		 */
		void EndDebugCapture();

		/**
		 * Scoped Debug capture.
		 */
		class ScopedDebugCapture
		{
		public:
			ScopedDebugCapture(Manager& manager, const char* name)
			    : manager_(manager)
			{
				manager_.BeginDebugCapture(name);
			}

			~ScopedDebugCapture() { manager_.EndDebugCapture(); }

		private:
			Manager& manager_;
		};


	private:
		Manager(const Manager&) = delete;
		struct ManagerImpl* impl_ = nullptr;
	};
} // namespace GPU
