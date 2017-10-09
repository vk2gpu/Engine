#pragma once

#include "core/array.h"
#include "core/array_view.h"
#include "core/vector.h"
#include "gpu/resources.h"
#include "graphics/shader.h"
#include "graphics/texture.h"

namespace Graphics
{
	struct MaterialTexture
	{
		Core::Array<char, 32> bindingName_;
		Core::UUID resourceName_;
	};

	struct MaterialData
	{
		Core::UUID shader_;
		i32 numTextures_ = 0;
	};

	struct MaterialImpl
	{
		MaterialData data_;

		Core::Vector<MaterialTexture> textures_;

		ShaderRef shaderRes_;
		Core::Vector<TextureRef> textureRes_;

		MaterialImpl();
		~MaterialImpl();
	};

} // namespace Graphics
