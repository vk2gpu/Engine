#pragma once

#include "dll.h"
#include "core/map.h"
#include "core/string.h"
#include "graphics/shader.h"
#include "math/mat44.h"
#include "math/plane.h"
#include "math/vec2.h"

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
	Math::Plane frustumPlanes_[6];
	Math::Vec2 screenDimensions_;

	void CalculateFrustum()
	{
		frustumPlanes_[0] = Math::Plane(viewProj_[0][3] + viewProj_[0][0], viewProj_[1][3] + viewProj_[1][0],
		    viewProj_[2][3] + viewProj_[2][0], viewProj_[3][3] + viewProj_[3][0]);
		frustumPlanes_[1] = Math::Plane(viewProj_[0][3] - viewProj_[0][0], viewProj_[1][3] - viewProj_[1][0],
		    viewProj_[2][3] - viewProj_[2][0], viewProj_[3][3] - viewProj_[3][0]);
		frustumPlanes_[2] = Math::Plane(viewProj_[0][3] + viewProj_[0][1], viewProj_[1][3] + viewProj_[1][1],
		    viewProj_[2][3] + viewProj_[2][1], viewProj_[3][3] + viewProj_[3][1]);
		frustumPlanes_[3] = Math::Plane(viewProj_[0][3] - viewProj_[0][1], viewProj_[1][3] - viewProj_[1][1],
		    viewProj_[2][3] - viewProj_[2][1], viewProj_[3][3] - viewProj_[3][1]);
		frustumPlanes_[4] = Math::Plane(viewProj_[0][3] - viewProj_[0][2], viewProj_[1][3] - viewProj_[1][2],
		    viewProj_[2][3] - viewProj_[2][2], viewProj_[3][3] - viewProj_[3][2]);
		frustumPlanes_[5] = Math::Plane(viewProj_[0][3], viewProj_[1][3], viewProj_[2][3], viewProj_[3][3]);

		for(i32 i = 0; i < 6; ++i)
		{
			Math::Vec3 normal = frustumPlanes_[i].Normal();
			f32 scale = 1.0f / normal.Magnitude();
			frustumPlanes_[i] =
			    Math::Plane(frustumPlanes_[i].Normal().x * -scale, frustumPlanes_[i].Normal().y * -scale,
			        frustumPlanes_[i].Normal().z * -scale, frustumPlanes_[i].D() * scale);
		}
	}
};

struct VTParams
{
	Math::Vec2 tileSize_ = Math::Vec2(128, 128);
	Math::Vec2 vtSize_ = Math::Vec2(256 * 1024, 256 * 1024);
	Math::Vec2 cacheSize_ = Math::Vec2(12, 12) * Math::Vec2(128, 128);
	i32 feedbackDivisor_ = 4;
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
