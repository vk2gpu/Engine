#pragma once

#include "gpu/dll.h"
#include "gpu/types.h"
#include "gpu/format.h"
#include "gpu/render_state.h"

namespace GPU
{
	/**
	 * Implement our own handle type. This will help to
	 * enforce type safety, and prevent other handles
	 * being passed into this library.
	 */
	class Handle : public Core::Handle
	{
	public:
		Handle() = default;
		Handle(const Handle&) = default;
		Handle(const Core::Handle&) = delete;
		operator Core::Handle() const { return *this; }
	};

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

		MAX
	};

	/**
	 * BufferDesc.
	 * Structure is used when creating a buffer resource.
	 * Easier to extend than function calls.
	 */
	struct BufferDesc
	{
		BindFlags bindFlags_ = BindFlags::NONE;
		i64 size_ = 0;
		i64 stride_ = 0;
	};

	/**
	 * TextureDesc.
	 */
	struct TextureDesc
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
	struct TextureSubResourceData
	{
		void* data_ = nullptr;
		i32 rowPitch_ = 0;
		i32 slicePitch_ = 0;
	};

	/**
	 * SwapChainDesc.
	 */
	struct SwapChainDesc
	{
		Format format_ = Format::INVALID;
		i32 bufferCount_ = 0;
		void* outputWindow_ = 0;
	};

	/**
	 * ShaderDesc.
	 */
	struct ShaderDesc
	{
		ShaderType type_ = ShaderType::INVALID;
		const void* data_ = nullptr;
		i32 dataSize_ = 0;
	};

	/**
	 * GraphicsPipelineStateDesc
	 */
	struct GraphicsPipelineStateDesc
	{
		Handle shaders_[5];
		RenderState renderState_;
		VertexElement vertexElements_[MAX_VERTEX_ELEMENTS];
		PrimitiveTopology topology_ = PrimitiveTopology::INVALID;
		i32 numRTs_ = 0;
		Format rtvFormats_[MAX_BOUND_RTVS] = { Format::INVALID };
		Format dsvFormat_ = Format::INVALID;
	};

	/**
	 * ComputePipelineStateDesc
	 */
	struct ComputePipelineStateDesc
	{
		Handle shader_;
	};
} // namespace GPU
