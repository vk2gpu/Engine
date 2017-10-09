#pragma once

#include "core/function.h"
#include "graphics/pipeline.h"
#include "graphics/shader.h"
#include "math/vec3.h"
#include "math/mat44.h"

#include "common.h"
#include "render_packets.h"

namespace Testbed
{
	class ShadowPipeline : public Graphics::Pipeline
	{
	public:
		ShadowPipeline();
		virtual ~ShadowPipeline();

		void CreateTechniques(
		    Graphics::Material* material, Graphics::ShaderTechniqueDesc desc, ShaderTechniques& outTechniques);
		void SetDirectionalLight(Math::Vec3 eyePos, Light light);
		void SetDrawCallback(DrawFn drawFn);
		void Setup(Graphics::RenderGraph& renderGraph) override;
		bool HaveExecuteErrors() const override { return false; }


		DrawFn drawFn_;

		Graphics::Shader* shader_ = nullptr;

		Math::Vec3 eyePos_;
		Light directionalLight_;

		Core::Map<Core::String, GPU::FrameBindingSetDesc> fbsDescs_;

		ViewConstants view_;
	};

} // namespace Testbed
