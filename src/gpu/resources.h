#pragma once

#include "gpu/dll.h"
#include "gpu/types.h"

#include "core/float.h"

namespace GPU
{
	/**
	 * All the resource types we represent.
	 */
	enum class ResourceType : i32
	{
		INVALID = -1,
		SWAP_CHAIN = 0,
		BUFFER,
		TEXTURE,
		SAMPLER_STATE,
		SHADER,
		GRAPHICS_PIPELINE_STATE,
		COMPUTE_PIPELINE_STATE,

		PIPELINE_BINDING_SET,
		DRAW_BINDING_SET,
		FRAME_BINDING_SET,

		COMMAND_LIST,
		FENCE,

		MAX
	};

	/**
	 * Implement our own handle type. This will help to
	 * enforce type safety, and prevent other handles
	 * being passed into this library.
	 */
	class GPU_DLL Handle : public Core::Handle
	{
	public:
		Handle() = default;
		Handle(const Handle&) = default;
		explicit Handle(const Core::Handle& handle)
		    : Core::Handle(handle)
		{
		}
		operator Core::Handle() const { return *this; }
		ResourceType GetType() const { return (ResourceType)Core::Handle::GetType(); }
	};

	/**
	 * SwapChainDesc.
	 */
	struct GPU_DLL SwapChainDesc
	{
		i32 width_ = 0;
		i32 height_ = 0;
		Format format_ = Format::INVALID;
		i32 bufferCount_ = 0;
		void* outputWindow_ = 0;
	};

	/**
	 * BufferDesc.
	 * Structure is used when creating a buffer resource.
	 * Easier to extend than function calls.
	 */
	struct GPU_DLL BufferDesc
	{
		BindFlags bindFlags_ = BindFlags::NONE;
		i64 size_ = 0;
		i64 stride_ = 0;
	};

	/**
	 * TextureDesc.
	 */
	struct GPU_DLL TextureDesc
	{
		TextureType type_ = TextureType::INVALID;
		BindFlags bindFlags_ = BindFlags::NONE;
		enum class Format format_ = Format::INVALID;
		i32 width_ = 0;
		i32 height_ = 0;
		i32 depth_ = 0;
		i32 levels_ = 0;
		i32 elements_ = 0;
	};

	/**
	 * Texture data.
	 * Defines a single subresource of a texture.
	 */
	struct GPU_DLL TextureSubResourceData
	{
		const void* data_ = nullptr;
		i32 rowPitch_ = 0;
		i32 slicePitch_ = 0;
	};

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
	struct GPU_DLL SamplerState
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


	/**
	 * ShaderDesc.
	 */
	struct GPU_DLL ShaderDesc
	{
		ShaderType type_ = ShaderType::INVALID;
		const void* data_ = nullptr;
		i32 dataSize_ = 0;
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
	struct GPU_DLL BlendState
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
	struct GPU_DLL StencilFaceState
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
	struct GPU_DLL RenderState
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

	/**
	 * GraphicsPipelineStateDesc
	 */
	struct GPU_DLL GraphicsPipelineStateDesc
	{
		Handle shaders_[5];
		RenderState renderState_;
		VertexElement vertexElements_[MAX_VERTEX_ELEMENTS];
		TopologyType topology_ = TopologyType::INVALID;
		i32 numRTs_ = 0;
		Format rtvFormats_[MAX_BOUND_RTVS] = {Format::INVALID};
		Format dsvFormat_ = Format::INVALID;
	};

	/**
	 * ComputePipelineStateDesc
	 */
	struct GPU_DLL ComputePipelineStateDesc
	{
		Handle shader_;
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
	 * Binding for a render target view.
	 */
	struct GPU_DLL BindingRTV
	{
		Handle resource_;
		Format format_ = Format::INVALID;
		ViewDimension dimension_ = ViewDimension::INVALID;
		i64 mipSlice_ = 0;
		i64 planeSlice_FirstWSlice = 0;
		i64 arraySize_ = 0;
		i64 wSize_ = 0;
	};

	/**
	 * Binding for a depth stencil view.
	 */
	struct GPU_DLL BindingDSV
	{
		Handle resource_;
		Format format_ = Format::INVALID;
		ViewDimension dimension_ = ViewDimension::INVALID;
		DSVFlags flags_ = DSVFlags::NONE;
		i64 mipSlice_ = 0;
	};

	/**
	 * Binding for a shader resource view.
	 */
	struct GPU_DLL BindingSRV
	{
		Handle resource_;
		Format format_ = Format::INVALID;
		ViewDimension dimension_ = ViewDimension::INVALID;
		i64 mostDetailedMip_FirstElement_ = 0;
		i64 mipLevels_NumElements_ = 0;
		i64 arraySize_ = 0;
	};

	/**
	 * Binding for an unordered access view.
	 */
	struct GPU_DLL BindingUAV
	{
		Handle resource_;
		Format format_ = Format::INVALID;
		ViewDimension dimension_ = ViewDimension::INVALID;
		i64 mipSlice_FirstElement_ = 0;
		i64 firstArraySlice_NumElements_ = 0;
		i64 arraySize_ = 0;
	};

	/**
	 * Binding for a buffer.
	 */
	struct GPU_DLL BindingBuffer
	{
		Handle resource_;
		i32 offset_ = 0;
		i32 size_ = 0;
	};

	/**
	 * Binding for a sampler.
	 */
	struct GPU_DLL BindingSampler
	{
		Handle resource_;
	};

	/**
	 * Pipeline binding set.
	 * Common parameters shared by both graphics and compute
	 * pipeline states.
	 */
	struct GPU_DLL PipelineBindingSetDesc
	{
		i32 numSRVs_ = 0;
		i32 numUAVs_ = 0;
		i32 numCBs_ = 0;
		i32 numSamplers_ = 0;
		BindingSRV srvs_[MAX_SRV_BINDINGS];
		BindingUAV uavs_[MAX_UAV_BINDINGS];
		BindingBuffer cbvs_[MAX_CBV_BINDINGS];
		BindingSampler samplers_[MAX_SAMPLER_BINDINGS];
	};

	/**
	 * Draw binding set.
	 */
	struct GPU_DLL DrawBindingSetDesc
	{
		i32 numVertexBuffers_ = 0;
		BindingBuffer vbs_[16];
		BindingBuffer ib_;
	};

	/**
	 * Draw frame binding set.
	 */
	struct GPU_DLL FrameBindingSetDesc
	{
		BindingRTV rtvs_[MAX_BOUND_RTVS];
		BindingDSV dsv_;
	};

} // namespace GPU
