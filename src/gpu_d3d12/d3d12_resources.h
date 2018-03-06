#pragma once

#include "gpu_d3d12/d3d12_types.h"
#include "gpu_d3d12/d3d12_descriptor_heap_allocator.h"
#include "gpu/resources.h"
#include "core/array.h"
#include "core/misc.h"
#include "core/vector.h"
#include <utility>
#include <cstdalign>

namespace GPU
{
	/**
	 * Resource read wrapper.
	 * This is used to synchronize CPU access to the resource object.
	 * by managing a lightweight rw lock.
	 */
	template<typename TYPE>
	struct ResourceRead final
	{
		ResourceRead() = default;

		ResourceRead(const Core::RWLock& lock, const TYPE& res)
		    : lock_(&lock)
		    , res_(&res)
		{
			lock_->BeginRead();
		}

		~ResourceRead()
		{
			if(lock_)
				lock_->EndRead();
		}

		template<typename TYPE2>
		ResourceRead(ResourceRead<TYPE2>&& other, const TYPE& res)
		    : lock_(nullptr)
		    , res_(&res)
		{
			std::swap(lock_, other.lock_);
		}

		ResourceRead& operator=(ResourceRead&& other)
		{
			std::swap(lock_, other.lock_);
			std::swap(res_, other.res_);
			return *this;
		}

		ResourceRead(const ResourceRead&) = delete;
		ResourceRead(ResourceRead&&) = default;

		const TYPE& operator*() const { return *res_; }
		const TYPE* operator->() const { return res_; }
		explicit operator bool() const { return !!res_; }

	private:
		template<typename>
		friend struct ResourceRead;

		const Core::RWLock* lock_ = nullptr;
		const TYPE* res_ = nullptr;
	};

	/**
	 * Resource write wrapper.
	 * This is used to synchronize CPU access to the resource object
	 * by managing a lightweight rw lock.
	 */
	template<typename TYPE>
	struct ResourceWrite final
	{
		ResourceWrite() = default;

		ResourceWrite(Core::RWLock& lock, TYPE& res)
		    : lock_(&lock)
		    , res_(&res)
		{
			lock_->BeginWrite();
		}

		~ResourceWrite()
		{
			if(lock_)
				lock_->EndWrite();
		}

		template<typename TYPE2>
		ResourceWrite(ResourceWrite<TYPE2>&& other, TYPE& res)
		    : lock_(nullptr)
		    , res_(&res)
		{
			std::swap(lock_, other.lock_);
		}

		ResourceWrite(const ResourceWrite&) = delete;
		ResourceWrite(ResourceWrite&&) = default;

		TYPE& operator*() { return *res_; }
		TYPE* operator->() { return res_; }
		explicit operator bool() const { return !!res_; }

	private:
		template<typename>
		friend struct ResourceWrite;

		Core::RWLock* lock_ = nullptr;
		TYPE* res_ = nullptr;
	};

	/**
	 * Resource pool.
	 */
	template<typename TYPE>
	class ResourcePool
	{
	public:
		// 256 resources per block.
		static const i32 INDEX_BITS = 8;
		static const i32 BLOCK_SIZE = 1 << INDEX_BITS;

		ResourcePool(const char* name)
		    : name_(name)
		{
			DBG_LOG("GPU::ResourcePool<%s>: Size: %lld, Stored Size: %lld\n", name_, sizeof(TYPE), sizeof(Resource));
		}

		~ResourcePool()
		{
			for(auto& rb : rbs_)
				Core::GeneralAllocator().Delete(rb);
		}

		ResourceRead<TYPE> Read(Handle handle) const
		{
			Core::ScopedReadLock lock(blockLock_);
			const i32 idx = handle.GetIndex();
			const i32 blockIdx = idx >> INDEX_BITS;
			const i32 resourceIdx = idx & (BLOCK_SIZE - 1);

			auto& res = rbs_[blockIdx]->resources_[resourceIdx];

			return ResourceRead<TYPE>(res.lock_, res.res_);
		}

		ResourceWrite<TYPE> Write(Handle handle)
		{
			Core::ScopedWriteLock lock(blockLock_);
			const i32 idx = handle.GetIndex();
			const i32 blockIdx = idx >> INDEX_BITS;
			const i32 resourceIdx = idx & (BLOCK_SIZE - 1);

			// TODO: Change this code to use virtual memory and just commit as required.
			while(blockIdx >= rbs_.size())
			{
				rbs_.push_back(Core::GeneralAllocator().New<Resources>());
			}

			auto& res = rbs_[blockIdx]->resources_[resourceIdx];

			return ResourceWrite<TYPE>(res.lock_, res.res_);
		}

		i32 size() const { return rbs_.size() * BLOCK_SIZE; }

	private:
		struct Resource
		{
			Core::RWLock lock_;
			TYPE res_;
		};

		struct Resources
		{
			Core::Array<Resource, BLOCK_SIZE> resources_;
		};

		Core::RWLock blockLock_;
		Core::Vector<Resources*> rbs_;
		const char* name_ = nullptr;
	};

	class D3D12ScopedResourceBarrier
	{
	public:
		D3D12ScopedResourceBarrier(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, u32 subresource,
		    D3D12_RESOURCE_STATES oldState, D3D12_RESOURCE_STATES newState)
		    : commandList_(commandList)
		{
			barrier_.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier_.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier_.Transition.pResource = resource;
			barrier_.Transition.Subresource = subresource;
			barrier_.Transition.StateBefore = oldState;
			barrier_.Transition.StateAfter = newState;
			commandList_->ResourceBarrier(1, &barrier_);
		}

		~D3D12ScopedResourceBarrier()
		{
			std::swap(barrier_.Transition.StateBefore, barrier_.Transition.StateAfter);
			commandList_->ResourceBarrier(1, &barrier_);
		}

	private:
		ID3D12GraphicsCommandList* commandList_ = nullptr;
		D3D12_RESOURCE_BARRIER barrier_;
	};

	struct D3D12Resource
	{
		ComPtr<ID3D12Resource> resource_;
		i32 numSubResources_ = 0;
		D3D12_RESOURCE_STATES supportedStates_ = D3D12_RESOURCE_STATE_COMMON;
		D3D12_RESOURCE_STATES defaultState_ = D3D12_RESOURCE_STATE_COMMON;
	};

	struct D3D12Buffer : D3D12Resource
	{
		BufferDesc desc_;
	};

	struct D3D12Texture : D3D12Resource
	{
		TextureDesc desc_;
	};

	struct D3D12SwapChain
	{
		ComPtr<IDXGISwapChain3> swapChain_;
		Core::Vector<D3D12Texture> textures_;
		i32 bbIdx_ = 0;
	};

	struct D3D12Shader
	{
		u8* byteCode_ = nullptr;
		u32 byteCodeSize_ = 0;
	};

	struct D3D12SubresourceRange
	{
		const D3D12Resource* resource_ = nullptr;
		i32 firstSubRsc_ = 0;
		i32 numSubRsc_ = 0;

		explicit operator bool() const { return resource_ && numSubRsc_ > 0; }
	};

	struct D3D12GraphicsPipelineState
	{
		RootSignatureType rootSignature_ = RootSignatureType::GRAPHICS;
		u32 stencilRef_ = 0; // TODO: Make part of draw?
		ComPtr<ID3D12PipelineState> pipelineState_;
	};

	struct D3D12ComputePipelineState
	{
		RootSignatureType rootSignature_ = RootSignatureType::COMPUTE;
		ComPtr<ID3D12PipelineState> pipelineState_;
	};

	struct D3D12PipelineBindingSet
	{
		D3D12DescriptorAllocation cbvs_;
		D3D12DescriptorAllocation srvs_;
		D3D12DescriptorAllocation uavs_;
		D3D12DescriptorAllocation samplers_;

		Core::Vector<D3D12SubresourceRange> srvTransitions_;
		Core::Vector<D3D12SubresourceRange> uavTransitions_;
		Core::Vector<D3D12SubresourceRange> cbvTransitions_;

		bool shaderVisible_ = false;
		bool temporary_ = false;
	};

	struct D3D12DrawBindingSet
	{
		DrawBindingSetDesc desc_;
		Core::Array<const D3D12Resource*, MAX_VERTEX_STREAMS> vbResources_ = {};
		const D3D12Resource* ibResource_ = nullptr;
		Core::Array<D3D12_VERTEX_BUFFER_VIEW, MAX_VERTEX_STREAMS> vbs_ = {};
		D3D12_INDEX_BUFFER_VIEW ib_;
	};

	struct D3D12FrameBindingSet
	{
		D3D12DescriptorAllocation rtvs_;
		D3D12DescriptorAllocation dsv_;

		Core::Vector<D3D12SubresourceRange> rtvResources_;
		D3D12SubresourceRange dsvResource_;
		FrameBindingSetDesc desc_;
		const D3D12SwapChain* swapChain_ = nullptr;
		i32 numRTs_ = 0;
		i32 numBuffers_ = 1;
	};

	struct D3D12Fence
	{
		ComPtr<ID3D12Fence> fence_;
		HANDLE event_;
	};
}
