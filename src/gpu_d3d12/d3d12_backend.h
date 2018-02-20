#pragma once

#include "gpu/dll.h"
#include "gpu/backend.h"
#include "gpu_d3d12/d3d12_command_list.h"
#include "gpu_d3d12/d3d12_compile_context.h"
#include "gpu_d3d12/d3d12_resources.h"

#include "core/concurrency.h"
#include "core/library.h"
#include "core/vector.h"

namespace GPU
{
	class D3D12Backend : public IBackend
	{
	public:
		D3D12Backend(const SetupParams& setupParams);
		~D3D12Backend();

		/**
		 * device operations.
		 */
		i32 EnumerateAdapters(AdapterInfo* outAdapters, i32 maxAdapters) override;
		bool IsInitialized() const override;
		ErrorCode Initialize(i32 adapterIdx) override;

		/**
		 * Resource creation/destruction.
		 */
		ErrorCode CreateSwapChain(Handle handle, const SwapChainDesc& desc, const char* debugName) override;
		ErrorCode CreateBuffer(
		    Handle handle, const BufferDesc& desc, const void* initialData, const char* debugName) override;
		ErrorCode CreateTexture(Handle handle, const TextureDesc& desc, const TextureSubResourceData* initialData,
		    const char* debugName) override;
		ErrorCode CreateShader(Handle handle, const ShaderDesc& desc, const char* debugName) override;
		ErrorCode CreateGraphicsPipelineState(
		    Handle handle, const GraphicsPipelineStateDesc& desc, const char* debugName) override;
		ErrorCode CreateComputePipelineState(
		    Handle handle, const ComputePipelineStateDesc& desc, const char* debugName) override;
		ErrorCode CreatePipelineBindingSet(
		    Handle handle, const PipelineBindingSetDesc& desc, const char* debugName) override;
		ErrorCode CreateDrawBindingSet(Handle handle, const DrawBindingSetDesc& desc, const char* debugName) override;
		ErrorCode CreateFrameBindingSet(Handle handle, const FrameBindingSetDesc& desc, const char* debugName) override;
		ErrorCode CreateCommandList(Handle handle, const char* debugName) override;
		ErrorCode CreateFence(Handle handle, const char* debugName) override;
		ErrorCode DestroyResource(Handle handle) override;

		ErrorCode AllocTemporaryPipelineBindingSet(Handle handle, const PipelineBindingSetDesc& desc) override;
		ErrorCode UpdatePipelineBindings(Handle pbs, i32 base, Core::ArrayView<const BindingCBV> descs) override;
		ErrorCode UpdatePipelineBindings(Handle pbs, i32 base, Core::ArrayView<const BindingSRV> descs) override;
		ErrorCode UpdatePipelineBindings(Handle pbs, i32 base, Core::ArrayView<const BindingUAV> descs) override;
		ErrorCode UpdatePipelineBindings(Handle pbs, i32 base, Core::ArrayView<const SamplerState> descs) override;
		ErrorCode CopyPipelineBindings(
		    Core::ArrayView<const PipelineBinding> dst, Core::ArrayView<const PipelineBinding> src) override;
		ErrorCode ValidatePipelineBindings(Core::ArrayView<const PipelineBinding> pb) override;

		ErrorCode CompileCommandList(Handle handle, const CommandList& commandList) override;
		ErrorCode SubmitCommandLists(Core::ArrayView<Handle> handles) override;

		ErrorCode PresentSwapChain(Handle handle) override;
		ErrorCode ResizeSwapChain(Handle handle, i32 width, i32 height) override;

		void NextFrame() override;

		/**
		 * Will return D3D12Resource for a @a handle.
		 * Supports BUFFER, TEXTURE, and SWAP_CHAIN.
		 */
		ResourceRead<D3D12Resource> GetD3D12Resource(Handle handle);

		/**
		 * Will return D3D12Buffer for a @a handle.
		 * Supports BUFFER.
		 */
		ResourceRead<D3D12Buffer> GetD3D12Buffer(Handle handle);

		/**
		 * Will return D3D12Resource for a @a handle.
		 * Supports TEXTURE and SWAP_CHAIN.
		 * @param bufferIdx If >= 0, will return appropriate buffer for swapchain.
		 */
		ResourceRead<D3D12Texture> GetD3D12Texture(Handle handle, i32 bufferIdx = -1);

		ComPtr<IDXGIDebug> dxgiDebug_;
		ComPtr<ID3D12Debug> d3dDebug_;
		ComPtr<ID3D12Debug1> d3dDebug1_;

		ComPtr<IDXGIFactory4> dxgiFactory_;

		const SetupParams& setupParams_;

		/// Cached adapter infos.
		Core::Vector<ComPtr<IDXGIAdapter1>> dxgiAdapters_;
		Core::Vector<GPU::AdapterInfo> adapterInfos_;

		/// D3D12 devices.
		class D3D12Device* device_ = nullptr;

		/// Resources.
		ResourcePool<D3D12SwapChain> swapchainResources_;
		ResourcePool<D3D12Buffer> bufferResources_;
		ResourcePool<D3D12Texture> textureResources_;
		ResourcePool<D3D12Shader> shaders_;
		ResourcePool<D3D12GraphicsPipelineState> graphicsPipelineStates_;
		ResourcePool<D3D12ComputePipelineState> computePipelineStates_;
		ResourcePool<D3D12PipelineBindingSet> pipelineBindingSets_;
		ResourcePool<D3D12DrawBindingSet> drawBindingSets_;
		ResourcePool<D3D12FrameBindingSet> frameBindingSets_;
		ResourcePool<D3D12CommandList*> commandLists_;


		/// Vendor specific extensions.
		AGSContext* agsContext_ = nullptr;
		AGSGPUInfo agsGPUInfo_;
	};


} // namespace GPU
