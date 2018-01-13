#pragma once

#include "image/dll.h"
#include "image/image.h"

namespace Core
{
	class File;
} // namespace Core

namespace Image
{
	/**
	 * Load image from a file.
	 */
	IMAGE_DLL Image Load(Core::File& file, ErrorHandlerFn errorHandlerFn = nullptr);

} // namespace Image
