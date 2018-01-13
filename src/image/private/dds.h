#pragma once

#include "core/types.h"
#include "core/function.h"
#include "gpu/fwd_decls.h"
#include "image/image.h"

namespace Core
{
	class File;
} // namespace Core

namespace Image
{
	namespace DDS
	{
		/// First 4 expected bytes of a DDS.
		static const u32 HEADER_MAGIC_ID = 0x20534444;

		using ErrorHandlerFn = Core::Function<void(const char* /* errorMsg */)>;

		/**
		 * Load an image from a DDS file.
		 */
		Image LoadImage(Core::File& imageFile, ErrorHandlerFn errorHandlerFn = nullptr);

	} // namespace DDS
} // namespace Image
