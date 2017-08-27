#pragma once

#include "graphics/dll.h"
#include "graphics/render_resources.h"
#include "core/array_view.h"
#include "core/vector.h"
#include "plugin/plugin.h"

namespace Graphics
{
	class RenderGraph;

	/**
	 * Graphics pipeline interface.
	 * These are responsible for setting up render passes.
	 */
	class GRAPHICS_DLL IPipeline
	{
	public:
		virtual ~IPipeline() {}

		/**
		 * Get resource names.
		 */
		virtual Core::ArrayView<const char*> GetResourceNames() const = 0;

		/**
		 * Get resoure index.
		 */
		virtual i32 GetResourceIdx(const char* name) const = 0;

		/**
		 * Set render graph resource.
		 * @pre Called before Setup is called.
		 */
		virtual void SetResource(i32 idx, RenderGraphResource res) = 0;

		/**
		 * Get render graph resource.
		 * @pre Called after Setup is called.
		 */
		virtual RenderGraphResource GetResource(i32 idx) const = 0;

		/**
		 * Setup render passes.
		 */
		virtual void Setup(RenderGraph& renderGraph) = 0;

		/**
		 * Any errors occur during the execute phase?
		 */
		virtual bool HaveExecuteErrors() const = 0;
	};

	/**
	 * Base pipeline to provide some helpers.
	 * Not required to use this.
	 */
	class GRAPHICS_DLL Pipeline : public IPipeline
	{
	public:
		/**
		 * @param resourceNames Null terminated array of resource names, must be static.
		 */
		Pipeline(const char** resourceNames);
		virtual ~Pipeline();

		Core::ArrayView<const char*> GetResourceNames() const override;
		i32 GetResourceIdx(const char* name) const override;
		void SetResource(i32 idx, RenderGraphResource res) override;
		RenderGraphResource GetResource(i32 idx) const override;
		void SetResource(const char* name, RenderGraphResource res);
		RenderGraphResource GetResource(const char* name) const;

	protected:
		const char** resourceNames_ = nullptr;
		Core::Vector<RenderGraphResource> resources_;
	};

	/**
	 * Define pipeline plugin.
	 */
	struct PipelinePlugin : Plugin::Plugin
	{
		DECLARE_PLUGININFO(PipelinePlugin, 0);

		typedef IPipeline* (*CreatePipelineFn)();
		CreatePipelineFn CreatePipeline = nullptr;

		typedef void (*DestroyPipelineFn)(IPipeline*&);
		DestroyPipelineFn DestroyPipeline = nullptr;
	};

} // namespace Resource#pragma once
