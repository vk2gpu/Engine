#pragma once

#include "graphics/dll.h"
#include "gpu/fwd_decls.h"
#include "resource/resource.h"
#include "resource/ref.h"

namespace Graphics
{
	using TextureRef = Resource::Ref<class Texture>;

	class GRAPHICS_DLL Texture
	{
	public:
		DECLARE_RESOURCE(Texture, "Graphics.Texture", 0);
		Texture();
		~Texture();

		/// @return Is texture ready for use?
		bool IsReady() const { return !!impl_; }

		/// @return Texture desc.
		const GPU::TextureDesc& GetDesc() const;

		/// @return Texture handle.
		GPU::Handle GetHandle() const;

	private:
		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;

		friend class TextureFactory;

		struct TextureImpl* impl_ = nullptr;
	};

} // namespace Graphics
