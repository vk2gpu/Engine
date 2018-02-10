#include "graphics/converters/import_texture.h"
#include "graphics/texture.h"
#include "resource/converter.h"
#include "core/array.h"
#include "core/debug.h"
#include "core/enum.h"
#include "core/file.h"
#include "core/hash.h"
#include "core/misc.h"
#include "core/vector.h"

#include "gpu/enum.h"
#include "gpu/resources.h"
#include "gpu/utils.h"

#include "job/function_job.h"
#include "job/manager.h"

#include "image/image.h"
#include "image/load.h"
#include "image/process.h"

#include "serialization/serializer.h"

#include <cstring>
#include <utility>

namespace
{
	class ConverterTexture : public Resource::IConverter
	{
	public:
		ConverterTexture() {}

		virtual ~ConverterTexture() {}

		bool SupportsFileType(const char* fileExt, const Core::UUID& type) const override
		{
			return (type == Graphics::Texture::GetTypeUUID()) ||
			       (fileExt && (strcmp(fileExt, "png") == 0 || strcmp(fileExt, "jpg") == 0 ||
			                       strcmp(fileExt, "tga") == 0 || strcmp(fileExt, "dds") == 0));
		}

		bool Convert(Resource::IConverterContext& context, const char* sourceFile, const char* destPath) override
		{
			auto metaData = context.GetMetaData<Graphics::MetaDataTexture>();

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
			Image::Image image = LoadImage(context, sourceFile);

			if(!image)
			{
				context.AddError(__FILE__, __LINE__, "ERROR: Failed to load image."); // TODO: AddError needs varargs.
				return false;
			}

			context.AddDependency(sourceFile);

			// If we get an R8G8B8A8 image in, we want to attempt to compress it
			// to an appropriate format.
			bool retVal = false;
			if(image.GetFormat() == GPU::Format::R8G8B8A8_UNORM)
			{
				if(metaData.isInitialized_ == false)
				{
					metaData.format_ = GPU::Format::BC7_UNORM;
					metaData.generateMipLevels_ = true;
				}

				if(metaData.generateMipLevels_)
				{
					u32 maxDim = Core::Max(image.GetWidth(), Core::Max(image.GetHeight(), image.GetDepth()));
					i32 levels = 32 - Core::CountLeadingZeros(maxDim);
					Image::Image lsImage(image.GetType(), GPU::Format::R32G32B32A32_FLOAT, image.GetWidth(),
					    image.GetHeight(), image.GetDepth(), levels, nullptr, nullptr);
					Image::Image newImage(image.GetType(), image.GetFormat(), image.GetWidth(), image.GetHeight(),
					    image.GetDepth(), levels, nullptr, nullptr);

					bool success = true;

					// Unpack to floating point.
					success &= Image::Convert(lsImage, image, Image::ImageFormat::R32G32B32A32_FLOAT);
					DBG_ASSERT(success);

					// Convert to linear space.
					success &= Image::GammaToLinear(lsImage, lsImage);
					DBG_ASSERT(success);

					// Generate mips.
					success &= Image::GenerateMips(lsImage, lsImage);
					DBG_ASSERT(success);

					// Convert to gamma space.
					success &= Image::LinearToGamma(lsImage, lsImage);
					DBG_ASSERT(success);

					// Pack back to 32-bit RGBA.
					success &= Image::Convert(newImage, lsImage, Image::ImageFormat::R8G8B8A8_UNORM);
					DBG_ASSERT(success);

					std::swap(image, newImage);
				}

				auto formatInfo = GPU::GetFormatInfo(metaData.format_);
				if(formatInfo.blockW_ > 1 || formatInfo.blockH_ > 1)
				{
					Image::Image encodedImage;
					// TODO: Use better than VERY_LOW in tools.
					if(Image::Convert(encodedImage, image, metaData.format_, Image::ConvertQuality::VERY_LOW))
						image = std::move(encodedImage);
				}
			}
			else
			{
				metaData.generateMipLevels_ = false;
			}

			const auto formatInfo = GPU::GetFormatInfo(image.GetFormat());

			GPU::TextureDesc desc;
			desc.type_ = GPU::TextureType::TEX2D;
			desc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
			desc.format_ = image.GetFormat();
			desc.width_ = Core::Max(formatInfo.blockW_, image.GetWidth());
			desc.height_ = Core::Max(formatInfo.blockH_, image.GetHeight());
			desc.depth_ = (i16)image.GetDepth();
			desc.levels_ = (i16)image.GetLevels();

			retVal = WriteTexture(outFilename, desc, image.GetMipData<u8>(0));

			if(retVal)
			{
				context.AddOutput(outFilename);
			}

			// Setup metadata.
			metaData.format_ = image.GetFormat();
			context.SetMetaData(metaData);

			return retVal;
		}

		Image::Image LoadImage(Resource::IConverterContext& context, const char* sourceFile)
		{
			char fileExt[8] = {0};
			Core::FileSplitPath(sourceFile, nullptr, 0, nullptr, 0, fileExt, sizeof(fileExt));
			Core::File imageFile(sourceFile, Core::FileFlags::DEFAULT_READ, context.GetPathResolver());

			return Image::Load(
			    imageFile, [&context](const char* errorMsg) { context.AddError(__FILE__, __LINE__, errorMsg); });
		}

		bool WriteTexture(const char* outFilename, const GPU::TextureDesc& desc, const u8* data)
		{
			// Write out texture data.
			Core::File outFile(outFilename, Core::FileFlags::DEFAULT_WRITE);
			if(outFile)
			{
				outFile.Write(&desc, sizeof(desc));
				i64 size = GPU::GetTextureSize(
				    desc.format_, desc.width_, desc.height_, desc.depth_, desc.levels_, desc.elements_);
				outFile.Write(data, size);

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
