#pragma once

#include "core/types.h"
#include "gpu/dll.h"
#include "gpu/command_list.h"
#include "gpu/resources.h"
#include "gpu/types.h"

namespace GPU
{
	class GPU_DLL Manager final
	{
	public:
		/**
		 * Initialize GPU manager
		 * @param setpParams Setup parameters used to create appropriate backend.
		 */
		static void Initialize(const SetupParams& setupParams);

		/**
		 * Finalize GPU manager.
		 */
		static void Finalize();

		/**
		 * Is initialized?
		 */
		static bool IsInitialized();

		/**
		 * Enumerate adapters.
		 * @param outAdapters Pointer to adapter info structure array.
		 * @param maxAdapters Maximum number to enumerate.
		 * @return Total number of adapters.
		 */
		static i32 EnumerateAdapters(AdapterInfo* outAdapters, i32 maxAdapters);

		/**
		 * Create adapter.
		 * @param adapterIdx Adapter index.
		 * @pre !IsInitialized()
		 */
		static ErrorCode CreateAdapter(i32 adapterIdx);

		/**
		 * Is adapter created?
		 */
		static bool IsAdapterCreated();

		/**
		 * Create swapchain.
		 * @param desc Swapchain descriptor.
		 * @param debugName Debug name.
		 */
		static Handle CreateSwapChain(const SwapChainDesc& desc, const char* debugName);

		/**
		 * Create buffer.
		 * @param desc Buffer descriptor.
		 * @param initialData Data to create buffer with.
		 * @param debugName Debug name.
		 * @pre initialData == nullptr, or allocated size matches one in @a desc
		 */
		static Handle CreateBuffer(const BufferDesc& desc, const void* initialData, const char* debugName);

		/**
		 * Create texture.
		 * @param desc Texture descriptor.
		 * @param initialData Data to create texture with.
		 * @param debugName Debug name.
		 * @pre initialData == nullptr, or has enough for (levels * elements).
		 */
		static Handle CreateTexture(
		    const TextureDesc& desc, const TextureSubResourceData* initialData, const char* debugName);

		/**
		 * Create sample state.
		 * @param samplerState Sampler state to create.
		 */
		static Handle CreateSamplerState(const SamplerState& state, const char* debugName);

		/**
		 * Create shader.
		 * @param desc Shader desc, contains byte code and type.
		 * @param debugName Debug name.
		 */
		static Handle CreateShader(const ShaderDesc& desc, const char* debugName);

		/**
		 * Create graphics pipeline state.
		 */
		static Handle CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc, const char* debugName);

		/**
		 * Create compute pipeline state.
		 */
		static Handle CreateComputePipelineState(const ComputePipelineStateDesc& desc, const char* debugName);

		/**
		 * Create pipeline binding set.
		 */
		static Handle CreatePipelineBindingSet(const PipelineBindingSetDesc& desc, const char* debugName);

		/**
		 * Create draw binding set.
		 */
		static Handle CreateDrawBindingSet(const DrawBindingSetDesc& desc, const char* debugName);

		/**
		 * Create frame binding set.
		 */
		static Handle CreateFrameBindingSet(const FrameBindingSetDesc& desc, const char* debugName);

		/**
		 * Create command list.
		 */
		static Handle CreateCommandList(const char* debugName);

		/**
		 * Create fence.
		 * Used for synchronisation in and around queues.
		 */
		static Handle CreateFence(const char* debugName);

		/**
		 * Destroy resource.
		 */
		static void DestroyResource(Handle handle);

		/**
		 * Compile command list.
		 * @param handle Handle to command list.
		 * @param commandList Input software command list.
		 * @return Success.
		 */
		static bool CompileCommandList(Handle handle, const CommandList& commandList);

		/**
		 * Submit command list.
		 * @param handle Handle to command list.
		 * @return Success.
		 */
		static bool SubmitCommandList(Handle handle);

		/**
		 * Submit command lists.
		 * @param handles Handles to command lists.
		 * @return Success.
		 */
		static bool SubmitCommandLists(Core::ArrayView<Handle> handles);

		/**
		 * Present swapchain.
		 */
		static bool PresentSwapChain(Handle handle);

		/**
		 * Resize swapchain.
		 */
		static bool ResizeSwapChain(Handle handle, i32 width, i32 height);

		/**
		 * Next frame.
		 */
		static void NextFrame();

		/**
		 * Is valid handle?
		 */
		static bool IsValidHandle(Handle handle);

		/**
		 * Get handle allocator.
		 */
		static const Core::HandleAllocator& GetHandleAllocator();

		/**
		 * Begin debug capture.
		 */
		static void BeginDebugCapture(const char* name);

		/**
		 * End debug capture.
		 */
		static void EndDebugCapture();

		/**
		 * Open debug capture.
		 * Where supported, this will open the appropriate debug tool.
		 * @param quitOnOpen If true, will quit once debug capture application has opened.
		 */
		static void OpenDebugCapture(bool quitOnOpen = false);

		/**
		 * Trigger debug capture.
		 * Where supported, this will trigger a capture with debug tool.
		 */
		static void TriggerDebugCapture();

		/**
		 * Scoped Debug capture.
		 */
		class ScopedDebugCapture
		{
		public:
			ScopedDebugCapture(const char* name) { BeginDebugCapture(name); }
			~ScopedDebugCapture() { EndDebugCapture(); }
		};

		/**
		 * Scoped manager init/fini.
		 * Mostly a convenience for unit tests.
		 */
		class Scoped
		{
		public:
			Scoped(const SetupParams& setupParams) { Initialize(setupParams); }
			~Scoped() { Finalize(); }
		};


	private:
		Manager() = delete;
		~Manager() = delete;
		Manager(const Manager&) = delete;
	};
} // namespace GPU
 