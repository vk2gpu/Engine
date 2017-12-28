#include "gpu_d3d12/d3d12_descriptor_heap_allocator.h"
#include "core/misc.h"

#include <utility>
#include <algorithm>

namespace GPU
{
	D3D12DescriptorHeapAllocator::D3D12DescriptorHeapAllocator(ID3D12Device* device,
	    D3D12_DESCRIPTOR_HEAP_TYPE heapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, i32 blockSize,
	    const char* debugName)
	    : d3dDevice_(device)
	    , heapType_(heapType)
	    , heapFlags_(heapFlags)
	    , blockSize_(blockSize)
	    , debugName_(debugName)
	{
		handleIncrementSize_ = d3dDevice_->GetDescriptorHandleIncrementSize(heapType);
		AddBlock();
	}

	D3D12DescriptorHeapAllocator::~D3D12DescriptorHeapAllocator() {}

	D3D12DescriptorAllocation D3D12DescriptorHeapAllocator::Alloc(i32 size)
	{
		if(size == 0)
			return D3D12DescriptorAllocation();

		Core::ScopedMutex lock(allocMutex_);

		for(i32 i = 0; i < blocks_.size(); ++i)
		{
			auto& block = blocks_[i];

			auto allocId = block.allocator_.AllocRange(size);
			if(auto alloc = block.allocator_.GetAlloc(allocId))
			{
				D3D12DescriptorAllocation retVal;
				retVal.d3dDescriptorHeap_ = block.d3dDescriptorHeap_;
				retVal.offset_ = alloc.offset_;
				retVal.size_ = size;
				retVal.allocId_ = ((u32)i << 16) | (u32)allocId;
				retVal.cpuDescHandle_ = block.d3dDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
				retVal.cpuDescHandle_.ptr += (alloc.offset_ * handleIncrementSize_);
				retVal.gpuDescHandle_ = block.d3dDescriptorHeap_->GetGPUDescriptorHandleForHeapStart();
				retVal.gpuDescHandle_.ptr += (alloc.offset_ * handleIncrementSize_);
				block.numAllocs_++;
				return retVal;
			}
			else if(i == (blocks_.size() - 1))
			{
				DBG_ASSERT(block.numAllocs_ > 0);
				AddBlock();
			}
		}

		// Failure.
		DBG_BREAK;
		return D3D12DescriptorAllocation();
	}

	void D3D12DescriptorHeapAllocator::Free(D3D12DescriptorAllocation alloc)
	{
		if(alloc.allocId_ == 0)
			return;

		Core::ScopedMutex lock(allocMutex_);
		auto& block = blocks_[alloc.allocId_ >> 16];
		DBG_ASSERT(block.numAllocs_ >= 1);

		block.allocator_.FreeRange(alloc.allocId_ & 0xffff);
		block.numAllocs_--;
	}

	void D3D12DescriptorHeapAllocator::AddBlock()
	{
		DescriptorBlock block(blockSize_, Core::Min(blockSize_, 4096));
		D3D12_DESCRIPTOR_HEAP_DESC desc;
		desc.Type = heapType_;
		desc.NumDescriptors = blockSize_;
		desc.Flags = heapFlags_;
		desc.NodeMask = 0x0;

		CHECK_D3D(d3dDevice_->CreateDescriptorHeap(
		    &desc, IID_ID3D12DescriptorHeap, (void**)block.d3dDescriptorHeap_.GetAddressOf()));
		SetObjectName(block.d3dDescriptorHeap_.Get(), debugName_);

		ClearDescriptorRange(block.d3dDescriptorHeap_.Get(), DescriptorHeapSubType::INVALID, 0, blockSize_);

		blocks_.emplace_back(std::move(block));
	}


} // namespace GPU
