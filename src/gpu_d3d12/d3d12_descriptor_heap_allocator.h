#pragma once

#include "gpu_d3d12/d3d12_types.h"
#include "core/concurrency.h"
#include "core/vector.h"

namespace GPU
{
	struct D3D12DescriptorAllocation
	{
		/// Descriptor heap we're pointing to.
		ComPtr<ID3D12DescriptorHeap> d3dDescriptorHeap_;
		/// Offset in descriptor heap.
		i32 offset_ = 0;
		/// Block index.
		i32 blockIdx_ = 0;
	};


	class D3D12DescriptorHeapAllocator
	{
	public:
		D3D12DescriptorHeapAllocator(
		    ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, i32 blockSize, const char* debugName);
		~D3D12DescriptorHeapAllocator();

		D3D12DescriptorAllocation Alloc(i32 numDescriptors);
		void Free(D3D12DescriptorAllocation alloc);

	private:
		D3D12DescriptorHeapAllocator(const D3D12DescriptorHeapAllocator&) = delete;

		void AddBlock();
		void ConsolodateAllocations();

		void ClearRange(ID3D12DescriptorHeap* d3dDescriptorHeap, i32 offset, i32 numDescriptors);

		struct DescriptorBlock
		{
			ComPtr<ID3D12DescriptorHeap> d3dDescriptorHeap_;

			struct Allocation
			{
				i32 offset_;
				i32 size_;
			};

			Core::Vector<Allocation> freeAllocations_;
			Core::Vector<Allocation> usedAllocations_;
		};

		/// device to use.
		ComPtr<ID3D12Device> d3dDevice_;
		/// Heap type we are allocating for.
		D3D12_DESCRIPTOR_HEAP_TYPE heapType_ = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		/// Heap flags.
		D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags_ = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		/// Minimum resource block size.
		i32 blockSize_ = 0;
		/// Debug name.
		const char* debugName_ = nullptr;
		/// Mutex to allow multiple threads to allocate at the same time.
		Core::Mutex allocMutex_;
		/// Blocks in pool.
		Core::Vector<DescriptorBlock> blocks_;
	};


} // namespace GPU
