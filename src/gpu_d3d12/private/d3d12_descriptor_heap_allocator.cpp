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

	D3D12DescriptorAllocation D3D12DescriptorHeapAllocator::Alloc(i32 numDescriptors)
	{
		Core::ScopedMutex lock(allocMutex_);

		for(i32 i = 0; i < blocks_.size(); ++i)
		{
			auto& block = blocks_[i];

			auto allocId = block.allocator_.AllocRange(numDescriptors);
			if(auto alloc = block.allocator_.GetAlloc(allocId))
			{
				D3D12DescriptorAllocation retVal;
				retVal.d3dDescriptorHeap_ = block.d3dDescriptorHeap_;
				retVal.offset_ = alloc.offset_;
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

	D3D12DescriptorAllocation D3D12DescriptorHeapAllocator::Alloc(i32 numCBV, i32 numSRV, i32 numUAV)
	{
		auto alloc = Alloc(MAX_CBV_BINDINGS + MAX_SRV_BINDINGS + MAX_UAV_BINDINGS);
		i32 offset = alloc.offset_;
		ClearRange(alloc.d3dDescriptorHeap_.Get(), DescriptorHeapSubType::CBV, offset, MAX_CBV_BINDINGS);
		offset += MAX_CBV_BINDINGS;
		ClearRange(alloc.d3dDescriptorHeap_.Get(), DescriptorHeapSubType::SRV, offset, MAX_SRV_BINDINGS);
		offset += MAX_SRV_BINDINGS;
		ClearRange(alloc.d3dDescriptorHeap_.Get(), DescriptorHeapSubType::UAV, offset, MAX_UAV_BINDINGS);
		return alloc;
	}

	void D3D12DescriptorHeapAllocator::Free(D3D12DescriptorAllocation alloc)
	{
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

		ClearRange(block.d3dDescriptorHeap_.Get(), DescriptorHeapSubType::INVALID, 0, blockSize_);

		blocks_.emplace_back(std::move(block));
	}

	void D3D12DescriptorHeapAllocator::ClearRange(
	    ID3D12DescriptorHeap* d3dDescriptorHeap, DescriptorHeapSubType subType, i32 offset, i32 numDescriptors)
	{
		auto descriptorSize = d3dDevice_->GetDescriptorHandleIncrementSize(heapType_);
		D3D12_CPU_DESCRIPTOR_HANDLE handle = d3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += offset * descriptorSize;
		for(i32 i = offset; i < (offset + numDescriptors); ++i)
		{
			if(subType == DescriptorHeapSubType::INVALID)
			{
				switch(heapType_)
				{
				case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
				{
					D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
					d3dDevice_->CreateConstantBufferView(&desc, handle);
					break;
				}
				case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
				{
					D3D12_SAMPLER_DESC desc = {};
					// Setup easy to spot when debugging, but valid default.
					desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
					desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
					desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
					desc.BorderColor[0] = 1.0f;
					desc.BorderColor[1] = 2.0f;
					desc.BorderColor[2] = 3.0f;
					desc.BorderColor[3] = 4.0f;
					d3dDevice_->CreateSampler(&desc, handle);
					break;
				}
				case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
				case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
					// Don't need to clear these ranges.
					break;
				default:
					DBG_BREAK;
					break;
				};
			}
			else
			{
				switch(subType)
				{
				case DescriptorHeapSubType::CBV:
				{
					D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
					d3dDevice_->CreateConstantBufferView(&desc, handle);
					break;
				}
				case DescriptorHeapSubType::SRV:
				{
					D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
					desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
					desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
					desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
					    D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0, D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
					    D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0, D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0);
					d3dDevice_->CreateShaderResourceView(nullptr, &desc, handle);
					break;
				}
				case DescriptorHeapSubType::UAV:
				{
					D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
					desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
					desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
					d3dDevice_->CreateUnorderedAccessView(nullptr, nullptr, &desc, handle);
					break;
				}
				default:
					DBG_BREAK;
					break;
				};
			}

			// Advance handle.
			handle.ptr += descriptorSize;
		}
	}

} // namespace GPU
