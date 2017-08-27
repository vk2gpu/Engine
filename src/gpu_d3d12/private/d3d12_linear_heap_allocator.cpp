#include "gpu_d3d12/d3d12_linear_heap_allocator.h"
#include "core/misc.h"

#include <utility>

namespace GPU
{
	D3D12LinearHeapAllocator::D3D12LinearHeapAllocator(
	    ID3D12Device* d3dDevice, D3D12_HEAP_TYPE heapType, i64 minResourceBlockSize)
	    : d3dDevice_(d3dDevice)
	    , heapType_(heapType)
	    , minResourceBlockSize_(minResourceBlockSize)
	    , blocks_()
	    , blocksCreated_(0)
	{
		DBG_ASSERT(minResourceBlockSize_ >= MIN_RESOURCE_BLOCK_SIZE);
		DBG_ASSERT((minResourceBlockSize_ & (MAX_ALIGNMENT - 1)) == 0);
		DBG_ASSERT(heapType_ == D3D12_HEAP_TYPE_UPLOAD || heapType_ == D3D12_HEAP_TYPE_READBACK);
	}

	D3D12LinearHeapAllocator::~D3D12LinearHeapAllocator() {}

	D3D12ResourceAllocation D3D12LinearHeapAllocator::Alloc(i64 size, i64 alignment)
	{
		DBG_ASSERT(size > 0);
		DBG_ASSERT(alignment > 0 && alignment <= MAX_ALIGNMENT);

		Core::ScopedMutex lock(allocMutex_);

		D3D12ResourceAllocation allocation;

		ResourceBlock* foundBlock = nullptr;

		// Check if we have a valid block size.
		for(auto& block : blocks_)
		{
			i64 alignedOffset = Core::PotRoundUp(block.currentOffset_, alignment);
			i64 remaining = static_cast<i64>(block.size_ - alignedOffset);
			if(remaining >= static_cast<i64>(size))
			{
				foundBlock = &block;
			}
		}

		// Allocate new block if need be.
		if(foundBlock == nullptr)
		{
			auto newBlock = CreateResourceBlock(size);
			blocks_.push_back(newBlock);
			foundBlock = &blocks_.back();
		}

		// Grab the correct offset and assert remaining size.
		i64 alignedOffset = Core::PotRoundUp(foundBlock->currentOffset_, alignment);
		i64 remaining = static_cast<i64>(foundBlock->size_ - alignedOffset);
		DBG_ASSERT(remaining >= static_cast<i64>(size));

		// Setup allocation
		if(remaining >= static_cast<i64>(size))
		{
			allocation.baseResource_ = foundBlock->resource_;
			allocation.offsetInBaseResource_ = alignedOffset;
			allocation.address_ = static_cast<u8*>(foundBlock->baseAddress_) + alignedOffset;
			allocation.size_ = size;

			// Advance current offset.
			foundBlock->currentOffset_ = alignedOffset + size;
			DBG_ASSERT((allocation.offsetInBaseResource_ & (alignment - 1)) == 0);
		}

		return std::move(allocation);
	}

	void D3D12LinearHeapAllocator::Reset()
	{
		Core::ScopedMutex lock(allocMutex_);

		i64 totalUsage = 0;
		i64 totalSize = 0;

		// Set block offsets back to 0.
		for(auto& block : blocks_)
		{
			totalUsage += block.currentOffset_;
			totalSize += block.size_;
			block.currentOffset_ = 0;
		}

		// Reset stats.
		blocksCreated_ = 0;
	}

	D3D12LinearHeapAllocator::ResourceBlock D3D12LinearHeapAllocator::CreateResourceBlock(i64 size)
	{
		// Minimum sized block, round up to max alignment.
		ResourceBlock block;
		block.size_ = Core::PotRoundUp(Core::Max(size, minResourceBlockSize_), MAX_ALIGNMENT);

		// Setup heap properties.
		D3D12_HEAP_PROPERTIES heapProperties;
		heapProperties.Type = heapType_;
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProperties.CreationNodeMask = 0x0;
		heapProperties.VisibleNodeMask = 0x0;

		D3D12_RESOURCE_DESC resourceDesc;
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Alignment = MAX_ALIGNMENT;
		resourceDesc.Width = block.size_;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		// Setup appropriate resource usage.
		D3D12_RESOURCE_STATES resourceUsage =
		    (heapType_ == D3D12_HEAP_TYPE_UPLOAD) ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COPY_DEST;

		// Committed resource.
		HRESULT hr = S_OK;

		CHECK_D3D(hr = d3dDevice_->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		              resourceUsage, nullptr, IID_PPV_ARGS(block.resource_.GetAddressOf())));
#ifdef DEBUG
		SetObjectName(block.resource_.Get(), "D3D12LinearHeapAllocator");
#endif

		// Persistently map.
		CHECK_D3D(hr = block.resource_->Map(0, nullptr, &block.baseAddress_));

		// Blocks created.
		++blocksCreated_;

		return std::move(block);
	}
} // namespace GPU
