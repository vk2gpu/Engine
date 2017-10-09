#pragma once

#include "gpu/types.h"
#include "serialization/serializer.h"

namespace Graphics
{
	struct MetaDataTexture
	{
		bool isInitialized_ = false;
		GPU::Format format_ = GPU::Format::INVALID;
		bool generateMipLevels_ = false;

		bool Serialize(Serialization::Serializer& serializer)
		{
			isInitialized_ = true;

			bool retVal = true;
			retVal &= serializer.Serialize("format", format_);
			retVal &= serializer.Serialize("generateMipLevels", generateMipLevels_);
			return retVal;
		}
	};
} // namespace Graphics
