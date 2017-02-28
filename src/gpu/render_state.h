#pragma once

#include "gpu/dll.h"
#include "gpu/types.h"

namespace GPU
{
	/**
	 * Render state types.
	 */
	enum class FillMode : i32
	{
		INVALID = -1,
		SOLID = 0,
		WIREFRAME,

		MAX,
	};

	enum class CullMode : i32
	{
		INVALID = -1,
		NONE = 0,
		CCW,
		CW,

		MAX,
	};

	enum class BlendType : i32
	{
		INVALID = -1,
		ZERO = 0,
		ONE,
		SRC_COLOUR,
		INV_SRC_COLOUR,
		SRC_ALPHA,
		INV_SRC_ALPHA,
		DEST_COLOUR,
		INV_DEST_COLOUR,
		DEST_ALPHA,
		INV_DEST_ALPHA,

		MAX,
	};

	enum class BlendFunc : i32
	{
		INVALID = -1,
		ADD = 0,
		SUBTRACT,
		REV_SUBTRACT,
		MINIMUM,
		MAXIMUM,

		MAX,
	};

	enum class CompareMode : i32
	{
		INVALID = -1,
		NEVER = 0,
		LESS,
		EQUAL,
		LESSEQUAL,
		GREATER,
		NOTEQUAL,
		GREATEREQUAL,
		ALWAYS,

		MAX,
	};

	enum class StencilFunc : i32
	{
		INVALID = -1,
		KEEP = 0,
		ZERO,
		REPLACE,
		INCR,
		INCR_WRAP,
		DECR,
		DECR_WRAP,
		INVERT,

		MAX,
	};

	/**
	 * Blend state.
	 * One for each RT.
	 */
	struct BlendState
	{
		u32 Enable_ = 0;
		BlendType SrcBlend_ = BlendType::ONE;
		BlendType DestBlend_ = BlendType::ONE;
		BlendFunc BlendOp_ = BlendFunc::ADD;
		BlendType SrcBlendAlpha_ = BlendType::ONE;
		BlendType DestBlendAlpha_ = BlendType::ONE;
		BlendFunc BlendOpAlpha_ = BlendFunc::ADD;
		u32 WriteMask_ = 0xf;
	};

	/**
	 * Stencil face state.
	 * One front, one back.
	 */
	struct StencilFaceState
	{
		StencilFunc Fail_ = StencilFunc::KEEP;
		StencilFunc DepthFail_ = StencilFunc::KEEP;
		StencilFunc Pass_ = StencilFunc::KEEP;
		CompareMode Func_ = CompareMode::ALWAYS;
		u32 Mask_ = 0;
	};

	/**
	 * Render state.
	 */
	struct RenderState
	{
		// Blend state.
		BlendState blendStates_[MAX_BOUND_RTVS];

		// Depth stencil.
		StencilFaceState StencilFront_;
		StencilFaceState StencilBack_;
		u32 DepthTestEnable_ = 0;
		u32 DepthWriteEnable_ = 0;
		CompareMode DepthFunc_ = CompareMode::GREATEREQUAL;
		u32 StencilEnable_ = 0;
		u32 StencilRef_ = 0;
		u8 StencilRead_ = 0;
		u8 StencilWrite_ = 0;

		// Rasterizer.
		FillMode FillMode_ = FillMode::SOLID;
		CullMode CullMode_ = CullMode::CCW;
		f32 DepthBias_ = 0.0f;
		f32 SlopeScaledDepthBias_ = 0.0f;
		u32 DepthClipEnable_ = 0;
		u32 AntialiasedLineEnable_ = 0;
	};

} // namespace GPU
