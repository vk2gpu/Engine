#pragma once

#include "core/array.h"
#include "core/array_view.h"
#include "graphics/render_resources.h"

namespace Graphics
{
	struct RenderPassImpl
	{
		static constexpr i32 MAX_INPUTS = 16;
		static constexpr i32 MAX_OUTPUTS = 16;

		Core::Array<RenderGraphResource, MAX_INPUTS> inputs_;
		Core::Array<RenderGraphResource, MAX_OUTPUTS> outputs_;

		i32 numInputs_ = 0;
		i32 numOutputs_ = 0;

		void AddInput(RenderGraphResource res)
		{
			DBG_ASSERT(numInputs_ < inputs_.size());
			if(numInputs_ < inputs_.size())
				inputs_[numInputs_++] = res;
		}

		void AddOutput(RenderGraphResource res)
		{
			DBG_ASSERT(numOutputs_ < outputs_.size());
			if(numOutputs_ < outputs_.size())
				outputs_[numOutputs_++] = res;
		}

		Core::ArrayView<const RenderGraphResource> GetInputs() const
		{
			return Core::ArrayView<const RenderGraphResource>(inputs_.data(), numInputs_);
		}

		Core::ArrayView<const RenderGraphResource> GetOutputs() const
		{
			return Core::ArrayView<const RenderGraphResource>(outputs_.data(), numOutputs_);
		}
	};
} // namespace Graphics