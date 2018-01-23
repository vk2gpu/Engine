#pragma once

#include "dll.h"
#include "test_shared.h"
#include "graphics/texture.h"

class APP_COMMON_DLL TextureCompressor
{
public:
	TextureCompressor();
	~TextureCompressor();

	/**
	 * Compress a texture to target format.
	 * @param cmdList Command list to use.
	 * @param inTexture Input texture to compress.
	 * @param format Format to compress to.
	 * @param outputTexture Output texture to write to.
	 * @param point Point to copy to on @a outputTexture.
	 */
	bool Compress(GPU::CommandList& cmdList, Graphics::Texture* inTexture, GPU::Format format,
	    GPU::Handle outputTexture, GPU::Point point = GPU::Point());

private:
	struct LookupTable
	{
		u32 Expand5[32];
		u32 Expand6[64];
		u32 OMatch5[256][2];
		u32 OMatch6[256][2];
	};

	GPU::Handle lookupTableCB_;
	Graphics::ShaderRef shader_;
	Graphics::ShaderBindingSet bindings_;
};
