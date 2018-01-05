#pragma once

#include "gpu_d3d12/d3d12_types.h"
#include "gpu_d3d12/d3d12_descriptor_heap_allocator.h"
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
		D3D12LinearDescriptorAllocator(D3D12DescriptorHeapAllocator& allocator, i32 blockSize);
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

	private:
		D3D12LinearDescriptorAllocator(const D3D12LinearDescriptorAllocator&) = delete;

		/// Allocator.
		D3D12DescriptorHeapAllocator& allocator_;
		/// Base allocation.
		D3D12DescriptorAllocation alloc_;
		/// Device.
		ComPtr<ID3D12Device> d3dDevice_;
		/// Heap type.
		D3D12_DESCRIPTOR_HEAP_TYPE heapType_;
		/// Current allocation offset.
		volatile i32 allocOffset_ = 0;
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
