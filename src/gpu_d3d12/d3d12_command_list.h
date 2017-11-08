#pragma once

#include "gpu_d3d12/dll.h"
#include "gpu_d3d12/d3d12_types.h"
#include "gpu/command_list.h"
#include "core/vector.h"

namespace GPU
{
	class D3D12Device;

	class D3D12CommandList
	{
	public:
		D3D12CommandList(D3D12Device& d3dDevice, u32 nodeMask, D3D12_COMMAND_LIST_TYPE type);
		~D3D12CommandList();

		/**
		 * Get command list.
		 * Will open command list if not currently open.
		 */
		ID3D12GraphicsCommandList* Get();

		/**
		 * Open a command list for use.
		 */
		ID3D12GraphicsCommandList* Open();

		/**
		 * Close command list.
		 */
		ErrorCode Close();

		/**
		 * Submit command list to queue.
		 */
		ErrorCode Submit(ID3D12CommandQueue* d3dCommandQueue);

		/**
		 * Signal for next list.
		 */
		ErrorCode SignalNext(ID3D12CommandQueue* d3dCommandQueue);

		D3D12Device& device_;
		D3D12_COMMAND_LIST_TYPE type_;
		Core::Vector<ComPtr<ID3D12CommandAllocator>> d3dCommandAllocators_;
		ComPtr<ID3D12GraphicsCommandList> d3dCommandList_;
		i32 listCount_ = MAX_GPU_FRAMES;
		i32 listIdx_ = 0;
		bool isOpen_ = false;

		/// Fence to wait on pending command lists to finish execution.
		HANDLE fenceEvent_;
		ComPtr<ID3D12Fence> d3dFence_;
	};

} // namespace GPU
