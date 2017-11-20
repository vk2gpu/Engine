#include "graphics/converters/dds.h"
#include "graphics/converters/image.h"
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

#include "serialization/serializer.h"

#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4456)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#pragma warning(pop)

#include "graphics/ispc/image_processing_ispc.h"

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
			Graphics::Image image = LoadImage(context, sourceFile);

			if(!image)
			{
				context.AddError(__FILE__, __LINE__, "ERROR: Failed to load image."); // TODO: AddError needs varargs.
				return false;
			}

			context.AddDependency(sourceFile);

			// If we get an R8G8B8A8 image in, we want to attempt to compress it
			// to an appropriate format.
			bool retVal = false;
			if(image.format_ == GPU::Format::R8G8B8A8_UNORM)
			{
				if(metaData.isInitialized_ == false)
				{
					metaData.format_ = GPU::Format::BC3_UNORM;
					metaData.generateMipLevels_ = true;
				}

				if(metaData.generateMipLevels_)
				{
					u32 maxDim = Core::Max(image.width_, Core::Max(image.height_, image.depth_));
					i32 levels = 32 - Core::CountLeadingZeros(maxDim);
					Graphics::Image lsImage(image.type_, GPU::Format::R32G32B32A32_FLOAT, image.width_, image.height_,
					    image.depth_, levels, nullptr, nullptr);
					Graphics::Image newImage(image.type_, image.format_, image.width_, image.height_, image.depth_,
					    levels, nullptr, nullptr);

					// Unpack into linear space.
					i32 numTexels = image.width_ * image.height_;
					ispc::ImageProc_Unpack_R8G8B8A8(
					    numTexels, image.GetData<ispc::Color_R8G8B8A8>(), lsImage.GetData<ispc::Color>());
					ispc::ImageProc_GammaToLinear(
					    image.width_, image.height_, lsImage.GetData<ispc::Color>(), lsImage.GetData<ispc::Color>());

					// Downsample all mip levels, conver to gamma space, then pack into new image.
					auto* currData = lsImage.GetData<ispc::Color>();
					auto* nextData = currData + (image.width_ * image.height_);
					auto* packedData = newImage.GetData<ispc::Color_R8G8B8A8>();
					i32 currW = image.width_;
					i32 currH = image.height_;
					for(i32 mip = 0; mip < levels; ++mip)
					{
						if(mip < (levels - 1))
							ispc::ImageProc_Downsample2x(currW, currH, currData, nextData);

						ispc::ImageProc_LinearToGamma(currW, currH, currData, currData);
						ispc::ImageProc_Pack_R8G8B8A8(numTexels, currData, packedData);

						packedData += numTexels;

						currW = Core::Max(1, currW >> 1);
						currH = Core::Max(1, currH >> 1);

						numTexels = currW * currH;
						currData = nextData;
						nextData = currData + numTexels;
					}

					std::swap(image, newImage);
				}

				auto formatInfo = GPU::GetFormatInfo(metaData.format_);
				if(formatInfo.blockW_ > 1 || formatInfo.blockH_ > 1)
				{
					Graphics::Image encodedImage = EncodeAsBCn(image, metaData.format_);
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
			desc.format_ = image.format_;
			desc.width_ = image.width_;
			desc.height_ = image.height_;
			desc.depth_ = (i16)image.depth_;
			desc.levels_ = (i16)image.levels_;

			retVal = WriteTexture(outFilename, desc, image.data_);

			if(retVal)
			{
				context.AddOutput(outFilename);
			}

			// Setup metadata.
			metaData.format_ = image.format_;
			context.SetMetaData(metaData);

			return retVal;
		}

		Graphics::Image LoadImage(Resource::IConverterContext& context, const char* sourceFile)
		{
			char fileExt[8] = {0};
			Core::FileSplitPath(sourceFile, nullptr, 0, nullptr, 0, fileExt, sizeof(fileExt));
			if(strcmp(fileExt, "dds") == 0)
			{
				return Graphics::DDS::LoadImage(context, sourceFile);
			}

			// Fall through to stb_image for loading other images.
			Core::File imageFile(sourceFile, Core::FileFlags::READ, context.GetPathResolver());
			Core::Vector<u8> imageData;
			imageData.resize((i32)imageFile.Size());
			imageFile.Read(imageData.data(), imageFile.Size());

			int w, h;
			u8* data = stbi_load_from_memory(imageData.data(), imageData.size(), &w, &h, nullptr, STBI_rgb_alpha);

			Graphics::Image image(GPU::TextureType::TEX2D, GPU::Format::R8G8B8A8_UNORM, w, h, 1, 1, data,
			    [](u8* data) { stbi_image_free(data); });

			return image;
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

		Graphics::Image EncodeAsBCn(const Graphics::Image& image, GPU::Format format)
		{
			auto formatInfo = GPU::GetFormatInfo(format);

			if(image.type_ != GPU::TextureType::TEX2D)
			{
				Core::Log("ERROR: Can only encode TEX2D as BC (for now)");
				return Graphics::Image();
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
					DBG_BREAK;
				}

				auto outImage = Graphics::Image(
				    image.type_, format, Core::PotRoundUp(image.width_, formatInfo.blockW_), Core::PotRoundUp(image.height_, formatInfo.blockH_), image.depth_, image.levels_, nullptr, nullptr);

				// Setup jobs.
				struct JobParams
				{
					i32 inOffset_ = 0;
					i32 outOffset_ = 0;
					i32 w_ = 0;
					i32 h_ = 0;
				};

				Core::Vector<JobParams> params;
				params.reserve(image.levels_);

				JobParams param;
				for(i32 mip = 0; mip < image.levels_; ++mip)
				{
					const i32 w = Core::Max(1, image.width_ >> mip);
					const i32 h = Core::Max(1, image.height_ >> mip);
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
					auto* inData = image.data_ + param.inOffset_;
					auto* outData = outImage.data_ + param.outOffset_;
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
						squish::CompressImage(reinterpret_cast<squish::u8*>(block.data()), rw, rh, outData, squishFormat);
					}
					else
					{
						Core::Log("ERROR: Image can't be encoded. (%ux%u)", w, h);
					}
				});

				Job::Counter* counter = nullptr;
				encodeJob.RunMultiple(0, image.levels_ - 1, &counter);
				Job::Manager::WaitForCounter(counter, 0);

				return outImage;
			}

			return Graphics::Image();
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
