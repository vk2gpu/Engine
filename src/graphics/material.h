#pragma once

#include "graphics/dll.h"
#include "graphics/fwd_decls.h"
#include "resource/resource.h"
#include "resource/ref.h"

namespace Graphics
{
	using MaterialRef = Resource::Ref<class Material>;

	class GRAPHICS_DLL Material
	{
	public:
		DECLARE_RESOURCE("Graphics.Material", 0);

		/// @return Get shader associated with this material.
		Shader* GetShader() const;

		/// Create shader technique.
		ShaderTechnique CreateTechnique(const char* name, const ShaderTechniqueDesc& desc);

	private:
		friend class MaterialFactory;

		Material();
		~Material();
		Material(const Material&) = delete;
		Material& operator=(const Material&) = delete;

		struct MaterialImpl* impl_ = nullptr;
	};

} // namespace Graphics
