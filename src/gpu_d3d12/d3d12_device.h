#pragma once

#include "gpu_d3d12/d3d12_types.h"
#include "gpu_d3d12/d3d12_backend.h"
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
		D3D12Device(
		    D3D12Backend& backend, const SetupParams& setupParams, IDXGIFactory4* dxgiFactory, IDXGIAdapter1* adapter);
		~D3D12Device();

		void CreateCommandQueues();
		void CreateRootSignatures();
		void CreateCommandSignatures();
		void CreateDefaultPSOs();
		void CreateUploadAllocators();
		void CreateDescriptorAllocators();

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

		ErrorCode UpdateSRVs(D3D12PipelineBindingSet& pbs, i32 first, i32 num, D3D12SubresourceRange* resources,
		    const D3D12_SHADER_RESOURCE_VIEW_DESC* descs);
		ErrorCode UpdateUAVs(D3D12PipelineBindingSet& pbs, i32 first, i32 num, D3D12SubresourceRange* resources,
		    const D3D12_UNORDERED_ACCESS_VIEW_DESC* descs);
		ErrorCode UpdateCBVs(D3D12PipelineBindingSet& pbs, i32 first, i32 num, D3D12SubresourceRange* resources,
		    const D3D12_CONSTANT_BUFFER_VIEW_DESC* descs);
		ErrorCode UpdateSamplers(
		    const D3D12PipelineBindingSet& pbs, i32 first, i32 num, const D3D12_SAMPLER_DESC* descs);
		ErrorCode UpdateFrameBindingSet(D3D12FrameBindingSet& frameBindingSet,
		    const D3D12_RENDER_TARGET_VIEW_DESC* rtvDescs, const D3D12_DEPTH_STENCIL_VIEW_DESC* dsvDesc);

		ErrorCode SubmitCommandLists(Core::ArrayView<D3D12CommandList*> commandLists);

		ErrorCode ResizeSwapChain(D3D12SwapChain& swapChain, i32 width, i32 height);

		operator bool() const { return !!d3dDevice_; }

		bool FlushUploads(i64 minCommands, i64 minBytes);

		ComPtr<IDXGIFactory4> dxgiFactory_;
		ComPtr<ID3D12Device> d3dDevice_;

		ComPtr<ID3D12CommandQueue> d3dDirectQueue_;       // direct
		ComPtr<ID3D12CommandQueue> d3dAsyncComputeQueue_; // compute

		/// Frame counter.
		i64 frameIdx_ = 0;
		ComPtr<ID3D12Fence> d3dFrameFence_;
		HANDLE frameFenceEvent_ = 0;

		/// Upload management.
		Core::Mutex uploadMutex_;
		Core::Array<class D3D12LinearHeapAllocator*, MAX_GPU_FRAMES> uploadAllocators_ = {};
		class D3D12CommandList* uploadCommandList_ = nullptr;
		ComPtr<ID3D12Fence> d3dUploadFence_;
		HANDLE uploadFenceEvent_ = 0;
		volatile i64 uploadBytesPending_ = 0;
		volatile i64 uploadCommandsPending_ = 0;
		volatile i64 uploadFenceIdx_ = 0;

		/// Descriptor allocators.
		class D3D12DescriptorHeapAllocator* viewAllocator_ = nullptr;
		class D3D12DescriptorHeapAllocator* samplerAllocator_ = nullptr;
		class D3D12DescriptorHeapAllocator* rtvAllocator_ = nullptr;
		class D3D12DescriptorHeapAllocator* dsvAllocator_ = nullptr;

		struct DescriptorAllocators
		{
			class D3D12LinearDescriptorAllocator* viewAllocator_ = nullptr;
			class D3D12LinearDescriptorAllocator* samplerAllocator_ = nullptr;
			class D3D12LinearDescriptorAllocator* rtvAllocator_ = nullptr;
			class D3D12LinearDescriptorAllocator* dsvAllocator_ = nullptr;

			class D3D12LinearDescriptorSubAllocator* cbvSubAllocator_ = nullptr;
			class D3D12LinearDescriptorSubAllocator* srvSubAllocator_ = nullptr;
			class D3D12LinearDescriptorSubAllocator* uavSubAllocator_ = nullptr;
		};

		Core::Array<DescriptorAllocators, MAX_GPU_FRAMES> descriptorAllocators_ = {};

		D3D12LinearDescriptorAllocator& GetSamplerDescriptorAllocator()
		{
			return *descriptorAllocators_[frameIdx_ % MAX_GPU_FRAMES].samplerAllocator_;
		}
		D3D12LinearDescriptorAllocator& GetViewDescriptorAllocator()
		{
			return *descriptorAllocators_[frameIdx_ % MAX_GPU_FRAMES].viewAllocator_;
		}

		D3D12LinearDescriptorSubAllocator& GetCBVSubAllocator()
		{
			return *descriptorAllocators_[frameIdx_ % MAX_GPU_FRAMES].cbvSubAllocator_;
		}
		D3D12LinearDescriptorSubAllocator& GetSRVSubAllocator()
		{
			return *descriptorAllocators_[frameIdx_ % MAX_GPU_FRAMES].srvSubAllocator_;
		}
		D3D12LinearDescriptorSubAllocator& GetUAVSubAllocator()
		{
			return *descriptorAllocators_[frameIdx_ % MAX_GPU_FRAMES].uavSubAllocator_;
		}


		D3D12LinearDescriptorAllocator& GetRTVDescriptorAllocator()
		{
			return *descriptorAllocators_[frameIdx_ % MAX_GPU_FRAMES].rtvAllocator_;
		}
		D3D12LinearDescriptorAllocator& GetDSVDescriptorAllocator()
		{
			return *descriptorAllocators_[frameIdx_ % MAX_GPU_FRAMES].dsvAllocator_;
		}


		D3D12LinearHeapAllocator& GetUploadAllocator() { return *uploadAllocators_[frameIdx_ % MAX_GPU_FRAMES]; }

		/// Root signatures.
		Core::Vector<ComPtr<ID3D12RootSignature>> d3dRootSignatures_;

		/// Command signatures
		ComPtr<ID3D12CommandSignature> d3dDrawCmdSig_;
		ComPtr<ID3D12CommandSignature> d3dDrawIndexedCmdSig_;
		ComPtr<ID3D12CommandSignature> d3dDispatchCmdSig_;

		Core::Vector<ComPtr<ID3D12PipelineState>> d3dDefaultPSOs_;


		/// Vendor specific extensions.
		AGSContext* agsContext_ = nullptr;
		unsigned int agsFeatureBits_ = 0;
	};
} // namespace GPU
