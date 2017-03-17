#pragma once

#include "gpu_d3d12/d3d12_types.h"
#include "gpu_d3d12/d3d12_resources.h"

#include "gpu/resources.h"
#include "core/map.h"

namespace Core
{
	inline u32 Hash(u32 input, const GPU::D3D12Resource* data) { return HashCRC32(input, &data, sizeof(data)); }
}

namespace GPU
{

	struct D3D12CompileContext
	{
		Core::Map<const D3D12Resource*, D3D12_RESOURCE_STATES> stateTracker_;
		Core::Map<const D3D12Resource*, D3D12_RESOURCE_BARRIER> pendingBarriers_;
		Core::Vector<D3D12_RESOURCE_BARRIER> barriers_;

		ID3D12GraphicsCommandList* d3dCommandList_ = nullptr;

		/**
		 * Add resource transition.
		 * @param resource Resource to transition.
		 * @param state States to transition.
		 */
		void AddTransition(const D3D12Resource* resource, D3D12_RESOURCE_STATES state);

		/**
		 * Flush resource transitions.
		 */
		void FlushTransitions();

		/**
		 * Restore default state of resources.
		 */
		void RestoreDefault();
	};
} // namespace GPU
