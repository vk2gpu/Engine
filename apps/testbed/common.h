#pragma once

#include "core/map.h"
#include "core/string.h"
#include "graphics/shader.h"
#include "math/vec2.h"
#include "math/mat44.h"

namespace Testbed
{
	struct ObjectConstants
	{
		Math::Mat44 world_;
	};

	struct ViewConstants
	{
		Math::Mat44 view_;
		Math::Mat44 proj_;
		Math::Mat44 viewProj_;
		Math::Mat44 invView_;
		Math::Mat44 invProj_;
		Math::Vec2 screenDimensions_;
	};

	struct Light
	{
		Math::Vec3 position_ = Math::Vec3(0.0f, 0.0f, 0.0f);
		Math::Vec3 color_ = Math::Vec3(0.0f, 0.0f, 0.0f);
		float radiusInner_ = 0.0f;
		float radiusOuter_ = 0.0f;
	};

	struct ShaderTechniques
	{
		Graphics::Material* material_ = nullptr;
		Core::Map<Core::String, i32> passIndices_;
		Core::Vector<Graphics::ShaderTechnique> passTechniques_;
	};


} // namespace Testbed
