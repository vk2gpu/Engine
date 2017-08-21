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

	private:
		friend class RenderGraph;

		RenderGraphBuilder(RenderGraphImpl* impl);
		~RenderGraphBuilder();

		RenderGraphImpl* impl_ = nullptr;
	};

	class GRAPHICS_DLL RenderGraphResources final
	{
	public:
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
			auto* renderPass = new(Alloc<RENDER_PASS>(1)) RENDER_PASS(builder, std::forward<ARGS>(args)...);
			InternalAddRenderPass(name, renderPass);
			return *renderPass;
		}

		/**
		 * Import GPU resource from handle.
		 */
		RenderGraphResource ImportResource(GPU::Handle handle);

		/**
		 * Clear all added render passes, memory used, etc.
		 */
		void Clear();

		/**
		 * Compile graph.
		 * This stage will determine the execute order of all the render passes
		 * added, and cull any parts of the graph that are unconnected.
		 * @param finalRes Final output resource for the graph. Will take newest version.
		 */
		void Compile(RenderGraphResource finalRes);

		/**
		 * Allocate memory that exists for the life time of a single execute phase.
		 */
		void* Alloc(i32 size);

		/**
		 * Allocate objects that exists for the life time of a single execute phase.
		 */
		template<typename TYPE>
		TYPE* Alloc(i32 num)
		{
			return reinterpret_cast<TYPE*>(Alloc(num * sizeof(TYPE)));
		}

	private:
		void InternalPushPass();
		void InternalPopPass();
		void InternalAddRenderPass(const char* name, RenderPass* renderPass);

		RenderGraphImpl* impl_ = nullptr;
	};
} // namespace Graphics
