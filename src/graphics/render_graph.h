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


	class GRAPHICS_DLL RenderGraphBuilder
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
		RenderGraphResource UseSRV(RenderGraphResource res);

		/**
		 * Use resource as RTV.
		 */
		RenderGraphResource UseRTV(RenderGraphResource res);

	private:
		friend class RenderGraph;

		RenderGraphBuilder(RenderGraphImpl* impl);
		~RenderGraphBuilder();

		RenderGraphImpl* impl_ = nullptr;
	};

	class GRAPHICS_DLL RenderGraphResources
	{
	public:
	private:
		friend class RenderGraph;

		RenderGraphResources(RenderGraphImpl* impl);
		~RenderGraphResources();

		struct RenderGraphImpl* impl_ = nullptr;
	};

	class GRAPHICS_DLL RenderGraph
	{
	public:
		RenderGraph();
		~RenderGraph();

		template<typename RENDER_PASS, typename... ARGS>
		RENDER_PASS& AddRenderPass(const char* name, ARGS&&... args)
		{
			InternalPushPass();
			RenderGraphBuilder builder(impl_);
			auto* renderPass = new(Alloc<RENDER_PASS>(1)) RENDER_PASS(builder, std::forward<ARGS>(args)...);
			InternalAddRenderPass(name, renderPass);
			InternalPopPass();
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
