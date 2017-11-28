#include "gpu_d3d12/d3d12_linear_descriptor_allocator.h"
#include "core/misc.h"

#include <utility>

namespace GPU
{
	D3D12LinearDescriptorAllocator::D3D12LinearDescriptorAllocator(ID3D12Device* device,
	    D3D12_DESCRIPTOR_HEAP_TYPE heapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, i32 blockSize,
	    const char* debugName)
	    : d3dDevice_(device)
	    , heapType_(heapType)
	    , heapFlags_(heapFlags)
	    , blockSize_(blockSize)
	    , debugName_(debugName)
	{
		handleIncrementSize_ = d3dDevice_->GetDescriptorHandleIncrementSize(heapType);

		D3D12_DESCRIPTOR_HEAP_DESC desc;
		desc.Type = heapType_;
		desc.NumDescriptors = blockSize_;
		desc.Flags = heapFlags_;
		desc.NodeMask = 0x0;

		CHECK_D3D(d3dDevice_->CreateDescriptorHeap(
		    &desc, IID_ID3D12DescriptorHeap, (void**)d3dDescriptorHeap_.GetAddressOf()));
		SetObjectName(d3dDescriptorHeap_.Get(), debugName_);

		ClearDescriptorRange(d3dDescriptorHeap_.Get(), DescriptorHeapSubType::INVALID, 0, blockSize_);
	}

	D3D12LinearDescriptorAllocator::~D3D12LinearDescriptorAllocator() {}

	D3D12DescriptorAllocation D3D12LinearDescriptorAllocator::Alloc(i32 numDescriptors)
	{
		auto offset = Core::AtomicAdd(&allocOffset_, numDescriptors);

		if((offset + numDescriptors) <= blockSize_)
		{
			D3D12DescriptorAllocation retVal;
			retVal.d3dDescriptorHeap_ = d3dDescriptorHeap_;
			retVal.offset_ = offset;
			retVal.size_ = numDescriptors;
			retVal.allocId_ = 0;
			retVal.cpuDescHandle_ = d3dDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
			retVal.gpuDescHandle_ = d3dDescriptorHeap_->GetGPUDescriptorHandleForHeapStart();

			if(numDescriptors > 0)
			{
				retVal.cpuDescHandle_.ptr += (offset * handleIncrementSize_);
				retVal.gpuDescHandle_.ptr += (offset * handleIncrementSize_);
			}
			return retVal;
		}
		// Failure.
		DBG_BREAK;
		return D3D12DescriptorAllocation();
	}

	D3D12DescriptorAllocation D3D12LinearDescriptorAllocator::Copy(const D3D12DescriptorAllocation& src, i32 size)
	{
		i32 copySize = Core::Min(size, src.size_);
		auto retVal = Alloc(size);
		if(copySize > 0)
			d3dDevice_->CopyDescriptorsSimple(copySize, retVal.cpuDescHandle_, src.cpuDescHandle_, heapType_);
		return retVal;
	}

	void D3D12LinearDescriptorAllocator::Reset()
	{
		ClearDescriptorRange(d3dDescriptorHeap_.Get(), DescriptorHeapSubType::INVALID, 0, blockSize_);
		Core::AtomicExchg(&allocOffset_, 0);
	}

} // namespace GPU
