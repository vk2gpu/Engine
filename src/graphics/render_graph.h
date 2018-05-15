#pragma once

#include "graphics/dll.h"
#include "graphics/render_resources.h"
#include "graphics/render_pass.h"
#include "core/function.h"

namespace Graphics
{
	class RenderGraphBuilder;
	class RenderGraph;
	class Buffer;
	class Texture;
	struct RenderGraphImpl;
	struct RenderPassImpl;

	using RenderGraphExecFn = Core::Function<void(RenderGraph&, void*), 256>;

	class GRAPHICS_DLL RenderGraphBuilder final
	{
	public:
		/**
		 * Create buffer from descriptor.
		 * @param name Debug name for buffer.
		 * @param desc Descriptor for buffer.
		 * @return Resource reference.
		 */
		RenderGraphResource Create(const char* name, const RenderGraphBufferDesc& desc);

		/**
		 * Create texture from descriptor.
		 * @param name Debug name for texture.
		 * @param desc Descriptor for texture.
		 * @return Resource reference.
		 */
		RenderGraphResource Create(const char* name, const RenderGraphTextureDesc& desc);

		/**
		 * Read from resource.
		 * @param res Input resource.
		 * @param bindFlags Bind flags required for read.
		 * @return Resource that subsequent passes should reference.
		 */
		RenderGraphResource Read(RenderGraphResource res, GPU::BindFlags bindFlags = GPU::BindFlags::NONE);

		/**
		 * Write to resource.
		 * @param res Input resource.
		 * @param bindFlags Bind flags required for write.
		 * @return Resource that subsequent passes should reference.
		 */
		RenderGraphResource Write(RenderGraphResource res, GPU::BindFlags bindFlags = GPU::BindFlags::NONE);

		/**
		 * Set resource as for use a an RTV.
		 * @param idx RTV index to bind to.
		 * @param res Resource.
		 * @param binding Binding info for RTV.
		 * @return Resource that subsequent passes should reference.
		 */
		RenderGraphResource SetRTV(i32 idx, RenderGraphResource res, GPU::BindingRTV binding = GPU::BindingRTV());

		/**
		 * Set resource for use as a DSV.
		 * @param res Resource to bind as DSV.
		 * @param binding Binding info for DSV.
		 * @return Resource that subsequent passes should reference.
		 */
		RenderGraphResource SetDSV(RenderGraphResource res, GPU::BindingDSV binding = GPU::BindingDSV());

		/**
		 * @return Buffer desc from render graph.
		 */
		bool GetBuffer(RenderGraphResource res, RenderGraphBufferDesc* outDesc = nullptr) const;

		/**
		 * @return Texture desc from render graph.
		 */
		bool GetTexture(RenderGraphResource res, RenderGraphTextureDesc* outDesc = nullptr) const;

		/**
		 * Allocate memory that exists for the life time of a single execute phase.
		 */
		void* Alloc(i32 size);

		/**
		 * Allocate objects that exists for the life time of a single execute phase.
		 * Does not construct object, this is the user's responsibility.
		 */
		template<typename TYPE>
		TYPE* Alloc(i32 num = 1)
		{
			return reinterpret_cast<TYPE*>(Alloc(num * sizeof(TYPE)));
		}

		/**
		 * Push data into render graph.
		 * @param bytes Number of bytes to allocate.
		 * @param data Pointer to data to copy.
		 * @return Allocated memory. Only valid until submission.
		 * @pre @a bytes > 0.
		 */
		void* Push(const void* data, i32 bytes);

		/**
		 * Templated push. See above.
		 */
		template<typename TYPE>
		TYPE* Push(const TYPE* data, i32 num = 1)
		{
			TYPE* dest = reinterpret_cast<TYPE*>(Alloc(sizeof(TYPE) * num));
			if(dest)
				for(i32 idx = 0; idx < num; ++idx)
					new(dest + idx) TYPE(data[idx]);
			return dest;
		}

	private:
		friend class RenderGraph;

		RenderGraphBuilder(RenderGraphImpl* impl, RenderPass* renderPass);
		~RenderGraphBuilder();

		RenderGraphImpl* impl_ = nullptr;
		RenderPass* renderPass_ = nullptr;
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

		/**
		 * @return Concrete frame binding set.
		 */
		GPU::Handle GetFrameBindingSet(GPU::FrameBindingSetDesc* outDesc = nullptr) const;

		/**
		 * Binding helpers.
		 */
		GPU::BindingCBV CBuffer(RenderGraphResource res, i32 offset, i32 size) const;
		GPU::BindingSRV Buffer(RenderGraphResource res, GPU::Format format = GPU::Format::INVALID, i32 firstElement = 0,
		    i32 numElements = 0, i32 structureByteStride = 0) const;
		GPU::BindingSRV Texture1D(RenderGraphResource res, GPU::Format format = GPU::Format::INVALID,
		    i32 mostDetailedMip = 0, i32 mipLevels = 0, f32 resourceMinLODClamp = 0.0f) const;
		GPU::BindingSRV Texture1DArray(RenderGraphResource res, GPU::Format format = GPU::Format::INVALID,
		    i32 mostDetailedMip = 0, i32 mipLevels = 0, i32 firstArraySlice = 0, i32 arraySize = 0,
		    f32 resourceMinLODClamp = 0.0f) const;
		GPU::BindingSRV Texture2D(RenderGraphResource res, GPU::Format format = GPU::Format::INVALID,
		    i32 mostDetailedMip = 0, i32 mipLevels = 0, i32 planeSlice = 0, f32 resourceMinLODClamp = 0.0f) const;
		GPU::BindingSRV Texture2DArray(RenderGraphResource res, GPU::Format format = GPU::Format::INVALID,
		    i32 mostDetailedMip = 0, i32 mipLevels = 0, i32 firstArraySlice = 0, i32 arraySize = 0, i32 planeSlice = 0,
		    f32 resourceMinLODClamp = 0.0f) const;
		GPU::BindingSRV Texture3D(RenderGraphResource res, GPU::Format format = GPU::Format::INVALID,
		    i32 mostDetailedMip = 0, i32 mipLevels = 0, f32 resourceMinLODClamp = 0.0f) const;
		GPU::BindingSRV TextureCube(RenderGraphResource res, GPU::Format format = GPU::Format::INVALID,
		    i32 mostDetailedMip = 0, i32 mipLevels = 0, f32 resourceMinLODClamp = 0.0f) const;
		GPU::BindingSRV TextureCubeArray(RenderGraphResource res, GPU::Format format = GPU::Format::INVALID,
		    i32 mostDetailedMip = 0, i32 mipLevels = 0, i32 first2DArrayFace = 0, i32 numCubes = 0,
		    f32 resourceMinLODClamp = 0.0f) const;
		GPU::BindingUAV RWBuffer(RenderGraphResource res, GPU::Format format = GPU::Format::INVALID,
		    i32 firstElement = 0, i32 numElements = 0, i32 structureByteStride = 0) const;
		GPU::BindingUAV RWTexture1D(
		    RenderGraphResource res, GPU::Format format = GPU::Format::INVALID, i32 mipSlice = 0) const;
		GPU::BindingUAV RWTexture1DArray(RenderGraphResource res, GPU::Format format = GPU::Format::INVALID,
		    i32 mipSlice = 0, i32 firstArraySlice = 0, i32 arraySize = 0) const;
		GPU::BindingUAV RWTexture2D(RenderGraphResource res, GPU::Format format = GPU::Format::INVALID,
		    i32 mipSlice = 0, i32 planeSlice = 0) const;
		GPU::BindingUAV RWTexture2DArray(RenderGraphResource res, GPU::Format format = GPU::Format::INVALID,
		    i32 mipSlice = 0, i32 planeSlice = 0, i32 firstArraySlice = 0, i32 arraySize = 0) const;
		GPU::BindingUAV RWTexture3D(RenderGraphResource res, GPU::Format format = GPU::Format::INVALID,
		    i32 mipSlice = 0, i32 firstWSlice = 0, i32 wSize = 0) const;

	private:
		friend class RenderGraph;

		RenderGraphResources(RenderGraphImpl* impl, RenderPassImpl* renderPass);
		~RenderGraphResources();
		RenderGraphResources(const RenderGraphResources&) = delete;

		RenderGraphImpl* impl_ = nullptr;
		RenderPassImpl* renderPass_ = nullptr;
	};


	class GRAPHICS_DLL RenderGraph final
	{
	public:
		RenderGraph();
		~RenderGraph();

		/**
		 * Add render pass to the graph.
		 * @param name Name of pass.
		 * @param args Argument list to pass to @a RENDER_PASS constructor.
		 */
		template<typename RENDER_PASS, typename... ARGS>
		RENDER_PASS& AddRenderPass(const char* name, ARGS&&... args)
		{
			auto* renderPassMem = InternalAlloc<RENDER_PASS>(1);
			RenderGraphBuilder builder(impl_, renderPassMem);
			auto* renderPass = new(renderPassMem) RENDER_PASS(builder, std::forward<ARGS>(args)...);
			InternalAddRenderPass(name, renderPass);
			return *renderPass;
		}

		/**
		 * Add callback render pass to the graph.
		 * @param name Name of pass.
		 * @param setupFn Setup function to call into now.
		 * @param executeFn Execute function to call during the execute phase later.
		 */
		template<typename DATA, typename SETUPFN>
		const CallbackRenderPass<DATA>& AddCallbackRenderPass(
		    const char* name, SETUPFN&& setupFn, typename CallbackRenderPass<DATA>::ExecuteFn&& executeFn)
		{
			auto& renderPass = AddRenderPass<CallbackRenderPass<DATA>>(name, std::move(setupFn), std::move(executeFn));
			return renderPass;
		}

		/**
		 * Import buffer resource from handle.
		 */
		RenderGraphResource ImportResource(const char* name, GPU::Handle handle, const RenderGraphBufferDesc& desc);

		/**
		 * Import texture resource from handle.
		 */
		RenderGraphResource ImportResource(const char* name, GPU::Handle handle, const RenderGraphTextureDesc& desc);

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
		 * @return true if successful.
		 */
		bool Execute(RenderGraphResource finalRes);

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

		/**
		 * Get buffer resource.
		 * @param res Render graph resource.
		 * @param outDesc Pointer to buffer descriptor to return.
		 * @param outHandle Pointer to handle to return. Intended for debug purposes only.
		 * @return true is success.
		 */
		bool GetBuffer(
		    RenderGraphResource res, RenderGraphBufferDesc* outDesc = nullptr, GPU::Handle* outHandle = nullptr) const;

		/**
		 * Get texture resource.
		 * @param res Render graph resource.
		 * @param outDesc Pointer to texture descriptor to return.
		 * @param outHandle Pointer to handle to return. Intended for debug purposes only.
		 * @return true is success.
		 */
		bool GetTexture(
		    RenderGraphResource res, RenderGraphTextureDesc* outDesc = nullptr, GPU::Handle* outHandle = nullptr) const;

	private:
		void InternalPushPass();
		void InternalPopPass();
		void InternalAddRenderPass(const char* name, RenderPass* renderPass);
		void* InternalAlloc(i32 size);

		template<typename TYPE>
		TYPE* InternalAlloc(i32 num = 1)
		{
			return reinterpret_cast<TYPE*>(InternalAlloc(num * sizeof(TYPE)));
		}

		RenderGraphImpl* impl_ = nullptr;
	};
} // namespace Graphics
