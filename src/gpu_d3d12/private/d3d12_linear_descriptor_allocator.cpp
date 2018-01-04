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

		debugData_.resize(blockSize);

		ClearDescriptorRange(d3dDescriptorHeap_.Get(), debugData_, DescriptorHeapSubType::INVALID, 0, blockSize_);
	}

	D3D12LinearDescriptorAllocator::~D3D12LinearDescriptorAllocator() {}

	D3D12DescriptorAllocation D3D12LinearDescriptorAllocator::Alloc(i32 num, DescriptorHeapSubType subType)
	{
		auto offset = Core::AtomicAdd(&allocOffset_, num) - num;

		if((offset + num) <= blockSize_)
		{
			D3D12DescriptorAllocation retVal;
			retVal.d3dDescriptorHeap_ = d3dDescriptorHeap_;
			retVal.debugData_ = debugData_;
			retVal.offset_ = offset;
			retVal.size_ = num;
			retVal.allocId_ = 0;
			retVal.cpuDescHandle_ = d3dDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
			retVal.gpuDescHandle_ = d3dDescriptorHeap_->GetGPUDescriptorHandleForHeapStart();

			if(num > 0)
			{
				retVal.cpuDescHandle_.ptr += (offset * handleIncrementSize_);
				retVal.gpuDescHandle_.ptr += (offset * handleIncrementSize_);
			}

			if(subType != DescriptorHeapSubType::INVALID)
				ClearDescriptorRange(
				    d3dDescriptorHeap_.Get(), retVal.debugData_, subType, retVal.offset_, retVal.size_);

			return retVal;
		}
		// Failure.
		DBG_ASSERT(false);
		return D3D12DescriptorAllocation();
	}

	D3D12DescriptorAllocation D3D12LinearDescriptorAllocator::Copy(
	    const D3D12DescriptorAllocation& src, i32 size, DescriptorHeapSubType subType)
	{
		i32 copySize = Core::Min(size, src.size_);
		auto retVal = Alloc(size, subType);
		if(copySize > 0)
		{
			d3dDevice_->CopyDescriptorsSimple(copySize, retVal.cpuDescHandle_, src.cpuDescHandle_, heapType_);
			for(i32 i = 0; i < size; ++i)
			{
				retVal.debugData_[retVal.offset_ + i] = src.debugData_[src.offset_ + i];
			}
		}
		return retVal;
	}

	void D3D12LinearDescriptorAllocator::Reset()
	{
		ClearDescriptorRange(d3dDescriptorHeap_.Get(), debugData_, DescriptorHeapSubType::INVALID, 0, blockSize_);
		Core::AtomicExchg(&allocOffset_, 0);
	}


	D3D12LinearDescriptorSubAllocator::D3D12LinearDescriptorSubAllocator(
	    D3D12LinearDescriptorAllocator& allocator, DescriptorHeapSubType subType, i32 blockSize)
	    : allocator_(allocator)
	    , subType_(subType)
	    , blockSize_(blockSize)
	{
	}

	D3D12LinearDescriptorSubAllocator::~D3D12LinearDescriptorSubAllocator() {}

	D3D12DescriptorAllocation D3D12LinearDescriptorSubAllocator::Alloc(i32 num, i32 padding)
	{
		D3D12DescriptorAllocation retVal = {};

		Core::ScopedMutex lock(mutex_);

		// Allocate a new block if we need to.
		i32 numRemaining = alloc_.size_ - allocOffset_;
		if(numRemaining < padding)
		{
			i32 blockSize = Core::Max(blockSize_, padding);
			alloc_ = allocator_.Alloc(blockSize, subType_);
			allocOffset_ = 0;
		}

		// Sanity check, previous allocation can fail.
		numRemaining = alloc_.size_ - allocOffset_;
		if(numRemaining >= padding)
		{
			retVal = alloc_;
			retVal.debugData_ = alloc_.debugData_;
			retVal.offset_ += allocOffset_;
			retVal.size_ = padding;

			retVal.cpuDescHandle_.ptr += allocOffset_ * allocator_.GetHandleIncrementSize();
			retVal.gpuDescHandle_.ptr += allocOffset_ * allocator_.GetHandleIncrementSize();

			// TODO: Don't clear!
			ClearDescriptorRange(
			    retVal.d3dDescriptorHeap_.Get(), alloc_.debugData_, subType_, retVal.offset_, retVal.size_);

			allocOffset_ += num;
		}

		return retVal;
	}

	void D3D12LinearDescriptorSubAllocator::Reset()
	{
		Core::ScopedMutex lock(mutex_);
		alloc_ = D3D12DescriptorAllocation();
		allocOffset_ = 0;
	}

} // namespace GPU
