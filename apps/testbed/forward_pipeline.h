#pragma once

#include "core/function.h"
#include "graphics/pipeline.h"
#include "graphics/shader.h"
#include "math/vec3.h"
#include "math/mat44.h"

#include "common.h"

namespace Testbed
{
	using CustomBindFn = Core::Function<bool(Graphics::Shader*, Graphics::ShaderTechnique&)>;
	using DrawFn = Core::Function<void(GPU::CommandList& cmdList, const char* passName, const GPU::DrawState& drawState,
	    GPU::Handle fbs, GPU::Handle viewCBHandle, GPU::Handle objectSBHandle, CustomBindFn customBindFn)>;

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
		    Graphics::Shader* shader, Graphics::ShaderTechniqueDesc desc, ShaderTechniques& outTechniques);
		void SetCamera(const Math::Mat44& view, const Math::Mat44& proj, Math::Vec2 screenDimensions);
		void SetDrawCallback(DrawFn drawFn);
		void Setup(Graphics::RenderGraph& renderGraph) override;
		bool HaveExecuteErrors() const override { return false; }


		DrawFn drawFn_;
		DebugMode debugMode_ = DebugMode::LIGHT_CULLING;

		Graphics::Shader* shader_ = nullptr;
		Graphics::ShaderTechnique techComputeTileInfo_;
		Graphics::ShaderTechnique techComputeLightLists_;
		Graphics::ShaderTechnique techDebugTileInfo_;

		Core::Vector<Light> lights_;

		Core::Map<Core::String, GPU::FrameBindingSetDesc> fbsDescs_;

		ViewConstants view_;
	};

} // namespace Testbed
