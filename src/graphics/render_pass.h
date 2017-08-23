#pragma once

#include "graphics/dll.h"
#include "graphics/render_resources.h"
#include "core/array.h"
#include "core/array_view.h"
#include "gpu/fwd_decls.h"

namespace Graphics
{
	class RenderGraphBuilder;
	class RenderGraphResources;
	class RenderGraph;

	class GRAPHICS_DLL RenderPass
	{
	public:
		RenderPass(RenderGraphBuilder& builder);
		virtual ~RenderPass();

		/**
		 * Execute render pass, building up command list.
		 */
		virtual void Execute(RenderGraphResources& res, GPU::CommandList& cmdList) = 0;

		/**
		 * @return inputs for this render passs.
		 */
		Core::ArrayView<const RenderGraphResource> GetInputs() const;

		/**
		 * @return outputs for this render pass.
		 */
		Core::ArrayView<const RenderGraphResource> GetOutputs() const;

	private:
		friend class RenderGraph;
		friend class RenderGraphBuilder;
		struct RenderPassImpl* impl_ = nullptr;
	};

} // namespace Graphics
