#pragma once

#include "gpu_d3d12/d3d12types.h"
#include "gpu_d3d12/d3d12resources.h"
#include "gpu/resources.h"

namespace GPU
{
	class D3D12Device
	{
	public:
		D3D12Device(IDXGIFactory4* dxgiFactory, IDXGIAdapter1* adapter);
		~D3D12Device();

		void CreateCommandQueues();
		void CreateRootSignatures();
		void CreateDefaultPSOs();

		ErrorCode CreateSwapChain(
		    D3D12SwapChainResource& outResource, const SwapChainDesc& desc, const char* debugName);
		ErrorCode CreateBuffer(D3D12Resource& outResource, const BufferDesc& desc, const void* initialData, const char* debugName);
		ErrorCode CreateTexture(D3D12Resource& outResource, const TextureDesc& desc, const TextureSubResourceData* initialData, const char* debugName);

		operator bool() const { return !!device_; }

		ComPtr<IDXGIFactory4> dxgiFactory_;
		ComPtr<ID3D12Device> device_;

		ComPtr<ID3D12CommandQueue> directQueue_;
		ComPtr<ID3D12CommandQueue> copyQueue_;
		ComPtr<ID3D12CommandQueue> asyncComputeQueue_;

		// Root signatures.
		Core::Vector<ComPtr<ID3D12RootSignature>> rootSignatures_;

		Core::Vector<ComPtr<ID3D12PipelineState>> defaultPSOs_;
	};
} // namespace GPU
