#pragma once

#include "graphics/dll.h"
#include "gpu/fwd_decls.h"

namespace Graphics
{
	class RenderGraphBuilder;
	class RenderGraph;

	class GRAPHICS_DLL RenderPass
	{
	public:
		RenderPass(RenderGraphBuilder& builder);
		virtual ~RenderPass();

		/**
		 * Execute render pass, building up command list.
		 */
		virtual void Execute(RenderGraph& rg, GPU::CommandList& cmdList) = 0;

	private:
	};

} // namespace Graphics
