#pragma once

#include "gpu_d3d12/d3d12_types.h"
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
		ResourceVector() = default;
		~ResourceVector() = default;

		TYPE& operator[](i32 idx)
		{
			DBG_ASSERT(idx < Handle::MAX_INDEX);
			if(idx >= storage_.size())
				storage_.resize(Core::PotRoundUp(idx + 1, 32));
			return storage_[idx];
		}

		const TYPE& operator[](i32 idx) const
		{
			DBG_ASSERT(idx < Handle::MAX_INDEX);
			if(idx >= storage_.size())
				storage_.resize(Core::PotRoundUp(idx + 1, 32));
			return storage_[idx];
		}

		i32 size() const { return storage_.size(); }

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

	struct D3D12SwapChain
	{
		ComPtr<IDXGISwapChain3> swapChain_;
		Core::Vector<D3D12Resource> textures_;
	};

	struct D3D12Shader
	{
		u8* byteCode_ = nullptr;
		u32 byteCodeSize_ = 0;
	};

	struct D3D12SamplerState
	{
		D3D12_SAMPLER_DESC desc_;
	};

	struct D3D12GraphicsPipelineState
	{
		RootSignatureType rootSignature_ = RootSignatureType::INVALID;
		u32 stencilRef_ = 0; // TODO: Make part of draw?
		ComPtr<ID3D12PipelineState> pipelineState_;
	};

	struct D3D12ComputePipelineState
	{
		RootSignatureType rootSignature_ = RootSignatureType::INVALID;
		ComPtr<ID3D12PipelineState> pipelineState_;
	};
}
