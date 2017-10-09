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

	class GRAPHICS_DLL Shader
	{
	public:
		DECLARE_RESOURCE("Graphics.Shader", 0);

		/// @return Is texture ready for use?
		bool IsReady() const { return !!impl_; }

		/**
		 * @return Binding index. -1 if it doesn't exist.
		 */
		i32 GetBindingIndex(const char* name) const;

		/**
		 * Will create a technique for use during rendering.
		 * If required it'll create pipeline states to match @a desc.
		 */
		ShaderTechnique CreateTechnique(const char* name, const ShaderTechniqueDesc& desc);

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

	class GRAPHICS_DLL ShaderTechnique
	{
	public:
		ShaderTechnique();
		~ShaderTechnique();
		ShaderTechnique(ShaderTechnique&&);
		ShaderTechnique& operator=(ShaderTechnique&&);

		// Sampler
		void SetSampler(i32 idx, GPU::Handle res);

		// Set.
		bool Set(i32 idx, const GPU::BindingCBV& binding);
		bool Set(i32 idx, const GPU::BindingSRV& binding);
		bool Set(i32 idx, const GPU::BindingUAV& binding);

		// Convenience setters for development.
		bool Set(const char* name, const GPU::BindingCBV& binding);
		bool Set(const char* name, const GPU::BindingSRV& binding);
		bool Set(const char* name, const GPU::BindingUAV& binding);

		/**
		 * Get binding for the current technique setup.
		 */
		GPU::Handle GetBinding();

		/// @return Is shader technique is valid.
		operator bool() const;

	private:
		friend class Shader;

		ShaderTechnique(const ShaderTechnique&) = delete;
		ShaderTechnique& operator=(const ShaderTechnique&) = delete;

		struct ShaderTechniqueImpl* impl_ = nullptr;
	};

} // namespace Graphics
