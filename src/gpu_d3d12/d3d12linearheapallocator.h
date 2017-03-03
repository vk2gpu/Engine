#pragma once

#include "gpu_d3d12/d3d12types.h"
#include "core/concurrency.h"
#include "core/vector.h"

namespace GPU
{
	struct D3D12ResourceAllocation
	{
		/// Base resource we're pointing to.
		ComPtr<ID3D12Resource> baseResource_;
		/// Offset in base resource.
		i64 offsetInBaseResource_ = 0;
		/// Address of allocation.
		void* address_ = nullptr;
		/// Size of allocation.
		i64 size_ = 0;
	};

	class D3D12LinearHeapAllocator
	{
	public:
		static const i64 DEFAULT_ALIGNMENT = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
		static const i64 MAX_ALIGNMENT = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		static const i64 MIN_RESOURCE_BLOCK_SIZE = MAX_ALIGNMENT;

	public:
		D3D12LinearHeapAllocator(ID3D12Device* device, D3D12_HEAP_TYPE heapType, i64 minResourceBlockSize);
		~D3D12LinearHeapAllocator();

		D3D12ResourceAllocation Alloc(i64 Size, i64 Alignment = DEFAULT_ALIGNMENT);
		void Reset();


	private:
		struct ResourceBlock
		{
			ComPtr<ID3D12Resource> resource_;
			void* baseAddress_ = nullptr;
			i64 size_ = 0;
			i64 currentOffset_ = 0;
			i64 allocCounter_ = 0;
		};

		ResourceBlock CreateResourceBlock(i64 Size);

	private:
		/// device to use.
		ComPtr<ID3D12Device> device_;
		/// Heap type we are allocating for.
		D3D12_HEAP_TYPE heapType_ = D3D12_HEAP_TYPE_DEFAULT;
		/// Minimum resource block size.
		i64 minResourceBlockSize_ = 0;
		/// Mutex to allow multiple threads to allocate at the same time.
		Core::Mutex allocMutex_;
		/// Blocks in pool.
		Core::Vector<ResourceBlock> blocks_;
		/// Blocks created.
		i64 blocksCreated_ = 0;
	};
} // namespace GPU
