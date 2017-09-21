#pragma once

#include "core/function.h"
#include "core/debug.h"
#include "graphics/shader.h"
#include "graphics/model.h"
#include "math/vec3.h"
#include "math/mat44.h"

#include "common.h"

namespace Testbed
{
	using CustomBindFn = Core::Function<bool(Graphics::Shader*, Graphics::ShaderTechnique&)>;
	using DrawFn = Core::Function<void(GPU::CommandList& cmdList, const char* passName, const GPU::DrawState& drawState,
	                                  GPU::Handle fbs, GPU::Handle viewCBHandle, GPU::Handle objectSBHandle,
	                                  CustomBindFn customBindFn),
	    64>;

	enum class RenderPacketType : i16
	{
		UNKNOWN = 0,
		MESH,

		MAX,
	};

	struct RenderPacketBase
	{
		RenderPacketType type_ = RenderPacketType::UNKNOWN;
		i16 size_ = 0;
	};

	template<typename TYPE>
	struct RenderPacket : RenderPacketBase
	{
		RenderPacket()
		{
			DBG_ASSERT(TYPE::TYPE != RenderPacketType::UNKNOWN);
			DBG_ASSERT(sizeof(TYPE) <= SHRT_MAX);
			type_ = TYPE::TYPE;
			size_ = sizeof(TYPE);
		}
	};

	void SortPackets(Core::Vector<RenderPacketBase*>& packets);

	struct MeshRenderPacket : RenderPacket<MeshRenderPacket>
	{
		static const RenderPacketType TYPE = RenderPacketType::MESH;

		GPU::Handle db_;
		Graphics::ModelMeshDraw draw_;
		Testbed::ObjectConstants object_;
		Graphics::ShaderTechniqueDesc techDesc_;
		Graphics::Shader* shader_ = nullptr;
		Testbed::ShaderTechniques* techs_ = nullptr;

		// tmp object state data.
		Math::Mat44 world_;
		f32 angle_ = 0.0f;
		Math::Vec3 position_;

		static void DrawPackets(Core::ArrayView<MeshRenderPacket*> packets, Core::ArrayView<i32> passTechIndices,
		    GPU::CommandList& cmdList, const GPU::DrawState& drawState, GPU::Handle fbs, GPU::Handle viewCBHandle,
		    GPU::Handle objectSBHandle, CustomBindFn customBindFn);

		bool IsInstancableWith(const MeshRenderPacket& other) const
		{
			return db_ == other.db_ && memcmp(&draw_, &other.draw_, sizeof(draw_)) == 0 &&
			       memcmp(&techDesc_, &other.techDesc_, sizeof(techDesc_)) == 0 && shader_ == other.shader_ &&
			       techs_ == other.techs_;
		}
	};

} // namespace Testbed
