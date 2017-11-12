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
	class ForwardPipeline : public Graphics::Pipeline
	{
	public:
		enum class DebugMode
		{
			OFF,
			LIGHT_CULLING,

			MAX,
		};

		ForwardPipeline();
		virtual ~ForwardPipeline();

		void CreateTechniques(
		    Graphics::Material* material, Graphics::ShaderTechniqueDesc desc, ShaderTechniques& outTechniques);
		void SetCamera(
		    const Math::Mat44& view, const Math::Mat44& proj, Math::Vec2 screenDimensions, bool updateFrustum);
		void SetDrawCallback(DrawFn drawFn);
		void Setup(Graphics::RenderGraph& renderGraph) override;
		bool HaveExecuteErrors() const override { return false; }


		DrawFn drawFn_;
		DebugMode debugMode_ = DebugMode::LIGHT_CULLING;

		Graphics::Shader* shader_ = nullptr;

		Core::Vector<Light> lights_;

		Core::Map<Core::String, GPU::FrameBindingSetDesc> fbsDescs_;

		ViewConstants view_;
	};

} // namespace Testbed
