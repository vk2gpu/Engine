#pragma once

#include "image/dll.h"
#include "image/types.h"
#include "image/image.h"

namespace Core
{
	class File;
} // namespace Core

namespace Image
{
	/**
	 * Save image to a file.
	 * @return true on success, false on failure (i.e. @a fileType is not supported).
	 */
	IMAGE_DLL bool Save(Core::File& file, const Image& image, FileType fileType);

} // namespace Image
