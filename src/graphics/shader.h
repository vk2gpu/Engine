#pragma once

#include "graphics/dll.h"
#include "graphics/fwd_decls.h"
#include "core/array.h"
#include "gpu/fwd_decls.h"
#include "gpu/types.h"
#include "gpu/resources.h"
#include "resource/resource.h"
#include "resource/ref.h"

namespace Graphics
{
	using ShaderRef = Resource::Ref<class Shader>;

	/**
	 * Shader resource.
	 * This would loaded from a converted ESF (engine shader file).
	 * From here techniques and binding sets can be created for use in
	 * the graphics pipeline.
	 */
	class GRAPHICS_DLL Shader
	{
	public:
		DECLARE_RESOURCE(Shader, "Graphics.Shader", 0);

		/// @return Is texture ready for use?
		bool IsReady() const { return !!impl_; }

		/**
		 * Will create a technique for use during rendering.
		 * If required it'll create pipeline states to match @a desc.
		 */
		ShaderTechnique CreateTechnique(const char* name, const ShaderTechniqueDesc& desc);

		/**
		 * Will create a binding set to allow resources to be bound to shaders.
		 */
		ShaderBindingSet CreateBindingSet(const char* name);

		/**
		 * Will create a binding set to allow resources to be bound to shaders.
		 */
		static ShaderBindingSet CreateSharedBindingSet(const char* name);

	private:
		friend class ShaderFactory;

		Shader();
		~Shader();
		Shader(const Shader&) = delete;
		Shader& operator=(const Shader&) = delete;

		struct ShaderImpl* impl_ = nullptr;
	};

	/**
	 * Descriptor to provide additional information to prepare a technique to be used for rendering.
	 */
	struct GRAPHICS_DLL ShaderTechniqueDesc
	{
		ShaderTechniqueDesc() = default;

		ShaderTechniqueDesc& SetVertexElement(i32 idx, const GPU::VertexElement& element);
		ShaderTechniqueDesc& SetVertexElements(Core::ArrayView<GPU::VertexElement> elements);
		ShaderTechniqueDesc& SetTopology(GPU::TopologyType topology);
		ShaderTechniqueDesc& SetRTVFormat(i32 idx, GPU::Format format);
		ShaderTechniqueDesc& SetDSVFormat(GPU::Format format);
		ShaderTechniqueDesc& SetFrameBindingSet(const GPU::FrameBindingSetDesc& desc);

		i32 numVertexElements_ = 0;
		Core::Array<GPU::VertexElement, GPU::MAX_VERTEX_ELEMENTS> vertexElements_ = {};
		GPU::TopologyType topology_ = GPU::TopologyType::INVALID;
		i32 numRTs_ = 0;
		Core::Array<GPU::Format, GPU::MAX_BOUND_RTVS> rtvFormats_ = {};
		GPU::Format dsvFormat_ = GPU::Format::INVALID;
	};

	/**
	 * Shader technique.
	 * This is used to represent a single pipeline state setup. In ESF it
	 * is defined in @see metadata.esh as a struct with members that can be set.
	 * An example setup would look like:
	 *
	 * Technique MyRenderPass =
	 * {
	 *     .VertexShader = my_vs,
	 *     .PixelShader = my_ps,
	 *     .RenderState = RS_MY_RENDER_STATE,
	 * };
	 */
	class GRAPHICS_DLL ShaderTechnique
	{
	public:
		ShaderTechnique();
		~ShaderTechnique();
		ShaderTechnique(ShaderTechnique&&);
		ShaderTechnique& operator=(ShaderTechnique&&);

		/// @return Is shader technique is valid.
		explicit operator bool() const;

	private:
		friend class Shader;
		friend class ShaderContext;

		ShaderTechnique(const ShaderTechnique&) = delete;
		ShaderTechnique& operator=(const ShaderTechnique&) = delete;

		struct ShaderTechniqueImpl* impl_ = nullptr;
	};

	/// Binding handle alias.
	using ShaderBindingHandle = u32;

	/**
	 * Shader binding set.
	 * This encapsulates a group of bindings which are declared in shaders.
	 * It is defined in @see metadata.esh as a struct_type.
	 * An example setup would look like:
	 * 
	 * [frequency(HIGH)]
	 * BindingSet MyBindings
	 * {
	 *     Texture2D<float4> mySRV;
	 *     ConstantBuffer<MyStruct> myCBV;
	 *     RWTexture2D<int4> myUAV;
	 * };
	 */
	class GRAPHICS_DLL ShaderBindingSet
	{
	public:
		ShaderBindingSet();
		~ShaderBindingSet();

		ShaderBindingSet(ShaderBindingSet&&);
		ShaderBindingSet& operator=(ShaderBindingSet&&);

		/**
		 * @return Binding index. Non-zero should be valid.
		 */
		ShaderBindingHandle GetBindingHandle(const char* name) const;

		// Setters.
		ShaderBindingSet& Set(ShaderBindingHandle idx, const GPU::SamplerState& sampler);
		ShaderBindingSet& Set(ShaderBindingHandle idx, const GPU::BindingCBV& binding);
		ShaderBindingSet& Set(ShaderBindingHandle idx, const GPU::BindingSRV& binding);
		ShaderBindingSet& Set(ShaderBindingHandle idx, const GPU::BindingUAV& binding);

		// Set all SRVs (temporary).
		ShaderBindingSet& SetAll(const GPU::BindingSRV& binding);

		// Convenience setters for development.
		ShaderBindingSet& Set(const char* name, const GPU::SamplerState& sampler);
		ShaderBindingSet& Set(const char* name, const GPU::BindingCBV& binding);
		ShaderBindingSet& Set(const char* name, const GPU::BindingSRV& binding);
		ShaderBindingSet& Set(const char* name, const GPU::BindingUAV& binding);
	
		/**
		 * Validate bindings.
		 * @return true if valid.
		 */
		bool Validate() const;

		explicit operator bool() const { return !!impl_; }

	private:
		friend class Shader;
		friend class ShaderContext;

		ShaderBindingSet(const ShaderBindingSet&) = delete;
		ShaderBindingSet& operator=(const ShaderBindingSet&) = delete;

		struct ShaderBindingSetImpl* impl_ = nullptr;
#if !defined(FINAL)
		const char* name_ = nullptr;
#endif
	};

	/**
	 * Shader context.
	 * This is used to bind resources to shaders when rendering.
	 */
	class GRAPHICS_DLL ShaderContext
	{
	public:
		ShaderContext(GPU::CommandList& cmdList);
		~ShaderContext();

		class ScopedBinding;

		/**
		 * Begin a binding scope.
		 * @return RAII object to unset after the calling scope.
		 */
		ScopedBinding BeginBindingScope(const ShaderBindingSet& bindingSet);

		/**
		 * Commit bindings for technique.
		 */
		bool CommitBindings(
		    const ShaderTechnique& tech, GPU::Handle& outPs, Core::ArrayView<GPU::PipelineBinding>& outPb);

		/**
		 * Scoped binding object to automatically unbind at the end of a scope.
		 * @see BegingBindingScope.
		 */
		class ScopedBinding
		{
		public:
			ScopedBinding(ScopedBinding&&) = default;
			~ScopedBinding()
			{
				if(idx_ >= 0)
					ctx_.EndBindingScope(idx_);
			}

			explicit operator bool() const { return idx_ >= 0; }

		private:
			friend class ShaderContext;

			ScopedBinding(const ScopedBinding&) = delete;

			ScopedBinding(ShaderContext& ctx, i32 idx)
			    : ctx_(ctx)
			    , idx_(idx)
			{
			}

			ShaderContext& ctx_;
			i32 idx_ = -1;
		};

	private:
		friend class Shader;
		friend class ScopedBinding;

		void EndBindingScope(i32 idx);

		ShaderContext(const ShaderContext&) = delete;
		ShaderContext& operator=(const ShaderContext&) = delete;

		struct ShaderContextImpl* impl_ = nullptr;
	};

} // namespace Graphics
