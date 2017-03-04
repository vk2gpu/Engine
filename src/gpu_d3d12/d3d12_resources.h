#pragma once

#include "gpu_d3d12/d3d12_types.h"
#include "core/misc.h"
#include "core/vector.h"

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

		i32 size() const
		{
			return storage_.size();
		}

	private:
		Core::Vector<TYPE> storage_;
	};


	struct D3D12Resource
	{
		ComPtr<ID3D12Resource> resource_;
		D3D12_RESOURCE_STATES supportedStates_ = D3D12_RESOURCE_STATE_COMMON;
		D3D12_RESOURCE_STATES defaultState_ = D3D12_RESOURCE_STATE_COMMON;
	};

	struct D3D12SwapChainResource
	{
		ComPtr<IDXGISwapChain3> swapChain_;
		Core::Vector<D3D12Resource> textures_;
	};
}
