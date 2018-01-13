#pragma once

#include "image/dll.h"
#include "core/types.h"
#include "core/function.h"
#include "gpu/types.h"

namespace Image
{
	/**
	 * List of file types supported for either read or write.
	 */
	enum class FileType : i32
	{
		UNKNOWN,
		DDS,
		BMP,
		PNG,
		TGA,
		JPEG,
		HDR,
	};

	/**
	 * Image type.
	 */
	using ImageType = GPU::TextureType;

	/**
	 * Supported formats for resources.
	 */
	using ImageFormat = GPU::Format;

	/**
	 * Error handling function.
	 */
	using ErrorHandlerFn = Core::Function<void(const char*)>;

} // namespace Image
