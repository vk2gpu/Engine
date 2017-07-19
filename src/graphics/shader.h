#pragma once

#include "graphics/dll.h"
#include "gpu/fwd_decls.h"
#include "resource/resource.h"

namespace Graphics
{
	class GRAPHICS_DLL Shader
	{
	public:
		DECLARE_RESOURCE("Graphics.Shader", 0);
		Shader();
		~Shader();

		/// @return Is texture ready for use?
		bool IsReady() const { return !!impl_; }

	private:
		Shader(const Shader&) = delete;
		Shader& operator=(const Shader&) = delete;

		friend class Factory;

		struct ShaderImpl* impl_ = nullptr;
	};

} // namespace Graphics
