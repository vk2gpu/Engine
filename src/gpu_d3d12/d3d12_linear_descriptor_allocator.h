#pragma once

#include "gpu_d3d12/d3d12_types.h"
#include "core/array.h"
#include "core/concurrency.h"
#include "core/vector.h"

namespace GPU
{
	/**
	 * General purpose descriptor allocator.
	 */
	class D3D12LinearDescriptorAllocator
	{
	public:
		D3D12LinearDescriptorAllocator(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType,
		    D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, i32 blockSize, const char* debugName);
		~D3D12LinearDescriptorAllocator();

		/**
		 * Allocate @a num descriptors of subtype @a subType.
		 */
		D3D12DescriptorAllocation Alloc(i32 num, DescriptorHeapSubType subType);

		/**
		 * Create a copy of a set of descriptors.
		 * @param src Where to copy from.
		 * @param size Size of allocation.
		 */
		D3D12DescriptorAllocation Copy(const D3D12DescriptorAllocation& src, i32 size, DescriptorHeapSubType subType);

		/**
		 * Reset allocator.
		 */
		void Reset();

		/**
		 * Get descriptor heap this allocates from.
		 */
		ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() const { return d3dDescriptorHeap_; }

		/**
		 * Get handle increment size;
		 */
		i32 GetHandleIncrementSize() const { return handleIncrementSize_; }

		/**
		 * Get GPU handle for base of allocator.
		 */
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const
		{
			return d3dDescriptorHeap_->GetGPUDescriptorHandleForHeapStart();
		}

	private:
		D3D12LinearDescriptorAllocator(const D3D12LinearDescriptorAllocator&) = delete;

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
		/// Current allocation offset.
		volatile i32 allocOffset_ = 0;
		/// Heap to allocate from.
		ComPtr<ID3D12DescriptorHeap> d3dDescriptorHeap_;
		/// Debug data.
		Core::Vector<D3D12DescriptorDebugData> debugData_;
	};

	/**
	 * Descriptor suballocator to allocate large chunks of sequential descriptors
	 * with the appropriate padding for Tier 1 hardware.
	 */
	class D3D12LinearDescriptorSubAllocator
	{
	public:
		D3D12LinearDescriptorSubAllocator(
		    D3D12LinearDescriptorAllocator& allocator, DescriptorHeapSubType subType, i32 blockSize);
		~D3D12LinearDescriptorSubAllocator();

		/**
		 * Allocate @a num descriptors with (@a padding - @a num) valid descriptors immediately after.
		 */
		D3D12DescriptorAllocation Alloc(i32 num, i32 padding);

		/**
		 * Reset allocator.
		 */
		void Reset();

	private:
		D3D12LinearDescriptorAllocator& allocator_;
		DescriptorHeapSubType subType_;
		i32 blockSize_ = 0;

		Core::Mutex mutex_; // TEMP: This should only be used from a single thread later.
		D3D12DescriptorAllocation alloc_;
		i32 allocOffset_ = 0;
	};


} // namespace GPU
