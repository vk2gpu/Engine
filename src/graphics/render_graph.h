#pragma once

#include "graphics/dll.h"
#include "graphics/render_resources.h"
#include "core/function.h"

namespace Graphics
{
	class RenderGraphBuilder;
	class RenderGraph;
	class RenderPass;
	class Buffer;
	class Texture;
	struct RenderGraphImpl;

	using RenderGraphExecFn = Core::Function<void(RenderGraph&, void*), 256>;

	class GRAPHICS_DLL RenderGraphBuilder final
	{
	public:
		/**
		 * Create buffer from descriptor.
		 */
		RenderGraphResource CreateBuffer(const char* name, const RenderGraphBufferDesc& desc);

		/**
		 * Create texture from descriptor.
		 */
		RenderGraphResource CreateTexture(const char* name, const RenderGraphTextureDesc& desc);

		/**
		 * Use resource as SRV.
		 */
		RenderGraphResource UseSRV(RenderPass* renderPass, RenderGraphResource res);

		/**
		 * Use resource as RTV.
		 */
		RenderGraphResource UseRTV(RenderPass* renderPass, RenderGraphResource res);

		/**
		 * Use resource as DSV.
		 */
		RenderGraphResource UseDSV(RenderPass* renderPass, RenderGraphResource res, GPU::DSVFlags flags);

		/**
		 * Allocate memory that exists for the life time of a single execute phase.
		 */
		void* Alloc(i32 size);

		/**
		 * Allocate objects that exists for the life time of a single execute phase.
		 */
		template<typename TYPE>
		TYPE* Alloc(i32 num = 1)
		{
			return reinterpret_cast<TYPE*>(Alloc(num * sizeof(TYPE)));
		}

	private:
		friend class RenderGraph;

		RenderGraphBuilder(RenderGraphImpl* impl);
		~RenderGraphBuilder();

		RenderGraphImpl* impl_ = nullptr;
	};

	class GRAPHICS_DLL RenderGraphResources final
	{
	public:
		/**
		 * @return Concrete buffer from render graph.
		 */
		GPU::Handle GetBuffer(RenderGraphResource res, RenderGraphBufferDesc* outDesc = nullptr) const;

		/**
		 * @return Concrete texture from render graph.
		 */
		GPU::Handle GetTexture(RenderGraphResource res, RenderGraphTextureDesc* outDesc = nullptr) const;

	private:
		friend class RenderGraph;

		RenderGraphResources(RenderGraphImpl* impl);
		~RenderGraphResources();

		struct RenderGraphImpl* impl_ = nullptr;
	};

	class GRAPHICS_DLL RenderGraph final
	{
	public:
		RenderGraph();
		~RenderGraph();

		template<typename RENDER_PASS, typename... ARGS>
		RENDER_PASS& AddRenderPass(const char* name, ARGS&&... args)
		{
			RenderGraphBuilder builder(impl_);
			auto* renderPass = new(builder.Alloc<RENDER_PASS>(1)) RENDER_PASS(builder, std::forward<ARGS>(args)...);
			InternalAddRenderPass(name, renderPass);
			return *renderPass;
		}

		/**
		 * Import GPU resource from handle.
		 */
		RenderGraphResource ImportResource(const char* name, GPU::Handle handle);

		/**
		 * Clear all added render passes, memory used, etc.
		 */
		void Clear();

		/**
		 * Execute graph.
		 * This stage will determine the execute order of all the render passes
		 * added, and cull any parts of the graph that are unconnected.
		 * It will then create the appropriate resource,, then execute the render passes 
		 * in the best order determined.
		 * @param finalRes Final output resource for the graph. Will take newest version.
		 */
		void Execute(RenderGraphResource finalRes);

		/**
		 * Get number of executed render passes.
		 */
		i32 GetNumExecutedRenderPasses() const;

		/**
		 * Get executed render passes + names.
		 * @pre renderPasses is nullptr, or points to an array large enough for all executed render passes.
		 * @pre renderPassNames is nullptr points to an array large enough for all executed render passes.
		 */
		void GetExecutedRenderPasses(const RenderPass** renderPasses, const char** renderPassNames) const;

		/**
		 * Get resource name.
		 */
		void GetResourceName(RenderGraphResource res, const char** name) const;

	private:
		void InternalPushPass();
		void InternalPopPass();
		void InternalAddRenderPass(const char* name, RenderPass* renderPass);

		RenderGraphImpl* impl_ = nullptr;
	};
} // namespace Graphics
