#pragma once

#include "gpu_d3d12/d3d12_types.h"
#include "core/concurrency.h"
#include "core/external_allocator.h"
#include "core/vector.h"

namespace GPU
{
	/**
	 * Descriptor heap allocation.
	 */
	struct D3D12DescriptorAllocation
	{
		/// Descriptor heap allocator.
		class D3D12DescriptorHeapAllocator* allocator_ = nullptr;
		/// Offset in descriptor heap.
		i32 offset_ = 0;
		/// Size of allocation.
		i32 size_ = 0;
		/// Allocation id.
		u32 allocId_ = 0;

		/**
		 * Do we have debug data?
		 */
		bool HaveDebugData() const { return true; }

		/**
		 * Get debug data for descriptor allocation.
		 */
		D3D12DescriptorDebugData& GetDebugData(i32 offset);
		const D3D12DescriptorDebugData& GetDebugData(i32 offset) const;
		Core::ArrayView<D3D12DescriptorDebugData> GetDebugDataRange(i32 offset, i32 num);

		/**
		 * Get CPU handle at @a offset within allocation.
		 */
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(i32 offset) const;

		/**
		 * Get GPU handle at @a offset within allocation.
		 */
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(i32 offset) const;

		/**
		 * Get descriptor heap.
		 */
		ID3D12DescriptorHeap* GetDescriptorHeap() const;
	};

	/**
 	 * Descriptor heap allocator.
	 */
	class D3D12DescriptorHeapAllocator
	{
	public:
		D3D12DescriptorHeapAllocator(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType,
		    D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, i32 blockSize, const char* debugName);
		~D3D12DescriptorHeapAllocator();

		D3D12DescriptorAllocation Alloc(i32 size);

		void Free(D3D12DescriptorAllocation alloc);

		i32 GetHandleIncrementSize() const { return handleIncrementSize_; }

		/// Get debug data for descriptor allocation.
		D3D12DescriptorDebugData& GetDebugData(const D3D12DescriptorAllocation& alloc, i32 offset);
		const D3D12DescriptorDebugData& GetDebugData(const D3D12DescriptorAllocation& alloc, i32 offset) const;
		Core::ArrayView<D3D12DescriptorDebugData> GetDebugDataRange(
		    const D3D12DescriptorAllocation& alloc, i32 offset, i32 num);

		/// Get CPU handle for descriptor allocation.
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(const D3D12DescriptorAllocation& alloc, i32 offset) const;

		/// Get GPU handle for descriptor allocation.
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(const D3D12DescriptorAllocation& alloc, i32 offset) const;

		/// Get descriptor heap for allocation.
		ID3D12DescriptorHeap* GetDescriptorHeap(const D3D12DescriptorAllocation& alloc) const;

	private:
		D3D12DescriptorHeapAllocator(const D3D12DescriptorHeapAllocator&) = delete;

		void AddBlock();

		struct DescriptorBlock
		{
			DescriptorBlock(i32 size, i32 maxAllocs)
			    : allocator_(size, maxAllocs)
#if ENABLE_DESCRIPTOR_DEBUG_DATA
			    , debugData_(size)
#endif
			{
			}

			DescriptorBlock(const DescriptorBlock&) = delete;
			DescriptorBlock(DescriptorBlock&&) = default;

			ComPtr<ID3D12DescriptorHeap> d3dDescriptorHeap_;
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle_;
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle_;

			Core::ExternalAllocator allocator_;
			i32 numAllocs_ = 0;

#if ENABLE_DESCRIPTOR_DEBUG_DATA
			Core::Vector<D3D12DescriptorDebugData> debugData_;
#else
			Core::ArrayView<D3D12DescriptorDebugData> debugData_;
#endif
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

#if ENABLE_DESCRIPTOR_DEBUG_DATA
	inline D3D12DescriptorDebugData& D3D12DescriptorHeapAllocator::GetDebugData(
	    const D3D12DescriptorAllocation& alloc, i32 offset)
	{
		auto& block = blocks_[alloc.allocId_ >> 16];
		return block.debugData_[alloc.offset_ + offset];
	}

	inline Core::ArrayView<D3D12DescriptorDebugData> D3D12DescriptorHeapAllocator::GetDebugDataRange(
	    const D3D12DescriptorAllocation& alloc, i32 offset, i32 num)
	{
		auto& block = blocks_[alloc.allocId_ >> 16];
		return Core::ArrayView<D3D12DescriptorDebugData>(
		    block.debugData_.data() + alloc.offset_ + offset, block.debugData_.data() + alloc.offset_ + offset + num);
	}

	inline const D3D12DescriptorDebugData& D3D12DescriptorHeapAllocator::GetDebugData(
	    const D3D12DescriptorAllocation& alloc, i32 offset) const
	{
		auto& block = blocks_[alloc.allocId_ >> 16];
		return block.debugData_[alloc.offset_ + offset];
	}
#else
	inline D3D12DescriptorDebugData& D3D12DescriptorHeapAllocator::GetDebugData(
	    const D3D12DescriptorAllocation& alloc, i32 offset)
	{
		static D3D12DescriptorDebugData debugData;
		return debugData;
	}

	inline Core::ArrayView<D3D12DescriptorDebugData> D3D12DescriptorHeapAllocator::GetDebugDataRange(
	    const D3D12DescriptorAllocation& alloc, i32 offset, i32 num)
	{
		return Core::ArrayView<D3D12DescriptorDebugData>();
	}

	inline const D3D12DescriptorDebugData& D3D12DescriptorHeapAllocator::GetDebugData(
	    const D3D12DescriptorAllocation& alloc, i32 offset) const
	{
		static const D3D12DescriptorDebugData debugData;
		return debugData;
	}
#endif // ENABLE_DESCRIPTOR_DEBUG_DATA

	inline D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeapAllocator::GetCPUHandle(
	    const D3D12DescriptorAllocation& alloc, i32 offset) const
	{
		const auto& block = blocks_[alloc.allocId_ >> 16];

		D3D12_CPU_DESCRIPTOR_HANDLE retVal = block.cpuHandle_;
		retVal.ptr += (alloc.offset_ + offset) * handleIncrementSize_;
		return retVal;
	}

	inline D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeapAllocator::GetGPUHandle(
	    const D3D12DescriptorAllocation& alloc, i32 offset) const
	{
		const auto& block = blocks_[alloc.allocId_ >> 16];

		D3D12_GPU_DESCRIPTOR_HANDLE retVal = block.gpuHandle_;
		retVal.ptr += (alloc.offset_ + offset) * handleIncrementSize_;
		return retVal;
	}

	inline ID3D12DescriptorHeap* D3D12DescriptorHeapAllocator::GetDescriptorHeap(
	    const D3D12DescriptorAllocation& alloc) const
	{
		const auto& block = blocks_[alloc.allocId_ >> 16];
		return block.d3dDescriptorHeap_.Get();
	}

#if ENABLE_DESCRIPTOR_DEBUG_DATA
	inline D3D12DescriptorDebugData& D3D12DescriptorAllocation::GetDebugData(i32 offset)
	{
		return allocator_->GetDebugData(*this, offset);
	}

	inline const D3D12DescriptorDebugData& D3D12DescriptorAllocation::GetDebugData(i32 offset) const
	{
		return allocator_->GetDebugData(*this, offset);
	}

	inline Core::ArrayView<D3D12DescriptorDebugData> D3D12DescriptorAllocation::GetDebugDataRange(i32 offset, i32 num)
	{
		return allocator_->GetDebugDataRange(*this, offset, num);
	}
#else
	inline D3D12DescriptorDebugData& D3D12DescriptorAllocation::GetDebugData(i32 offset)
	{
		static D3D12DescriptorDebugData debugData;
		return debugData;
	}

	inline const D3D12DescriptorDebugData& D3D12DescriptorAllocation::GetDebugData(i32 offset) const
	{
		static const D3D12DescriptorDebugData debugData;
		return debugData;
	}

	inline Core::ArrayView<D3D12DescriptorDebugData> D3D12DescriptorAllocation::GetDebugDataRange(i32 offset, i32 num)
	{
		return Core::ArrayView<D3D12DescriptorDebugData>();
	}
#endif // ENABLE_DESCRIPTOR_DEBUG_DATA

	inline D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorAllocation::GetCPUHandle(i32 offset) const
	{
		return allocator_->GetCPUHandle(*this, offset);
	}

	inline D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorAllocation::GetGPUHandle(i32 offset) const
	{
		return allocator_->GetGPUHandle(*this, offset);
	}

	inline ID3D12DescriptorHeap* D3D12DescriptorAllocation::GetDescriptorHeap() const
	{
		return allocator_->GetDescriptorHeap(*this);
	}

} // namespace GPU
