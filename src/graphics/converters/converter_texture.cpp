#include "graphics/texture.h"
#include "resource/converter.h"
#include "core/array.h"
#include "core/debug.h"
#include "core/file.h"
#include "core/vector.h"

#include "gpu/resources.h"
#include "gpu/utils.h"

#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4456)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#pragma warning(pop)

#include "squish.h"

#include <cstring>
#include <utility>

namespace
{
	struct Image
	{
		i32 width_ = 0;
		i32 height_ = 0;
		unsigned char* data_ = 0;
	};

	class ConverterTexture : public Resource::IConverter
	{
	public:
		ConverterTexture() {}

		virtual ~ConverterTexture() {}

		bool SupportsFileType(const char* fileExt, const Core::UUID& type) const override
		{
			return (type == Graphics::Texture::GetTypeUUID()) ||
			       (fileExt &&
			           (strcmp(fileExt, "png") == 0 || strcmp(fileExt, "jpg") == 0 || strcmp(fileExt, "tga") == 0));
		}

		bool Convert(Resource::IConverterContext& context, const char* sourceFile, const char* destPath) override
		{
			char file[Core::MAX_PATH_LENGTH];
			memset(file, 0, sizeof(file));
			if(!Core::FileSplitPath(sourceFile, nullptr, 0, file, sizeof(file), nullptr, 0))
			{
				context.AddError(__FILE__, __LINE__, "INTERNAL ERROR: Core::FileSplitPath failed.");
				return false;
			}

			char outFilename[Core::MAX_PATH_LENGTH];
			memset(outFilename, 0, sizeof(outFilename));
			strcat_s(outFilename, sizeof(outFilename), destPath);
			Core::FileNormalizePath(outFilename, sizeof(outFilename), true);

			// Load image with stb_image.
			Image image = LoadImage(context, sourceFile);

			if(image.data_ == nullptr)
			{
				context.AddError(__FILE__, __LINE__, "ERROR: Failed to load image."); // TODO: AddError needs varargs.
				return false;
			}

			context.AddDependency(sourceFile);

#if 0
			// Test compression.
			u8* outData = nullptr;
			i32 outSize = 0;
			EncodeAsBCn(image, GPU::Format::BC3_UNORM, outData, outSize);
#endif

			// TODO: Implement texture compression process.
			GPU::TextureDesc desc;
			desc.type_ = GPU::TextureType::TEX2D;
			desc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
			desc.format_ = GPU::Format::R8G8B8A8_UNORM;
			desc.width_ = image.width_;
			desc.height_ = image.height_;

			// Write out texture data.
			Core::File outFile(outFilename, Core::FileFlags::CREATE | Core::FileFlags::WRITE);
			if(outFile)
			{
				outFile.Write(&desc, sizeof(desc));
				i64 size = GPU::GetTextureSize(
				    desc.format_, desc.width_, desc.height_, desc.depth_, desc.levels_, desc.elements_);
				outFile.Write(image.data_, size);

				context.AddOutput(outFilename);

				return true;
			}

			return false;
		}

		Image LoadImage(Resource::IConverterContext& context, const char* sourceFile)
		{
			Core::File imageFile(sourceFile, Core::FileFlags::READ, context.GetPathResolver());
			Core::Vector<u8> imageData;
			imageData.resize((i32)imageFile.Size());
			imageFile.Read(imageData.data(), imageFile.Size());

			Image image;
			image.data_ = stbi_load_from_memory(
			    imageData.data(), imageData.size(), &image.width_, &image.height_, nullptr, STBI_rgb_alpha);
			return image;
		}

		void UnloadImage(Image& image)
		{
			stbi_image_free(image.data_);

			image.width_ = 0;
			image.height_ = 0;
			image.data_ = nullptr;
		}

		bool EncodeAsBCn(Image& image, GPU::Format format, u8*& outData, i32& outSize)
		{
			auto formatInfo = GPU::GetFormatInfo(format);

			if(format == GPU::Format::BC1_TYPELESS || format == GPU::Format::BC1_UNORM ||
			    format == GPU::Format::BC1_UNORM_SRGB || format == GPU::Format::BC2_TYPELESS ||
			    format == GPU::Format::BC2_UNORM || format == GPU::Format::BC2_UNORM_SRGB ||
			    format == GPU::Format::BC3_TYPELESS || format == GPU::Format::BC3_UNORM ||
			    format == GPU::Format::BC3_UNORM_SRGB || format == GPU::Format::BC4_TYPELESS ||
			    format == GPU::Format::BC4_UNORM || format == GPU::Format::BC4_SNORM ||
			    format == GPU::Format::BC5_TYPELESS || format == GPU::Format::BC5_UNORM ||
			    format == GPU::Format::BC5_SNORM)
			{
				u32 squishFormat = 0;

				switch(format)
				{
				case GPU::Format::BC1_TYPELESS:
				case GPU::Format::BC1_UNORM:
				case GPU::Format::BC1_UNORM_SRGB:
					squishFormat = squish::kBc1 | squish::kColourIterativeClusterFit;
					break;
				case GPU::Format::BC2_TYPELESS:
				case GPU::Format::BC2_UNORM:
				case GPU::Format::BC2_UNORM_SRGB:
					squishFormat = squish::kBc2 | squish::kColourIterativeClusterFit | squish::kWeightColourByAlpha;
					break;
				case GPU::Format::BC3_TYPELESS:
				case GPU::Format::BC3_UNORM:
				case GPU::Format::BC3_UNORM_SRGB:
					squishFormat = squish::kBc3 | squish::kColourIterativeClusterFit | squish::kWeightColourByAlpha;
					break;
				case GPU::Format::BC4_TYPELESS:
				case GPU::Format::BC4_UNORM:
				case GPU::Format::BC4_SNORM:
					squishFormat = squish::kBc4;
					break;
				case GPU::Format::BC5_TYPELESS:
				case GPU::Format::BC5_UNORM:
				case GPU::Format::BC5_SNORM:
					squishFormat = squish::kBc5;
					break;
				default:
					DBG_BREAK;
				}

				// Find out what space squish needs.
				outSize = squish::GetStorageRequirements(image.width_, image.height_, squishFormat);
				outData = new u8[outSize];

				// Squish takes RGBA8, so no need to convert before passing in.
				if(image.width_ >= 4 && image.height_ >= 4)
				{
					squish::CompressImage(
					    reinterpret_cast<squish::u8*>(image.data_), image.width_, image.height_, outData, squishFormat);
				}
				// If less than block size, copy into a 4x4 block.
				else if((image.width_ < 4 || image.height_ < 4) && !(image.width_ > 4 || image.height_ > 4))
				{
					Core::Array<u32, 4 * 4> block;

					// Copy into single block.
					for(i32 y = 0; y < image.height_; ++y)
					{
						for(i32 x = 0; x < image.width_; ++x)
						{
							i32 srcIdx = x + y * image.width_ * 4; // 4 bytes per pixel.
							i32 dstIdx = x + y * 4;
							memcpy(&block[dstIdx], &image.data_[srcIdx], 4);
						}
					}

					// Now encode.
					squish::CompressImage(reinterpret_cast<squish::u8*>(block.data()), 4, 4, outData, squishFormat);
				}
				else
				{
					Core::Log("ERROR: Image can't be encoded. (%ux%u)", image.width_, image.height_);
					delete[] outData;
					outData = nullptr;
					outSize = 0;
					return false;
				}

				//
				return true;
			}

			return false;
		}
	};
}


extern "C" {
EXPORT bool GetPlugin(struct Plugin::Plugin* outPlugin, Core::UUID uuid)
{
	bool retVal = false;

	// Fill in base info.
	if(uuid == Plugin::Plugin::GetUUID() || uuid == Resource::ConverterPlugin::GetUUID())
	{
		if(outPlugin)
		{
			outPlugin->systemVersion_ = Plugin::PLUGIN_SYSTEM_VERSION;
			outPlugin->pluginVersion_ = Resource::ConverterPlugin::PLUGIN_VERSION;
			outPlugin->uuid_ = Resource::ConverterPlugin::GetUUID();
			outPlugin->name_ = "Graphics.Texture Converter";
			outPlugin->desc_ = "Texture converter plugin.";
		}
		retVal = true;
	}

	// Fill in plugin specific.
	if(uuid == Resource::ConverterPlugin::GetUUID())
	{
		if(outPlugin)
		{
			auto* plugin = static_cast<Resource::ConverterPlugin*>(outPlugin);
			plugin->CreateConverter = []() -> Resource::IConverter* { return new ConverterTexture(); };
			plugin->DestroyConverter = [](Resource::IConverter*& converter) {
				delete converter;
				converter = nullptr;
			};
		}
		retVal = true;
	}

	return retVal;
}
}
