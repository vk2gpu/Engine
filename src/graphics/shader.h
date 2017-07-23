#pragma once

#include "graphics/dll.h"
#include "core/array.h"
#include "gpu/fwd_decls.h"
#include "gpu/types.h"
#include "resource/resource.h"

namespace Graphics
{
	struct ShaderTechniqueDesc;
	class ShaderTechnique;


	class GRAPHICS_DLL Shader
	{
	public:
		DECLARE_RESOURCE("Graphics.Shader", 0);

		/// @return Is texture ready for use?
		bool IsReady() const { return !!impl_; }

		/**
		 * Will create a technique for use during rendering.
		 * If required it'll create pipeline states to match @a desc.
		 */
		ShaderTechnique CreateTechnique(const char* name, const ShaderTechniqueDesc& desc);

	private:
		friend class Factory;

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
		ShaderTechniqueDesc& SetTopology(GPU::TopologyType topology);
		ShaderTechniqueDesc& SetRTVFormat(i32 idx, GPU::Format format);
		ShaderTechniqueDesc& SetDSVFormat(GPU::Format format);

		i32 numVertexElements_ = 0;
		Core::Array<GPU::VertexElement, GPU::MAX_VERTEX_ELEMENTS> vertexElements_;
		GPU::TopologyType topology_ = GPU::TopologyType::INVALID;
		i32 numRTs_ = 0;
		Core::Array<GPU::Format, GPU::MAX_BOUND_RTVS> rtvFormats_;
		GPU::Format dsvFormat_ = GPU::Format::INVALID;
	};

	class GRAPHICS_DLL ShaderTechnique
	{
	public:
		ShaderTechnique();
		~ShaderTechnique();
		ShaderTechnique(ShaderTechnique&&);
		ShaderTechnique& operator=(ShaderTechnique&&);

		/**
		 * Get binding for the current technique setup.
		 */
		GPU::Handle GetBinding();

		/// @return If shader technique is valid.
		operator bool() const { return !!impl_; }

	private:
		friend class Shader;

		ShaderTechnique(const ShaderTechnique&) = delete;
		ShaderTechnique& operator=(const ShaderTechnique&) = delete;

		struct ShaderTechniqueImpl* impl_ = nullptr;
	};

} // namespace Graphics
