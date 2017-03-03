#pragma once

#include "gpu/dll.h"
#include "gpu/backend.h"
#include "gpu_d3d12/d3d12commandlist.h"
#include "gpu_d3d12/d3d12resources.h"

#include "core/concurrency.h"
#include "core/library.h"
#include "core/vector.h"

namespace GPU
{
	class D3D12Backend : public IBackend
	{
	public:
		D3D12Backend(void* deviceWindow);
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
		ErrorCode CreateSamplerState(Handle handle, const SamplerState& state, const char* debugName) override;
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

	private:
		ComPtr<IDXGIDebug> dxgiDebug_;
		ComPtr<ID3D12Debug> d3dDebug_;

		ComPtr<IDXGIFactory4> dxgiFactory_;

		/// Cached adapter infos.
		Core::Vector<ComPtr<IDXGIAdapter1>> adapters_;
		Core::Vector<GPU::AdapterInfo> adapterInfos_;

		/// D3D12 devices.
		class D3D12Device* device_ = nullptr;

		/// Allocator for uploading data to the GPU.
		class D3D12LinearHeapAllocator* uploadAllocator_ = nullptr;
		/// Allocator for reading data back from the GPU.
		class D3D12LinearHeapAllocator* readbackAllocator_ = nullptr;

		/// Resources.
		Core::Mutex resourceMutex_;
		ResourceVector<D3D12SwapChainResource> swapchainResources_;
		ResourceVector<D3D12Resource> bufferResources_;
		ResourceVector<D3D12Resource> textureResources_;
		ResourceVector<D3D12CommandList*> commandLists_;
	};

} // namespace GPU
