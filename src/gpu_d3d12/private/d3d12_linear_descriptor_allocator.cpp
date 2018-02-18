#include "gpu_d3d12/d3d12_linear_descriptor_allocator.h"
#include "core/misc.h"

#include <utility>

namespace GPU
{
	D3D12LinearDescriptorAllocator::D3D12LinearDescriptorAllocator(
	    D3D12DescriptorHeapAllocator& allocator, i32 blockSize)
	    : allocator_(allocator)
	{
		alloc_ = allocator.Alloc(blockSize);

		ID3D12DescriptorHeap* heap = alloc_.allocator_->GetDescriptorHeap(alloc_);
		heap->GetDevice(IID_ID3D12Device, (void**)d3dDevice_.ReleaseAndGetAddressOf());
		heapType_ = heap->GetDesc().Type;

		ClearDescriptorRange(alloc_.GetDescriptorHeap(), DescriptorHeapSubType::INVALID, alloc_.GetCPUHandle(0),
		    blockSize, alloc_.GetDebugDataRange(0, blockSize));
	}

	D3D12LinearDescriptorAllocator::~D3D12LinearDescriptorAllocator() { allocator_.Free(alloc_); }

	D3D12DescriptorAllocation D3D12LinearDescriptorAllocator::Alloc(i32 num, DescriptorHeapSubType subType)
	{
		auto offset = Core::AtomicAdd(&allocOffset_, num) - num;

		if((offset + num) <= alloc_.size_)
		{
			D3D12DescriptorAllocation retVal = alloc_;
			retVal.offset_ += offset;
			retVal.size_ = num;
			retVal.allocId_ &= 0xffff0000;

#if 1
			if(subType != DescriptorHeapSubType::INVALID)
				ClearDescriptorRange(retVal.GetDescriptorHeap(), subType, retVal.GetCPUHandle(0), retVal.size_,
				    retVal.GetDebugDataRange(0, retVal.size_));
#endif

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
			d3dDevice_->CopyDescriptorsSimple(copySize, retVal.GetCPUHandle(0), src.GetCPUHandle(0), heapType_);
			for(i32 i = 0; i < size; ++i)
			{
				retVal.GetDebugData(i) = src.GetDebugData(i);
			}
		}
		return retVal;
	}

	void D3D12LinearDescriptorAllocator::Reset()
	{
		ClearDescriptorRange(alloc_.GetDescriptorHeap(), DescriptorHeapSubType::INVALID, alloc_.GetCPUHandle(0),
		    alloc_.size_, alloc_.GetDebugDataRange(0, alloc_.size_));
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
			retVal.offset_ += allocOffset_;
			retVal.size_ = padding;

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
