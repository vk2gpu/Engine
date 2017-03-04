#include "gpu_d3d12/d3d12_command_list.h"
#include "gpu_d3d12/d3d12_device.h"
#include "core/debug.h"

#include <utility>

namespace GPU
{
	D3D12CommandList::D3D12CommandList(D3D12Device& device, u32 nodeMask, D3D12_COMMAND_LIST_TYPE type)
	    : device_(device)
	{
		HRESULT hr = S_OK;

		d3dCommandAllocators_.resize(MAX_GPU_FRAMES);
		for(i32 i = 0; i < MAX_GPU_FRAMES; ++i)
		{
			CHECK_D3D(hr = device.device_->CreateCommandAllocator(
						  type, IID_ID3D12CommandAllocator, (void**)d3dCommandAllocators_[i].GetAddressOf()));
		}

		type_ = type;
		CHECK_D3D(hr = device.device_->CreateCommandList(nodeMask, type, d3dCommandAllocators_[0].Get(), nullptr,
						IID_ID3D12GraphicsCommandList, (void**)d3dCommandList_.GetAddressOf()));
		CHECK_D3D(hr = d3dCommandList_->Close());
	}

	D3D12CommandList::~D3D12CommandList() {}

} // namespace GPU
