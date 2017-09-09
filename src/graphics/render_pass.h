#pragma once

#include "graphics/dll.h"
#include "graphics/render_resources.h"
#include "core/array.h"
#include "core/array_view.h"
#include "core/function.h"
#include "gpu/fwd_decls.h"

namespace Graphics
{
	class RenderGraphBuilder;
	class RenderGraphResources;
	class RenderGraph;

	/**
	 * Base render pass.
	 */
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

		/**
		 * @return Get frame binding descriptor. Valid after construction.
		 */
		const GPU::FrameBindingSetDesc& GetFrameBindingDesc() const;

	private:
		friend class RenderGraph;
		friend struct RenderGraphImpl;
		friend class RenderGraphBuilder;
		struct RenderPassImpl* impl_ = nullptr;
	};


	/**
	 * Callback render pass for setting up render passes inline.
	 */
	template<typename DATA>
	class CallbackRenderPass : public RenderPass
	{
	public:
		using ExecuteFn = Core::Function<void(RenderGraphResources& res, GPU::CommandList& cmdList, const DATA& data)>;

		template<typename SETUPFN>
		CallbackRenderPass(RenderGraphBuilder& builder, SETUPFN&& setupFn, ExecuteFn&& executeFn)
		    : RenderPass(builder)
		    , executeFn_(executeFn)
		{
			setupFn(builder, data_);
		}

		~CallbackRenderPass() {}

		void Execute(RenderGraphResources& res, GPU::CommandList& cmdList) override { executeFn_(res, cmdList, data_); }

		/**
		 * @return Render pass data that's setup when added.
		 */
		const DATA& GetData() const { return data_; }

	private:
		DATA data_;
		ExecuteFn executeFn_;
	};

} // namespace Graphics
