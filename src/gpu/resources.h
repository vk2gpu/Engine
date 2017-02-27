#pragma once

#include "gpu/dll.h"
#include "gpu/types.h"
#include "gpu/format.h"

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
		BUFFER = 0,
		TEXTURE,
		SAMPLER_STATE,
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


} // namespace GPU
