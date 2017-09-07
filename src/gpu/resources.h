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
	};

	/**
	 * TextureDesc.
	 */
	struct GPU_DLL TextureDesc
	{
		TextureType type_ = TextureType::INVALID;
		BindFlags bindFlags_ = BindFlags::NONE;
		Format format_ = Format::INVALID;
		i32 width_ = 1;
		i32 height_ = 1;
		i16 depth_ = 1;
		i16 levels_ = 1;
		i16 elements_ = 1;
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
	 * Sampler state.
	 */
	struct GPU_DLL SamplerState
	{
		AddressingMode addressU_ = AddressingMode::WRAP;
		AddressingMode addressV_ = AddressingMode::WRAP;
		AddressingMode addressW_ = AddressingMode::WRAP;
		FilteringMode minFilter_ = FilteringMode::NEAREST;
		FilteringMode magFilter_ = FilteringMode::NEAREST;
		f32 mipLODBias_ = 0.0f;
		u32 maxAnisotropy_ = 1;
		float borderColor_[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		f32 minLOD_ = -Core::F32_MAX;
		f32 maxLOD_ = Core::F32_MAX;
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
	 * Blend state.
	 * One for each RT.
	 */
	struct GPU_DLL BlendState
	{
		u32 enable_ = 0;
		BlendType srcBlend_ = BlendType::ONE;
		BlendType destBlend_ = BlendType::ONE;
		BlendFunc blendOp_ = BlendFunc::ADD;
		BlendType srcBlendAlpha_ = BlendType::ONE;
		BlendType destBlendAlpha_ = BlendType::ONE;
		BlendFunc blendOpAlpha_ = BlendFunc::ADD;
		u8 writeMask_ = 0xf;
	};

	/**
	 * Stencil face state.
	 * One front, one back.
	 */
	struct GPU_DLL StencilFaceState
	{
		StencilFunc fail_ = StencilFunc::KEEP;
		StencilFunc depthFail_ = StencilFunc::KEEP;
		StencilFunc pass_ = StencilFunc::KEEP;
		CompareMode func_ = CompareMode::ALWAYS;
	};

	/**
	 * Render state.
	 */
	struct GPU_DLL RenderState
	{
		// Blend state.
		BlendState blendStates_[MAX_BOUND_RTVS];

		// Depth stencil.
		StencilFaceState stencilFront_;
		StencilFaceState stencilBack_;
		u32 depthEnable_ = 0;
		u32 depthWriteMask_ = 0;
		CompareMode depthFunc_ = CompareMode::GREATER_EQUAL;
		u32 stencilEnable_ = 0;
		u32 stencilRef_ = 0;
		u8 stencilRead_ = 0;
		u8 stencilWrite_ = 0;

		// Rasterizer.
		FillMode fillMode_ = FillMode::SOLID;
		CullMode cullMode_ = CullMode::NONE;
		f32 depthBias_ = 0.0f;
		f32 slopeScaledDepthBias_ = 0.0f;
		u32 antialiasedLineEnable_ = 0;
	};

	/**
	 * GraphicsPipelineStateDesc
	 */
	struct GPU_DLL GraphicsPipelineStateDesc
	{
		Handle shaders_[5];
		RenderState renderState_;
		i32 numVertexElements_ = 0;
		VertexElement vertexElements_[MAX_VERTEX_ELEMENTS];
		TopologyType topology_ = TopologyType::INVALID;
		i32 numRTs_ = 0;
		Format rtvFormats_[MAX_BOUND_RTVS] = {Format::INVALID, Format::INVALID, Format::INVALID, Format::INVALID,
		    Format::INVALID, Format::INVALID, Format::INVALID, Format::INVALID};
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
	 * Base binding information for views.
	 */
	struct GPU_DLL BindingView
	{
		Handle resource_;
		Format format_ = Format::INVALID;
		ViewDimension dimension_ = ViewDimension::INVALID;
	};


	/**
	 * Binding for a render target view.
	 */
	struct GPU_DLL BindingRTV : BindingView
	{
		i32 mipSlice_ = 0;
		i32 firstArraySlice_ = 0;
		i32 planeSlice_FirstWSlice_ = 0;
		i32 arraySize_ = 0;
		i32 wSize_ = 0;
	};

	/**
	 * Binding for a depth stencil view.
	 */
	struct GPU_DLL BindingDSV : BindingView
	{
		DSVFlags flags_ = DSVFlags::NONE;
		i32 mipSlice_ = 0;
		i32 firstArraySlice_ = 0;
		i32 arraySize_ = 0;
	};

	static_assert(sizeof(BindingDSV) <= 32, "BindingDSV should remain under 32 bytes.");

	/**
	 * Binding for a shader resource view.
	 */
	struct GPU_DLL BindingSRV : BindingView
	{
		i32 mostDetailedMip_FirstElement_ = 0;
		i32 mipLevels_NumElements_ = 0;
		i32 firstArraySlice_ = 0;
		i32 planeSlice_ = 0;
		i32 arraySize_ = 0;
		i32 structureByteStride_ = 0;
		f32 resourceMinLODClamp_ = 0.0f;
	};

	static_assert(sizeof(BindingSRV) <= 64, "BindingSRV should remain under 64 bytes.");

	/**
	 * Binding for an unordered access view.
	 */
	struct GPU_DLL BindingUAV : BindingView
	{
		i32 mipSlice_FirstElement_ = 0;
		i32 firstArraySlice_FirstWSlice_NumElements_ = 0;
		i32 arraySize_PlaneSlice_WSize_ = 0;
		i32 structureByteStride_ = 0;
	};

	static_assert(sizeof(BindingUAV) <= 32, "BindingUAV should remain under 32 bytes.");

	/**
	 * Binding for a buffer.
	 */
	struct GPU_DLL BindingBuffer
	{
		Handle resource_;
		i32 offset_ = 0;
		i32 size_ = 0;
		i32 stride_ = 0;
	};

	/**
	 * Binding for a CBV.
	 */
	using BindingCBV = BindingBuffer;

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
		Handle pipelineState_;
		i32 numSRVs_ = 0;
		i32 numUAVs_ = 0;
		i32 numCBVs_ = 0;
		i32 numSamplers_ = 0;
		BindingSRV srvs_[MAX_SRV_BINDINGS];
		BindingUAV uavs_[MAX_UAV_BINDINGS];
		BindingCBV cbvs_[MAX_CBV_BINDINGS];
		BindingSampler samplers_[MAX_SAMPLER_BINDINGS];
	};

	/**
	 * Draw binding set.
	 */
	struct GPU_DLL DrawBindingSetDesc
	{
		BindingBuffer vbs_[MAX_VERTEX_STREAMS];
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

	/**
	 * Utility functions for creating binding types.
	 */
	namespace Binding
	{
		GPU_DLL_INLINE BindingCBV CBuffer(GPU::Handle res, i32 offset, i32 size);

		GPU_DLL_INLINE BindingSRV Buffer(GPU::Handle res, GPU::Format format = GPU::Format::INVALID,
		    i32 firstElement = 0, i32 numElements = 0, i32 structureByteStride = 0);
		GPU_DLL_INLINE BindingSRV Texture1D(GPU::Handle res, GPU::Format format = GPU::Format::INVALID,
		    i32 mostDetailedMip = 0, i32 mipLevels = 0, f32 resourceMinLODClamp = 0.0f);
		GPU_DLL_INLINE BindingSRV Texture1DArray(GPU::Handle res, GPU::Format format = GPU::Format::INVALID,
		    i32 mostDetailedMip = 0, i32 mipLevels = 0, i32 firstArraySlice = 0, i32 arraySize = 0,
		    f32 resourceMinLODClamp = 0.0f);
		GPU_DLL_INLINE BindingSRV Texture2D(GPU::Handle res, GPU::Format format = GPU::Format::INVALID,
		    i32 mostDetailedMip = 0, i32 mipLevels = 0, i32 planeSlice = 0, f32 resourceMinLODClamp = 0.0f);
		GPU_DLL_INLINE BindingSRV Texture2DArray(GPU::Handle res, GPU::Format format = GPU::Format::INVALID,
		    i32 mostDetailedMip = 0, i32 mipLevels = 0, i32 firstArraySlice = 0, i32 arraySize = 0, i32 planeSlice = 0,
		    f32 resourceMinLODClamp = 0.0f);
		GPU_DLL_INLINE BindingSRV Texture3D(GPU::Handle res, GPU::Format format = GPU::Format::INVALID,
		    i32 mostDetailedMip = 0, i32 mipLevels = 0, f32 resourceMinLODClamp = 0.0f);
		GPU_DLL_INLINE BindingSRV TextureCube(GPU::Handle res, GPU::Format format = GPU::Format::INVALID,
		    i32 mostDetailedMip = 0, i32 mipLevels = 0, f32 resourceMinLODClamp = 0.0f);
		GPU_DLL_INLINE BindingSRV TextureCubeArray(GPU::Handle res, GPU::Format format = GPU::Format::INVALID,
		    i32 mostDetailedMip = 0, i32 mipLevels = 0, i32 first2DArrayFace = 0, i32 numCubes = 0,
		    f32 resourceMinLODClamp = 0.0f);

		GPU_DLL_INLINE BindingUAV RWBuffer(GPU::Handle res, GPU::Format format = GPU::Format::INVALID,
		    i32 firstElement = 0, i32 numElements = 0, i32 structureByteStride = 0);
		GPU_DLL_INLINE BindingUAV RWTexture1D(
		    GPU::Handle res, GPU::Format format = GPU::Format::INVALID, i32 mipSlice = 0);
		GPU_DLL_INLINE BindingUAV RWTexture1DArray(GPU::Handle res, GPU::Format format = GPU::Format::INVALID,
		    i32 mipSlice = 0, i32 firstArraySlice = 0, i32 arraySize = 0);
		GPU_DLL_INLINE BindingUAV RWTexture2D(
		    GPU::Handle res, GPU::Format format = GPU::Format::INVALID, i32 mipSlice = 0, i32 planeSlice = 0);
		GPU_DLL_INLINE BindingUAV RWTexture2DArray(GPU::Handle res, GPU::Format format = GPU::Format::INVALID,
		    i32 mipSlice = 0, i32 planeSlice = 0, i32 firstArraySlice = 0, i32 arraySize = 0);
		GPU_DLL_INLINE BindingUAV RWTexture3D(GPU::Handle res, GPU::Format format = GPU::Format::INVALID,
		    i32 mipSlice = 0, i32 firstWSlice = 0, i32 wSize = 0);
	} // namespace Binding

} // namespace GPU

#if CODE_INLINE
#include "gpu/private/resources.inl"
#endif
