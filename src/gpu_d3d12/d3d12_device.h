#pragma once

#include "gpu_d3d12/d3d12_types.h"
#include "gpu_d3d12/d3d12_command_list.h"
#include "gpu_d3d12/d3d12_resources.h"
#include "gpu/resources.h"
#include "core/array.h"
#include "core/concurrency.h"

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
		void CreateDescriptorHeapAllocators();

		void NextFrame();

		ErrorCode CreateSwapChain(D3D12SwapChain& outResource, const SwapChainDesc& desc, const char* debugName);
		ErrorCode CreateBuffer(
		    D3D12Resource& outResource, const BufferDesc& desc, const void* initialData, const char* debugName);
		ErrorCode CreateTexture(D3D12Resource& outResource, const TextureDesc& desc,
		    const TextureSubResourceData* initialData, const char* debugName);

		ErrorCode CreateGraphicsPipelineState(
		    D3D12GraphicsPipelineState& outGps, D3D12_GRAPHICS_PIPELINE_STATE_DESC desc, const char* debugName);
		ErrorCode CreateComputePipelineState(
		    D3D12ComputePipelineState& outCps, D3D12_COMPUTE_PIPELINE_STATE_DESC desc, const char* debugName);

		ErrorCode CreatePipelineBindingSet(
		    D3D12PipelineBindingSet& outPipelineBindingSet, const PipelineBindingSetDesc& desc, const char* debugName);
		void DestroyPipelineBindingSet(D3D12PipelineBindingSet& pipelineBindingSet);

		ErrorCode CreateFrameBindingSet(
		    D3D12FrameBindingSet& outFrameBindingSet, const FrameBindingSetDesc& desc, const char* debugName);
		void DestroyFrameBindingSet(D3D12FrameBindingSet& frameBindingSet);

		ErrorCode UpdateSRVs(D3D12PipelineBindingSet& pipelineBindingSet, i32 first, i32 num,
		    ID3D12Resource** resources, const D3D12_SHADER_RESOURCE_VIEW_DESC* descs);
		ErrorCode UpdateUAVs(D3D12PipelineBindingSet& pipelineBindingSet, i32 first, i32 num,
		    ID3D12Resource** resources, const D3D12_UNORDERED_ACCESS_VIEW_DESC* descs);
		ErrorCode UpdateCBVs(D3D12PipelineBindingSet& pipelineBindingSet, i32 first, i32 num,
		    const D3D12_CONSTANT_BUFFER_VIEW_DESC* descs);
		ErrorCode UpdateSamplers(
		    D3D12PipelineBindingSet& pipelineBindingSet, i32 first, i32 num, const D3D12_SAMPLER_DESC* descs);
		ErrorCode UpdateFrameBindingSet(D3D12FrameBindingSet& frameBindingSet,
		    const D3D12_RENDER_TARGET_VIEW_DESC* rtvDescs, const D3D12_DEPTH_STENCIL_VIEW_DESC* dsvDescs);

		ErrorCode SubmitCommandList(D3D12CommandList& commandList);


		operator bool() const { return !!d3dDevice_; }

		ComPtr<IDXGIFactory4> dxgiFactory_;
		ComPtr<ID3D12Device> d3dDevice_;

		ComPtr<ID3D12CommandQueue> d3dDirectQueue_;
		ComPtr<ID3D12CommandQueue> d3dCopyQueue_;
		ComPtr<ID3D12CommandQueue> d3dAsyncComputeQueue_;

		/// Frame counter.
		i64 frameIdx_ = 0;
		ComPtr<ID3D12Fence> d3dFrameFence_;
		HANDLE frameFenceEvent_ = 0;

		/// Upload management.
		Core::Mutex uploadMutex_;
		Core::Array<class D3D12LinearHeapAllocator*, MAX_GPU_FRAMES> uploadAllocators_;
		class D3D12CommandList* uploadCommandList_;
		ComPtr<ID3D12Fence> d3dUploadFence_;
		HANDLE uploadFenceEvent_ = 0;
		volatile i64 uploadFenceIdx_ = 0;

		/// Descriptor heap allocators.
		class D3D12DescriptorHeapAllocator* cbvSrvUavAllocator_ = nullptr;
		class D3D12DescriptorHeapAllocator* samplerAllocator_ = nullptr;
		class D3D12DescriptorHeapAllocator* rtvAllocator_ = nullptr;
		class D3D12DescriptorHeapAllocator* dsvAllocator_ = nullptr;

		D3D12LinearHeapAllocator& GetUploadAllocator() { return *uploadAllocators_[frameIdx_ % MAX_GPU_FRAMES]; }

		/// Root signatures.
		Core::Vector<ComPtr<ID3D12RootSignature>> d3dRootSignatures_;

		Core::Vector<ComPtr<ID3D12PipelineState>> d3dDefaultPSOs_;
	};
} // namespace GPU
