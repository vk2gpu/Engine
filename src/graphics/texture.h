#pragma once

#include "graphics/dll.h"
#include "gpu/fwd_decls.h"
#include "resource/resource.h"

namespace Graphics
{
	class GRAPHICS_DLL Texture
	{
	public:
		DECLARE_RESOURCE("Graphics.Texture", 0);
		Texture();
		~Texture();

		/// @return Is texture ready for use?
		bool IsReady() const { return !!impl_; }

	private:
		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;

		friend class Factory;

		struct TextureImpl* impl_ = nullptr;
	};

} // namespace Graphics
