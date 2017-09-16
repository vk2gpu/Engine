#include "texture_compressor.h"

TextureCompressor::TextureCompressor()
{
	Resource::Manager::RequestResource(shader_, "shader_tests/texture_compressor.esf");

	// Create lookup table to pass to shader.
	LookupTable lookupTable;
	for(i32 i = 0; i < 32; ++i)
		lookupTable.Expand5[i] = (i<<3)|(i>>2);
	for(i32 i = 0; i < 64; ++i)
		lookupTable.Expand6[i] = (i<<2)|(i>>4);

	auto stb__PrepareOptTable = [](u32 *table,const u32* expand,int size)
	{
		int i,mn,mx;
		for (i=0;i<256;i++)
		{
			int bestErr = 256;
			for (mn=0;mn<size;mn++) {
				for (mx=0;mx<size;mx++) {
					int mine = expand[mn];
					int maxe = expand[mx];
					int err = abs((2*maxe + mine) / 3 - i);
            
					// DX10 spec says that interpolation must be within 3% of "correct" result,
					// add this as error term. (normally we'd expect a random distribution of
					// +-1.5% error, but nowhere in the spec does it say that the error has to be
					// unbiased - better safe than sorry).
					err += abs(maxe - mine) * 3 / 100;
            
					if(err < bestErr)
					{ 
						table[i*2+0] = mx;
						table[i*2+1] = mn;
						bestErr = err;
					}
				}
			}
		}
	};

	stb__PrepareOptTable(&lookupTable.OMatch5[0][0], lookupTable.Expand5, 32);
	stb__PrepareOptTable(&lookupTable.OMatch6[0][0], lookupTable.Expand6, 64);

	GPU::BufferDesc desc;
	desc.bindFlags_ = GPU::BindFlags::CONSTANT_BUFFER;
	desc.size_ = sizeof(lookupTable);
	lookupTableCB_ = GPU::Manager::CreateBuffer(desc, &lookupTable, "TextureCompressor Lookup Table");
}

TextureCompressor::~TextureCompressor()
{
	GPU::Manager::DestroyResource(lookupTableCB_);

	Resource::Manager::WaitForResource(shader_);
	Resource::Manager::ReleaseResource(shader_);
}

bool TextureCompressor::Compress(GPU::CommandList& cmdList, Graphics::Texture* inTexture, 
	GPU::Format format, GPU::Handle outputTexture)
{
	// Wait until shader is ready.
	Resource::Manager::WaitForResource(shader_);

	// Setup correct technique name & format.
	const char* techName = "";
	GPU::Format uavFormat = GPU::Format::INVALID;
	switch(format)
	{
	case GPU::Format::BC1_TYPELESS:
	case GPU::Format::BC1_UNORM:
	case GPU::Format::BC1_UNORM_SRGB:
		techName = "TECH_COMPRESS_BC1";
		uavFormat = GPU::Format::R32G32_UINT;
		break;

	case GPU::Format::BC3_TYPELESS:
	case GPU::Format::BC3_UNORM:
	case GPU::Format::BC3_UNORM_SRGB:
		techName = "TECH_COMPRESS_BC3";
		uavFormat = GPU::Format::R32G32B32A32_UINT;
		break;

	case GPU::Format::BC4_TYPELESS:
	case GPU::Format::BC4_UNORM:
	case GPU::Format::BC4_SNORM:
		techName = "TECH_COMPRESS_BC4";
		uavFormat = GPU::Format::R32G32_UINT;
		break;

	case GPU::Format::BC5_TYPELESS:
	case GPU::Format::BC5_UNORM:
	case GPU::Format::BC5_SNORM:
		techName = "TECH_COMPRESS_BC5";
		uavFormat = GPU::Format::R32G32B32A32_UINT;
		break;

	default:
		return false;
	}

	auto tech = shader_->CreateTechnique(techName, Graphics::ShaderTechniqueDesc());
	const auto& desc = inTexture->GetDesc();
	
	GPU::TextureDesc outTextureDesc = inTexture->GetDesc();
	outTextureDesc.width_ = (outTextureDesc.width_ + 3) / 4;
	outTextureDesc.height_ = (outTextureDesc.height_ + 3) / 4;
	outTextureDesc.format_ = uavFormat;
	outTextureDesc.bindFlags_ |= GPU::BindFlags::UNORDERED_ACCESS;
	GPU::Handle intermediateTexture = GPU::Manager::CreateTexture(outTextureDesc, nullptr, "outCompressed");
	DBG_ASSERT(intermediateTexture);

	tech.Set("LookupTableCB", GPU::Binding::CBuffer(lookupTableCB_, 0, sizeof(LookupTable)));
	tech.Set("inTexture", GPU::Binding::Texture2D(inTexture->GetHandle(), desc.format_, 0, 1));
	tech.Set("outTexture", GPU::Binding::RWTexture2D(intermediateTexture, uavFormat));
	if(auto binding = tech.GetBinding())
	{
		GPU::Box box;
		box.x_ = 0;
		box.y_ = 0;
		box.w_ = outTextureDesc.width_;
		box.h_ = outTextureDesc.height_;
		cmdList.Dispatch(binding, box.w_, box.h_, 1);
		cmdList.CopyTextureSubResource(outputTexture, 0, GPU::Point(), intermediateTexture, 0, box);
	}

	GPU::Manager::DestroyResource(intermediateTexture);
	return true;
}
