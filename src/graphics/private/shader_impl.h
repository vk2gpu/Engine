#pragma once
#include "gpu/resources.h"

namespace Graphics
{
	struct ShaderHeader
	{
		/// Major version signifies a breaking change to the binary format.
		static const i16 MAJOR_VERSION = 0x0000;
		/// Mimor version signifies non-breaking change to binary format.
		static const i16 MINOR_VERSION = 0x0001;

		i16 majorVersion_ = MAJOR_VERSION;
		i16 minorVersion_ = MINOR_VERSION;
	};

	struct ShaderImpl
	{
	};

} // namespace Graphics
