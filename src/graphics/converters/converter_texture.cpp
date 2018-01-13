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

#include <squish.h>

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
			       (fileExt &&
			           (strcmp(fileExt, "png") == 0 || strcmp(fileExt, "jpg") == 0 || strcmp(fileExt, "tga") == 0 ||
			               strcmp(fileExt, "dds") == 0));
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
					metaData.format_ = GPU::Format::BC3_UNORM;
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
					Image::Image encodedImage = EncodeAsBCn(image, metaData.format_);
					if(encodedImage)
					{
						image = std::move(encodedImage);
					}
				}
			}
			else
			{
				metaData.generateMipLevels_ = false;
			}

			GPU::TextureDesc desc;
			desc.type_ = GPU::TextureType::TEX2D;
			desc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
			desc.format_ = image.GetFormat();
			desc.width_ = image.GetWidth();
			desc.height_ = image.GetHeight();
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
			Core::File imageFile(sourceFile, Core::FileFlags::READ, context.GetPathResolver());

			return Image::Load(
			    imageFile, [&context](const char* errorMsg) { context.AddError(__FILE__, __LINE__, errorMsg); });
		}

		bool WriteTexture(const char* outFilename, const GPU::TextureDesc& desc, const u8* data)
		{
			// Write out texture data.
			Core::File outFile(outFilename, Core::FileFlags::CREATE | Core::FileFlags::WRITE);
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

		Image::Image EncodeAsBCn(const Image::Image& image, GPU::Format format)
		{
			auto formatInfo = GPU::GetFormatInfo(format);

			if(image.GetType() != GPU::TextureType::TEX2D)
			{
				Core::Log("ERROR: Can only encode TEX2D as BC (for now)");
				return Image::Image();
			}

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
					DBG_ASSERT(false);
				}

				auto outImage =
				    Image::Image(image.GetType(), format, Core::PotRoundUp(image.GetWidth(), formatInfo.blockW_),
				        Core::PotRoundUp(image.GetHeight(), formatInfo.blockH_), image.GetDepth(), image.GetLevels(),
				        nullptr, nullptr);

				// Setup jobs.
				struct JobParams
				{
					i32 inOffset_ = 0;
					i32 outOffset_ = 0;
					i32 w_ = 0;
					i32 h_ = 0;
				};

				Core::Vector<JobParams> params;
				params.reserve(image.GetLevels());

				JobParams param;
				for(i32 mip = 0; mip < image.GetLevels(); ++mip)
				{
					const i32 w = Core::Max(1, image.GetWidth() >> mip);
					const i32 h = Core::Max(1, image.GetHeight() >> mip);
					const i32 bw = Core::PotRoundUp(w, formatInfo.blockW_) / formatInfo.blockW_;
					const i32 bh = Core::PotRoundUp(h, formatInfo.blockH_) / formatInfo.blockH_;

					param.w_ = w;
					param.h_ = h;

					params.push_back(param);

					param.inOffset_ += (param.w_ * param.h_ * 4);
					param.outOffset_ += (bw * bh * formatInfo.blockBits_) / 8;
				}

				Job::FunctionJob encodeJob("Encode Job", [&params, squishFormat, &image, &outImage](i32 idx) {
					auto param = params[idx];
					auto* inData = image.GetMipData<u8>(0) + param.inOffset_;
					auto* outData = outImage.GetMipData<u8>(0) + param.outOffset_;
					auto w = param.w_;
					auto h = param.h_;

					// Squish takes RGBA8, so no need to convert before passing in.
					if(w >= 4 && h >= 4)
					{
						squish::CompressImage(inData, w, h, outData, squishFormat);
					}
					// If less than block size, copy into a the appropriate block size block.
					else if(w < 4 || h < 4)
					{
						auto rw = Core::PotRoundUp(w, 4);
						auto rh = Core::PotRoundUp(h, 4);

						Core::Vector<u32> block(rw * rh);

						// Copy into single block.
						for(i32 y = 0; y < h; ++y)
						{
							for(i32 x = 0; x < w; ++x)
							{
								i32 srcIdx = x + y * rw * 4; // 4 bytes per pixel.
								i32 dstIdx = x + y * 4;
								memcpy(&block[dstIdx], &inData[srcIdx], 4);
							}
						}

						// Now encode.
						squish::CompressImage(
						    reinterpret_cast<squish::u8*>(block.data()), rw, rh, outData, squishFormat);
					}
					else
					{
						Core::Log("ERROR: Image can't be encoded. (%ux%u)", w, h);
					}
				});

				Job::Counter* counter = nullptr;
				encodeJob.RunMultiple(0, image.GetLevels() - 1, &counter);
				Job::Manager::WaitForCounter(counter, 0);

				return outImage;
			}

			return Image::Image();
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
