#include "image/save.h"
#include "image/private/dds.h"

#include "core/file.h"

#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4456)
#pragma warning(disable : 4996)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#pragma warning(pop)

namespace Image
{
	bool Save(Core::File& file, const Image& image, FileType fileType)
	{
		auto writeFn = [](void* context, void* data, int size) {
			Core::File* file = (Core::File*)(context);
			file->Write(data, size);
		};

		// TODO: Convert image to appropriate pixel format if required.
		switch(fileType)
		{
		case FileType::BMP:
			return 0 ==
			       stbi_write_bmp_to_func(
			           writeFn, &file, image.GetWidth(), image.GetHeight(), 4, image.GetMipBaseAddr(0));
			break;
		case FileType::PNG:
			return 0 ==
			       stbi_write_png_to_func(writeFn, &file, image.GetWidth(), image.GetHeight(), 4,
			           image.GetMipBaseAddr(0), image.GetWidth() * sizeof(u32));
			break;
		case FileType::TGA:
			return 0 ==
			       stbi_write_tga_to_func(
			           writeFn, &file, image.GetWidth(), image.GetHeight(), 4, image.GetMipBaseAddr(0));
			break;
		case FileType::HDR:
			return 0 ==
			       stbi_write_hdr_to_func(
			           writeFn, &file, image.GetWidth(), image.GetHeight(), 4, (const float*)image.GetMipBaseAddr(0));
			break;

		default:
			return false;
		}
	}

} // Image
