#pragma once

#include "gpu_d3d12/dll.h"
#include "gpu_d3d12/d3d12types.h"

namespace GPU
{
	class D3D12Device;

	class D3D12CommandList
	{
	public:
		D3D12CommandList(D3D12Device& device, u32 nodeMask, D3D12_COMMAND_LIST_TYPE type);
		~D3D12CommandList();

		D3D12Device& device_;
		ComPtr<ID3D12CommandAllocator> commandAllocator_;
		ComPtr<ID3D12CommandList> commandList_;
	};

} // namespace GPU
