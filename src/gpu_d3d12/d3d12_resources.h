#pragma once

#include "gpu_d3d12/d3d12_types.h"
#include "gpu_d3d12/d3d12_descriptor_heap_allocator.h"
#include "gpu/resources.h"
#include "core/array.h"
#include "core/misc.h"
#include "core/vector.h"
#include <utility>

namespace GPU
{
	/**
	 * Resource vector.
	 * Just a wrapper around Core::Vector that will automatically
	 * resize when inserting resources.
	 */
	template<typename TYPE>
	class ResourceVector
	{
	public:
		static const i32 RESIZE_ROUND_UP = 32;
		ResourceVector() = default;
		~ResourceVector() = default;

		TYPE& operator[](i32 idx)
		{
			DBG_ASSERT(idx < Handle::MAX_INDEX);
			if(idx >= storage_.size())
				storage_.resize(Core::PotRoundUp(idx + 1, RESIZE_ROUND_UP));
			return storage_[idx];
		}

		const TYPE& operator[](i32 idx) const
		{
			DBG_ASSERT(idx < Handle::MAX_INDEX);
			DBG_ASSERT(idx < storage_.size());
			return storage_[idx];
		}

		i32 size() const { return storage_.size(); }
		void clear() { storage_.clear(); }

	private:
		Core::Vector<TYPE> storage_;
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

	struct D3D12SamplerState
	{
		D3D12_SAMPLER_DESC desc_ = {};
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
		RootSignatureType rootSignature_ = RootSignatureType::INVALID;
		ComPtr<ID3D12PipelineState> pipelineState_;
		D3D12DescriptorAllocation srvs_;
		D3D12DescriptorAllocation uavs_;
		D3D12DescriptorAllocation cbvs_;
		D3D12DescriptorAllocation samplers_;
	};

	struct D3D12DrawBindingSet
	{
		DrawBindingSetDesc desc_;
		Core::Array<D3D12Resource, MAX_VERTEX_STREAMS> vbResources_;
		D3D12Resource ibResource_;
		Core::Array<D3D12_VERTEX_BUFFER_VIEW, MAX_VERTEX_STREAMS> vbs_;
		D3D12_INDEX_BUFFER_VIEW ib_;
	};

	struct D3D12FrameBindingSet
	{
		D3D12DescriptorAllocation rtvs_;
		D3D12DescriptorAllocation dsv_;
		Core::Vector<D3D12Resource*> rtvResources_;
		Core::Vector<D3D12Resource*> dsvResources_;
		FrameBindingSetDesc desc_;
		D3D12SwapChain* swapChain_ = nullptr;
		i32 numRTs_ = 0;
		i32 numBuffers_ = 1;
		Viewport viewport_;
		ScissorRect scissorRect_;
	};
}
