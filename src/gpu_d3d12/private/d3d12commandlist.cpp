#include "gpu_d3d12/d3d12commandlist.h"
#include "gpu_d3d12/d3d12device.h"
#include "core/debug.h"

namespace GPU
{
	D3D12CommandList::D3D12CommandList(D3D12Device& device, u32 nodeMask, D3D12_COMMAND_LIST_TYPE type)
	    : device_(device)
	{
		HRESULT hr = S_OK;
		CHECK_D3D(hr = device.device_->CreateCommandAllocator(
		              type, IID_ID3D12CommandAllocator, (void**)commandAllocator_.GetAddressOf()));
		CHECK_D3D(hr = device.device_->CreateCommandList(nodeMask, type, commandAllocator_.Get(), nullptr,
		              IID_ID3D12CommandList, (void**)commandList_.GetAddressOf()));
	}

	D3D12CommandList::~D3D12CommandList() {}

} // namespace GPU
