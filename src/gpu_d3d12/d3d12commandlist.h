#pragma once

#include "gpu_d3d12/dll.h"
#include "gpu_d3d12/d3d12types.h"
#include "gpu/command_list.h"
#include "core/vector.h"

namespace GPU
{
	class D3D12Device;

	class D3D12CommandList
	{
	public:
		D3D12CommandList(D3D12Device& device, u32 nodeMask, D3D12_COMMAND_LIST_TYPE type);
		~D3D12CommandList();

		D3D12Device& device_;
		D3D12_COMMAND_LIST_TYPE type_;
		Core::Vector<ComPtr<ID3D12CommandAllocator>> d3dCommandAllocators_;
		ComPtr<ID3D12GraphicsCommandList> d3dCommandList_;

		struct ResourceStateTracking
		{
			ComPtr<ID3D12Resource> d3dResource_;
			D3D12_RESOURCE_STATES currentState_;
			D3D12_RESOURCE_STATES defaultState_;
		};

		Core::Vector<ResourceStateTracking> swapchainResources_;
		Core::Vector<ResourceStateTracking> textureResources_;
		Core::Vector<ResourceStateTracking> bufferResources_;
	};

} // namespace GPU
