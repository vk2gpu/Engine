#include "graphics/render_pass.h"
#include "graphics/private/render_pass_impl.h"

#include "graphics/render_graph.h"

namespace Graphics
{
	RenderPass::RenderPass(RenderGraphBuilder& builder)
	{
		impl_ = new (builder.Alloc<RenderPassImpl>()) RenderPassImpl;
	}

	RenderPass::~RenderPass()
	{
		impl_->~RenderPassImpl();
		impl_ = nullptr;
	}

	Core::ArrayView<const RenderGraphResource> RenderPass::GetInputs() const { return impl_->GetInputs(); }

	Core::ArrayView<const RenderGraphResource> RenderPass::GetOutputs() const { return impl_->GetOutputs(); }

} // namespace Graphics