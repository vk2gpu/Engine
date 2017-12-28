#pragma once

#include "gpu_d3d12/d3d12_types.h"
#include "core/concurrency.h"
#include "core/external_allocator.h"
#include "core/vector.h"

namespace GPU
{
	class D3D12DescriptorHeapAllocator
	{
	public:
		D3D12DescriptorHeapAllocator(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType,
		    D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, i32 blockSize, const char* debugName);
		~D3D12DescriptorHeapAllocator();

		D3D12DescriptorAllocation Alloc(i32 size);

		void Free(D3D12DescriptorAllocation alloc);

		i32 GetHandleIncrementSize() const { return handleIncrementSize_; }

	private:
		D3D12DescriptorHeapAllocator(const D3D12DescriptorHeapAllocator&) = delete;

		void AddBlock();

		struct DescriptorBlock
		{
			DescriptorBlock(i32 size, i32 maxAllocs)
			    : allocator_(size, maxAllocs)
			{
			}

			ComPtr<ID3D12DescriptorHeap> d3dDescriptorHeap_;
			Core::ExternalAllocator allocator_;
			i32 numAllocs_ = 0;
		};

		/// device to use.
		ComPtr<ID3D12Device> d3dDevice_;
		/// Heap type we are allocating for.
		D3D12_DESCRIPTOR_HEAP_TYPE heapType_ = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		/// Heap flags.
		D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags_ = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		/// Minimum resource block size.
		i32 blockSize_ = 0;
		/// Increment size for handles.
		i32 handleIncrementSize_ = 0;
		/// Debug name.
		const char* debugName_ = nullptr;
		/// Mutex to allow multiple threads to allocate at the same time.
		Core::Mutex allocMutex_;
		/// Blocks in pool.
		Core::Vector<DescriptorBlock> blocks_;
	};


} // namespace GPU
