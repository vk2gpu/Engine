#pragma once

#include "core/types.h"
#include "core/vector.h"
#include "math/aabb.h"
#include "gpu/resources.h"
#include "graphics/shader.h"
#include "common.h"
#include "render_packets.h"

class CompressedModel
{
public:
	CompressedModel(const char* fileName);
	~CompressedModel();

	void DrawClusters(Testbed::DrawContext& drawCtx, Testbed::ObjectConstants object);

	struct Mesh
	{
		Math::AABB bounds_;
		i32 baseVertex_ = 0;
		i32 baseIndex_ = 0;
		i32 numIndices_ = 0;
	};

	Core::Vector<Mesh> meshes_;

	GPU::BufferDesc vertexDesc_;
	GPU::BufferDesc indexDesc_;

	Core::Vector<GPU::VertexElement> elements_;

	GPU::Handle vertexBuffer_;
	GPU::Handle indexBuffer_;

	GPU::Handle positionTex_;
	GPU::Handle normalTex_;
	GPU::Handle colorTex_;

	GPU::Handle dbs_;

	Core::Vector<Graphics::MaterialRef> materials_;
	Core::Vector<Graphics::MaterialRef> compressedMaterials_;

	Graphics::ShaderBindingSet objectBindings_;
	Graphics::ShaderBindingSet geometryBindings_;

	Graphics::ShaderTechniqueDesc techDesc_;
	Graphics::ShaderTechniqueDesc compressedTechDesc_;
	Core::Vector<Testbed::ShaderTechniques> techs_;
	Core::Vector<Testbed::ShaderTechniques> compressedTechs_;

	bool enableCulling_ = true;
};
