#pragma once

#include "gpu_d3d12/d3d12_types.h"
#include "gpu_d3d12/d3d12_command_list.h"
#include "gpu_d3d12/d3d12_resources.h"
#include "gpu/resources.h"
#include "core/array.h"

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
		void CreateUploadAllocators();

		void NextFrame();

		ErrorCode CreateSwapChain(
		    D3D12SwapChainResource& outResource, const SwapChainDesc& desc, const char* debugName);
		ErrorCode CreateBuffer(D3D12Resource& outResource, const BufferDesc& desc, const void* initialData, const char* debugName);
		ErrorCode CreateTexture(D3D12Resource& outResource, const TextureDesc& desc, const TextureSubResourceData* initialData, const char* debugName);

		ErrorCode SubmitCommandList(D3D12CommandList& commandList);


		operator bool() const { return !!d3dDevice_; }

		ComPtr<IDXGIFactory4> dxgiFactory_;
		ComPtr<ID3D12Device> d3dDevice_;

		ComPtr<ID3D12CommandQueue> d3dDirectQueue_;
		ComPtr<ID3D12CommandQueue> d3dCopyQueue_;
		ComPtr<ID3D12CommandQueue> d3dAsyncComputeQueue_;

		/// Frame counter.
		i32 frameIdx_ = 0;

		/// Allocator for uploading data to the GPU.
		/// TODO: Replace this with a circular buffer instead of array of allocators.
		Core::Array<class D3D12LinearHeapAllocator*, MAX_GPU_FRAMES> uploadAllocators_;
		class D3D12CommandList* uploadCommandList_;
		ComPtr<ID3D12Fence> d3dUploadFence_;
		HANDLE uploadFenceEvent_ = 0;
		volatile i64 uploadFenceIdx_ = 0;

		D3D12LinearHeapAllocator& GetUploadAllocator() { return *uploadAllocators_[frameIdx_ % MAX_GPU_FRAMES]; }

		// Root signatures.
		Core::Vector<ComPtr<ID3D12RootSignature>> d3dRootSignatures_;

		Core::Vector<ComPtr<ID3D12PipelineState>> d3dDefaultPSOs_;
	};
} // namespace GPU
