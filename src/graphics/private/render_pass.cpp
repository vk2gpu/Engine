#include "graphics/render_pass.h"
#include "graphics/private/render_pass_impl.h"

namespace Graphics
{
	RenderPass::RenderPass(RenderGraphBuilder&) 
	{
		// TODO: Allocate this on the render graph.
		impl_ = new RenderPassImpl;
	}

	RenderPass::~RenderPass()
	{
		delete impl_;
		impl_ = nullptr;
	}

#if 0
	void RenderPass::AddInput(RenderGraphResource res)
	{
		return impl_->AddInput(res);
	}

	void RenderPass::AddOutput(RenderGraphResource res)
	{
		return impl_->AddOutput(res);
	}
#endif

	Core::ArrayView<const RenderGraphResource> RenderPass::GetInputs() const
	{
		return impl_->GetInputs();
	}

	Core::ArrayView<const RenderGraphResource> RenderPass::GetOutputs() const
	{
		return impl_->GetOutputs();
	}

} // namespace Graphics