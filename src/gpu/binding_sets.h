#pragma once

#include "gpu/dll.h"
#include "gpu/types.h"
#include "gpu/format.h"
#include "gpu/resources.h"
#include "gpu/sampler_state.h"

namespace GPU
{
	/**
	 * Binding for a shader resource view.
	 */
	struct BindingSRV
	{
		Handle resource_;
		Format format_ = Format::INVALID;
		ViewDimension dimension_ = ViewDimension::INVALID;
		i64 mostDetailedMip_FirstElement_ = 0;
		i64 mipLevels_NumElements_ = 0;
		i64 arraySize_ = 0;
	};

	/**
	 * Binding for an unordered access view.
	 */
	struct BindingUAV
	{
		Handle resource_;
		Format format_ = Format::INVALID;
		ViewDimension dimension_ = ViewDimension::INVALID;
		i64 mipSlice_FirstElement_ = 0;
		i64 firstArraySlice_NumElements_ = 0;
		i64 arraySize_ = 0;
	};

	/**
	 * Binding for a constant buffer view.
	 */
	struct BindingCBV
	{
		Handle resource_;
		i32 offset_ = 0;
		i32 size_ = 0;
	};

	/**
	 * Binding for a sampler.
	 */
	struct BindingSampler
	{
		Handle resource_;
	};

	/**
	 * Pipeline binding set.
	 * Common parameters shared by both graphics and compute
	 * pipeline states.
	 */
	struct PipelineBindingSetDesc
	{
		i32 numSRVs_ = 0;
		i32 numUAVs_ = 0;
		i32 numCBs_ = 0;
		i32 numSamplers_ = 0;
		BindingSRV* srvs_ = nullptr;
		BindingSRV* uavs_ = nullptr;
		BindingSRV* cbvs_ = nullptr;
		BindingSampler* samplers_ = nullptr;
	};
};
