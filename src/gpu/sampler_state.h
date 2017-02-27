#pragma once

#include "gpu/dll.h"
#include "core/float.h"
#include "core/types.h"

namespace GPU
{
	/**
	 * Sampler types.
	 */
	enum class AddressingMode : u32
	{
		WRAP = 0,
		MIRROR,
		CLAMP,
		BORDER,

		MAX,
	};

	enum class FilteringMode : u32
	{
		NEAREST = 0,
		LINEAR,
		NEAREST_MIPMAP_NEAREST,
		LINEAR_MIPMAP_NEAREST,
		NEAREST_MIPMAP_LINEAR,
		LINEAR_MIPMAP_LINEAR,

		MAX,
	};

	/**
	 * Sampler state.
	 */
	struct SamplerState
	{
		AddressingMode AddressU_ = AddressingMode::WRAP;
		AddressingMode AddressV_ = AddressingMode::WRAP;
		AddressingMode AddressW_ = AddressingMode::WRAP;
		FilteringMode MinFilter_ = FilteringMode::NEAREST;
		FilteringMode MagFilter_ = FilteringMode::NEAREST;
		f32 MipLODBias_ = 0.0f;
		u32 MaxAnisotropy_ = 1;
		float BorderColour_[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		f32 MinLOD_ = -Core::F32_MAX;
		f32 MaxLOD_ = Core::F32_MAX;
	};

} // namespace GPU
