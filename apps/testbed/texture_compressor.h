#pragma once
#include "test_shared.h"
#include "graphics/texture.h"

class TextureCompressor
{
public:
	TextureCompressor();
	~TextureCompressor();

	bool Compress(
	    GPU::CommandList& cmdList, Graphics::Texture* inTexture, GPU::Format format, GPU::Handle outputTexture);

private:
	struct LookupTable
	{
		u32 Expand5[32];
		u32 Expand6[64];
		u32 OMatch5[256][2];
		u32 OMatch6[256][2];
	};

	GPU::Handle lookupTableCB_;
	Graphics::Shader* shader_ = nullptr;
};
