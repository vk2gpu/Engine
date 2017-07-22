#pragma once
#include "gpu/resources.h"

namespace Graphics
{
	static const i32 MAX_NAME_LENGTH = 64;

	struct ShaderHeader
	{
		/// Major version signifies a breaking change to the binary format.
		static const i16 MAJOR_VERSION = 0x0000;
		/// Mimor version signifies non-breaking change to binary format.
		static const i16 MINOR_VERSION = 0x0001;

		i16 majorVersion_ = MAJOR_VERSION;
		i16 minorVersion_ = MINOR_VERSION;

		i32 numCBuffers_ = 0;
		i32 numSamplers_ = 0;
		i32 numSRVs_ = 0;
		i32 numUAVs_ = 0;
		i32 numShaders_ = 0;
		i32 numRenderStates_ = 0;
		i32 numTechniques_ = 0;
	};

	struct ShaderBindingHeader
	{
		char name_[MAX_NAME_LENGTH];
	};

	struct ShaderBytecodeHeader
	{
		i32 numCBuffers_ = 0;
		i32 numSamplers_ = 0;
		i32 numSRVs_ = 0;
		i32 numUAVs_ = 0;
		i32 numBytes_ = 0;
	};

	struct ShaderBindingMapping
	{
		i32 binding_ = 0;
		i32 dstSlot_ = 0;
	};

	struct ShaderTechniqueHeader
	{
		char name_[MAX_NAME_LENGTH];
		i32 vs_;
		i32 gs_;
		i32 hs_;
		i32 ds_;
		i32 ps_;
		i32 cs_;
		i32 rs_;
	};

	struct ShaderImpl
	{
	};

} // namespace Graphics
