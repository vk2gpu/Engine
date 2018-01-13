#include "image/image.h"
#include "image/load.h"
#include "image/private/dds.h"
#include "core/file.h"

#define STBI_FAILURE_USERMSG
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4456)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#pragma warning(pop)

namespace Image
{
	Image Load(Core::File& file, ErrorHandlerFn errorHandlerFn)
	{
		const i64 pos = file.Tell();

		// Attempt to load as DDS.
		if(Image image = DDS::LoadImage(file))
			return image;

		file.Seek(pos);

		Core::Vector<u8> imageData;
		imageData.resize((i32)file.Size());
		file.Read(imageData.data(), file.Size());

		// Try loading as non-HDR image.
		int w, h;
		u8* data = stbi_load_from_memory(imageData.data(), imageData.size(), &w, &h, nullptr, STBI_rgb_alpha);

		if(data)
		{
			return Image(ImageType::TEX2D, ImageFormat::R8G8B8A8_UNORM, w, h, 1, 1, data,
			    [](u8* data) { stbi_image_free(data); });
		}

		// Try loading as HDR image.
		f32* dataf = stbi_loadf_from_memory(imageData.data(), imageData.size(), &w, &h, nullptr, STBI_rgb_alpha);

		if(dataf)
		{
			return Image(ImageType::TEX2D, ImageFormat::R32G32B32A32_FLOAT, w, h, 1, 1, (u8*)dataf,
			    [](u8* data) { stbi_image_free(data); });
		}

		return Image();
	}


} // Image
