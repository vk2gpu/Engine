#pragma once

#include "gpu/dll.h"
#include "core/types.h"
#include "core/handle.h"

namespace Core
{
	class Handle;
}

namespace GPU
{
	/**
	 * Resource bind flags.
	 */
	enum class BindFlags : u32
	{
		NONE = 0x00000000,
		VERTEX_BUFFER = 0x00000001,
		INDEX_BUFFER = 0x00000002,
		CONSTANT_BUFFER = 0x00000004,
		SHADER_RESOURCE = 0x00000008,
		STREAM_OUTPUT = 0x00000010,
		RENDER_TARGET = 0x00000020,
		DEPTH_STENCIL = 0x00000040,
		UNORDERED_ACCESS = 0x00000080,
		PRESENT = 0x00000100,
	};

	DEFINE_ENUM_CLASS_FLAG_OPERATOR(BindFlags, &);
	DEFINE_ENUM_CLASS_FLAG_OPERATOR(BindFlags, |);


	/**
	 * Texture types.
	 */
	enum class TextureType : i32
	{
		INVALID = -1,
		TEX1D = 0,
		TEX2D,
		TEX3D,
		TEXCUBE,
	};

	/**
	 * View dimension.
	 */
	enum class ViewDimension : i32
	{
		INVALID = -1,
		BUFFER = 0,
		TEX1D,
		TEX1D_ARRAY,
		TEX2D,
		TEX2D_ARRAY,
		TEX3D,
		TEXCUBE,
		TEXCUBE_ARRAY,
	};


	/**
	 * Scissor rect.
	 */
	struct ScissorRect
	{
		i32 x_ = 0;
		i32 y_ = 0;
		i32 w_ = 0;
		i32 h_ = 0;
	};

	/**
	 * Viewport.
	 * Must be inside of the render target.
	 */
	struct Viewport
	{
		f32 x_ = 0.0f;
		f32 y_ = 0.0f;
		f32 w_ = 0.0f;
		f32 h_ = 0.0f;
		f32 zMin_ = 0.0f;
		f32 zMax_ = 1.0f;
	};
};
