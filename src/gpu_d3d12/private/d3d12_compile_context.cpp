#include "gpu_d3d12/d3d12_compile_context.h"

namespace GPU
{
	void D3D12CompileContext::AddTransition(const D3D12Resource* resource, D3D12_RESOURCE_STATES state)
	{
		DBG_ASSERT(resource);
		DBG_ASSERT(resource->resource_);

		auto stateEntry = stateTracker_.find(resource);
		if(stateEntry == stateTracker_.end())
		{
			stateEntry = stateTracker_.insert(resource, resource->defaultState_);
		}

		if(state != stateEntry->Second_)
		{
			D3D12_RESOURCE_BARRIER barrier;
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = resource->resource_.Get();
			barrier.Transition.Subresource = 0xffffffff; // TODO.
			barrier.Transition.StateBefore = stateEntry->Second_;
			barrier.Transition.StateAfter = state;
			pendingBarriers_.insert(resource, barrier);
			stateEntry->Second_ = state;
		}
	}

	void D3D12CompileContext::FlushTransitions()
	{
		// Copy pending barriers into flat vector.
		for(auto barrier : pendingBarriers_)
		{
			barriers_.push_back(barrier.Second_);
		}
		pendingBarriers_.clear();

		// Perform resource barriers.
		d3dCommandList_->ResourceBarrier(barriers_.size(), barriers_.data());
		barriers_.clear();
	}

	void D3D12CompileContext::RestoreDefault()
	{
		for(auto state : stateTracker_)
		{
			AddTransition(state.First_, state.First_->defaultState_);
		}
		FlushTransitions();
	}

} // namespace GPU
