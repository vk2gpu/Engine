#pragma once

#include "core/function.h"
#include "core/debug.h"
#include "graphics/material.h"
#include "graphics/model.h"
#include "graphics/shader.h"
#include "math/vec3.h"
#include "math/mat44.h"

#include "common.h"

using CustomBindFn = Core::Function<bool(Graphics::Shader*, Graphics::ShaderTechnique&)>;

struct DrawContext
{
	DrawContext(GPU::CommandList& cmdList, Graphics::ShaderContext& shaderCtx, const char* passName,
		const GPU::DrawState& drawState, GPU::Handle fbs, GPU::Handle viewCBHandle, GPU::Handle objectSBHandle,
		CustomBindFn customBindFn)
		: cmdList_(cmdList)
		, shaderCtx_(shaderCtx)
		, passName_(passName)
		, drawState_(drawState)
		, fbs_(fbs)
		, viewCBHandle_(viewCBHandle)
		, objectSBHandle_(objectSBHandle)
		, customBindFn_(customBindFn)
	{
	}

	GPU::CommandList& cmdList_;
	Graphics::ShaderContext& shaderCtx_;
	const char* passName_;
	const GPU::DrawState& drawState_;
	GPU::Handle fbs_;
	GPU::Handle viewCBHandle_;
	GPU::Handle objectSBHandle_;
	CustomBindFn customBindFn_;
};

using DrawFn = Core::Function<void(DrawContext& drawCtx), 64>;

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
	ObjectConstants object_;
	Graphics::ShaderTechniqueDesc techDesc_;
	Graphics::Material* material_ = nullptr;
	ShaderTechniques* techs_ = nullptr;

	static void DrawPackets(Core::ArrayView<MeshRenderPacket*> packets, Core::ArrayView<i32> passTechIndices,
		const DrawContext& drawCtx);

	bool IsInstancableWith(const MeshRenderPacket& other) const
	{
		return db_ == other.db_ && memcmp(&draw_, &other.draw_, sizeof(draw_)) == 0 &&
			    memcmp(&techDesc_, &other.techDesc_, sizeof(techDesc_)) == 0 && material_ == other.material_ &&
			    techs_ == other.techs_;
	}
};
