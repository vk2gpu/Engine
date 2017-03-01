#pragma once

#include "gpu/dll.h"
#include "gpu/d3d12/d3d12types.h"
#include "gpu/private/backend.h"

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
		 * Device operations.
		 */
		i32 EnumerateAdapters(AdapterInfo* outAdapters, i32 maxAdapters) override;

		/**
		 * Initialize adapter.
		 */
		ErrorCode InitializeAdapter(i32 adapterIdx) override;

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
		ErrorCode LoadLibraries();

		/// DXGI & D3D12 DLL funcs.
		Core::LibHandle dxgiHandle_ = 0;
		Core::LibHandle d3d12Handle_ = 0;
		PFN_CREATE_DXGI_FACTORY dxgiCreateDXGIFactory1Fn_ = 0;
		PFN_D3D12_CREATE_DEVICE d3d12CreateDeviceFn_ = 0;
		PFN_D3D12_GET_DEBUG_INTERFACE d3d12GetDebugInterfaceFn_ = 0;
		PFN_D3D12_SERIALIZE_ROOT_SIGNATURE d3d12SerializeRootSignatureFn_ = 0;

		ComPtr<IDXGIFactory1> dxgiFactory_;

		/// Cached adapter infos.
		Core::Vector<ComPtr<IDXGIAdapter>> adapters_; 
		Core::Vector<GPU::AdapterInfo> adapterInfos_;

		/// D3D12 devices.
		ComPtr<ID3D12Device> device_;
	};

} // namespace GPU
