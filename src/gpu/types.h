#pragma once

#include "gpu/dll.h"
#include "core/types.h"
#include "core/float.h"
#include "core/handle.h"

namespace Core
{
	class Handle;
} // namespace Core

namespace GPU
{
	/**
	 * Constants.
	 */
	/// Number of frames to buffer ahead.
	static const i32 MAX_GPU_FRAMES = 4;
	/// Maximum number of RTVs that can be bound simultaneously.
	static const i32 MAX_BOUND_RTVS = 8;
	/// Maximum number of vertex elements in a pipeline state.
	static const i32 MAX_VERTEX_ELEMENTS = 16;
	/// Maximum number of vertex streams in the draw binding set.
	static const i32 MAX_VERTEX_STREAMS = 16;
	/// Maximum number of SRV bindings.
	static const i32 MAX_SRV_BINDINGS = 16;
	/// Maximum number of UAV bindings.
	static const i32 MAX_UAV_BINDINGS = 8;
	/// Maximum number of CBV bindings.
	static const i32 MAX_CBV_BINDINGS = 14;
	/// Maximum number of sampler bindings.
	static const i32 MAX_SAMPLER_BINDINGS = 16;
	/// Resource data alignment.
	static const i32 RESOURCE_DATA_ALIGNMENT = 256;

	/**
	 * Error codes.
	 */
	enum class ErrorCode : i32
	{
		OK = 0,
		FAIL = -1,
		UNIMPLEMENTED = -2,
		UNSUPPORTED = -3,
	};

/**
 * Utilities for handling error codes.
 */
#define RETURN_ON_ERROR(ERRORCODE)                                                                                     \
	{                                                                                                                  \
		auto errorCode_Internal = ERRORCODE;                                                                           \
		if(errorCode_Internal != ErrorCode::OK)                                                                        \
			return errorCode_Internal;                                                                                 \
	}

	/**
	 * Debug flags.
	 */
	enum class DebugFlags : u32
	{
		/// No debugger support.
		NONE = 0x0,

		/// Enable all warnings that may be disabled by default.
		ENABLE_ALL_WARNINGS = 0x1,

		/// Enable GPU based validation.
		GPU_BASED_VALIDATION = 0x2,

		/// Enable RenderDoc integration.
		RENDERDOC = 0x4,
	};

	DEFINE_ENUM_CLASS_FLAG_OPERATOR(DebugFlags, &);
	DEFINE_ENUM_CLASS_FLAG_OPERATOR(DebugFlags, |);

	/**
	 * Supported formats for resources.
	 */
	enum class Format : i32
	{
		INVALID = -1,
		R32G32B32A32_TYPELESS = 0,
		R32G32B32A32_FLOAT,
		R32G32B32A32_UINT,
		R32G32B32A32_SINT,
		R32G32B32_TYPELESS,
		R32G32B32_FLOAT,
		R32G32B32_UINT,
		R32G32B32_SINT,
		R16G16B16A16_TYPELESS,
		R16G16B16A16_FLOAT,
		R16G16B16A16_UNORM,
		R16G16B16A16_UINT,
		R16G16B16A16_SNORM,
		R16G16B16A16_SINT,
		R32G32_TYPELESS,
		R32G32_FLOAT,
		R32G32_UINT,
		R32G32_SINT,
		R32G8X24_TYPELESS,
		D32_FLOAT_S8X24_UINT,
		R32_FLOAT_X8X24_TYPELESS,
		X32_TYPELESS_G8X24_UINT,
		R10G10B10A2_TYPELESS,
		R10G10B10A2_UNORM,
		R10G10B10A2_UINT,
		R11G11B10_FLOAT,
		R8G8B8A8_TYPELESS,
		R8G8B8A8_UNORM,
		R8G8B8A8_UNORM_SRGB,
		R8G8B8A8_UINT,
		R8G8B8A8_SNORM,
		R8G8B8A8_SINT,
		R16G16_TYPELESS,
		R16G16_FLOAT,
		R16G16_UNORM,
		R16G16_UINT,
		R16G16_SNORM,
		R16G16_SINT,
		R32_TYPELESS,
		D32_FLOAT,
		R32_FLOAT,
		R32_UINT,
		R32_SINT,
		R24G8_TYPELESS,
		D24_UNORM_S8_UINT,
		R24_UNORM_X8_TYPELESS,
		X24_TYPELESS_G8_UINT,
		R8G8_TYPELESS,
		R8G8_UNORM,
		R8G8_UINT,
		R8G8_SNORM,
		R8G8_SINT,
		R16_TYPELESS,
		R16_FLOAT,
		D16_UNORM,
		R16_UNORM,
		R16_UINT,
		R16_SNORM,
		R16_SINT,
		R8_TYPELESS,
		R8_UNORM,
		R8_UINT,
		R8_SNORM,
		R8_SINT,
		A8_UNORM,
		R1_UNORM,
		R9G9B9E5_SHAREDEXP,
		R8G8_B8G8_UNORM,
		G8R8_G8B8_UNORM,
		BC1_TYPELESS,
		BC1_UNORM,
		BC1_UNORM_SRGB,
		BC2_TYPELESS,
		BC2_UNORM,
		BC2_UNORM_SRGB,
		BC3_TYPELESS,
		BC3_UNORM,
		BC3_UNORM_SRGB,
		BC4_TYPELESS,
		BC4_UNORM,
		BC4_SNORM,
		BC5_TYPELESS,
		BC5_UNORM,
		BC5_SNORM,
		B5G6R5_UNORM,
		B5G5R5A1_UNORM,
		B8G8R8A8_UNORM,
		B8G8R8X8_UNORM,
		R10G10B10_XR_BIAS_A2_UNORM,
		B8G8R8A8_TYPELESS,
		B8G8R8A8_UNORM_SRGB,
		B8G8R8X8_TYPELESS,
		B8G8R8X8_UNORM_SRGB,
		BC6H_TYPELESS,
		BC6H_UF16,
		BC6H_SF16,
		BC7_TYPELESS,
		BC7_UNORM,
		BC7_UNORM_SRGB,
		ETC1_UNORM,
		ETC2_UNORM,
		ETC2A_UNORM,
		ETC2A1_UNORM,

		MAX,
	};

	/**
	 * Resource bind flags.
	 */
	enum class BindFlags : u32
	{
		NONE = 0x00000000,
		VERTEX_BUFFER = 0x00000001,
		INDEX_BUFFER = 0x00000002,
		CONSTANT_BUFFER = 0x00000004,
		INDIRECT_BUFFER = 0x00000008,
		SHADER_RESOURCE = 0x00000010,
		STREAM_OUTPUT = 0x00000020,
		RENDER_TARGET = 0x00000040,
		DEPTH_STENCIL = 0x00000080,
		UNORDERED_ACCESS = 0x00000100,
		PRESENT = 0x00000200,
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
	 * Shader type.
	 */
	enum class ShaderType : i32
	{
		INVALID = -1,
		VS = 0,
		GS,
		HS,
		DS,
		PS,
		CS,
		MAX
	};

	/**
	 * Topology type.
	 */
	enum class TopologyType : i32
	{
		INVALID = -1,
		POINT = 0,
		LINE,
		TRIANGLE,
		PATCH,
		MAX
	};

	/**
	 * Primitive topology.
	 */
	enum class PrimitiveTopology : i32
	{
		INVALID = -1,
		POINT_LIST = 0,
		LINE_LIST,
		LINE_STRIP,
		LINE_LIST_ADJ,
		LINE_STRIP_ADJ,
		TRIANGLE_LIST,
		TRIANGLE_STRIP,
		TRIANGLE_LIST_ADJ,
		TRIANGLE_STRIP_ADJ,
		PATCH_LIST,
		MAX
	};

	/**
	 * Vertex usage.
	 */
	enum class VertexUsage : i32
	{
		INVALID = -1,
		POSITION = 0,
		BLENDWEIGHTS,
		BLENDINDICES,
		NORMAL,
		TEXCOORD,
		TANGENT,
		BINORMAL,
		COLOR,

		MAX
	};

	/**
	 * Sampler types.
	 */
	enum class AddressingMode : i32
	{
		INVALID = -1,
		WRAP = 0,
		MIRROR,
		CLAMP,
		BORDER,

		MAX,
	};

	enum class FilteringMode : i32
	{
		INVALID = -1,
		NEAREST = 0,
		LINEAR,
		NEAREST_MIPMAP_NEAREST,
		LINEAR_MIPMAP_NEAREST,
		NEAREST_MIPMAP_LINEAR,
		LINEAR_MIPMAP_LINEAR,

		MAX,
	};

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
		SRC_COLOR,
		INV_SRC_COLOR,
		SRC_ALPHA,
		INV_SRC_ALPHA,
		DEST_COLOR,
		INV_DEST_COLOR,
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
		LESS_EQUAL,
		GREATER,
		NOT_EQUAL,
		GREATER_EQUAL,
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
	 * DSV flags.
	 */
	enum class DSVFlags : u32
	{
		NONE = 0x0,
		READ_ONLY_DEPTH = 0x1,
		READ_ONLY_STENCIL = 0x2
	};

	DEFINE_ENUM_CLASS_FLAG_OPERATOR(DSVFlags, &);
	DEFINE_ENUM_CLASS_FLAG_OPERATOR(DSVFlags, |);

	/**
	 * Setup parameters.
	 */
	struct GPU_DLL SetupParams
	{
		/// API selection (i.e. "D3D12", "VLK", etc)
		const char* api_ = nullptr;
		/// Device window to use.
		void* deviceWindow_ = nullptr;
		/// Debuggers to natively support integration of.
		DebugFlags debugFlags_ = DebugFlags::NONE;
	};

	/**
	 * Adapter info.
	 */
	struct GPU_DLL AdapterInfo
	{
		i32 deviceIdx_ = 0;
		char description_[512];
		u32 vendorId_;
		u32 deviceId_;
		u32 subSysId_;
		u32 revision_;
		i64 dedicatedVideoMemory_;
		i64 dedicatedSystemMemory_;
		i64 sharedSystemMemory_;
	};

	/**
	 * Box.
	 */
	struct GPU_DLL Box
	{
		i32 x_ = 0;
		i32 y_ = 0;
		i32 z_ = 0;
		i32 w_ = 1;
		i32 h_ = 1;
		i32 d_ = 1;
	};

	/**
	 * Point.
	 */
	struct GPU_DLL Point
	{
		i32 x_ = 0;
		i32 y_ = 0;
		i32 z_ = 0;
	};

	/**
	 * Vertex element.
	 */
	struct GPU_DLL VertexElement
	{
		VertexElement() = default;
		VertexElement(i32 streamIdx, i32 offset, Format format, VertexUsage usage, i32 usageIdx)
		    : streamIdx_(streamIdx)
		    , offset_(offset)
		    , format_(format)
		    , usage_(usage)
		    , usageIdx_(usageIdx)
		{
		}

		i32 streamIdx_ = -1;
		i32 offset_ = -1;
		Format format_ = Format::INVALID;
		VertexUsage usage_ = VertexUsage::INVALID;
		i32 usageIdx_ = -1;
	};

	/**
	 * Scissor rect.
	 */
	struct GPU_DLL ScissorRect
	{
		i32 x_ = 0;
		i32 y_ = 0;
		i32 w_ = 0;
		i32 h_ = 0;

		bool operator==(const ScissorRect& other) const
		{
			return x_ == other.x_ && y_ == other.y_ && w_ == other.w_ && h_ == other.h_;
		}

		bool operator!=(const ScissorRect& other) const { return !(*this == other); }
	};

	/**
	 * Viewport.
	 * Must be inside of the render target.
	 */
	struct GPU_DLL Viewport
	{
		f32 x_ = 0.0f;
		f32 y_ = 0.0f;
		f32 w_ = 0.0f;
		f32 h_ = 0.0f;
		f32 zMin_ = 0.0f;
		f32 zMax_ = 1.0f;

		bool operator==(const Viewport& other) const
		{
			return Core::CompareFloat(x_, other.x_) && Core::CompareFloat(y_, other.y_) &&
			       Core::CompareFloat(w_, other.w_) && Core::CompareFloat(h_, other.h_) &&
			       Core::CompareFloat(zMin_, other.zMin_) && Core::CompareFloat(zMax_, other.zMax_);
		}

		bool operator!=(const Viewport& other) const { return !(*this == other); }
	};

	/**
	 * Draw state.
	 */
	struct GPU_DLL DrawState
	{
		/// Viewport.
		Viewport viewport_;
		/// Scissor rect.
		ScissorRect scissorRect_;
		/// Stencil ref.
		u8 stencilRef_ = 0;

		// Comparison.
		bool operator==(const DrawState& other) const
		{
			return viewport_ == other.viewport_ && scissorRect_ == other.scissorRect_ &&
			       stencilRef_ == other.stencilRef_;
		}

		bool operator!=(const DrawState& other) const { return !(*this == other); }
	};

	/**
	 * Draw arguments.
	 * - Should currently match D3D12 & Vulkan structure.
	 */
	struct GPU_DLL DrawArgs 
	{
		u32 vertexCountPerInstance_ = 0;
		u32 instanceCount_ = 0;
		u32 startVertexLocation_ = 0;
		u32 startInstanceLocation_ = 0;
	};

	/**
	 * Draw indexed arguments.
	 * - Should currently match D3D12 & Vulkan structure.
	 */
	struct GPU_DLL DrawIndexedArgs 
	{
		u32 indexCountPerInstance_ = 0;
		u32 instanceCount_ = 0;
		u32 startVertexLocation_ = 0;
		i32 baseVertexLocation_ = 0;
		u32 startInstanceLocation_ = 0;
	};
	
	/**
	 * Dispatch arguments.
	 * - Should currently match D3D12 & Vulkan structure.
	 */
	struct GPU_DLL DispatchArgs
	{
		u32 threadGroupCountX_;
		u32 threadGroupCountY_;
		u32 threadGroupCountZ_;
	};

} // namespace GPU
