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
		i32 baseCluster_ = 0;
		i32 numClusters_ = 0;
	};

	struct MeshCluster
	{
		i32 meshIdx_ = 0;
		i32 baseDrawArg_ = 0;
		i32 baseVertex_ = 0;
		i32 baseIndex_ = 0;
		i32 numIndices_ = 0;
	};

	Core::Vector<Mesh> meshes_;
	Core::Vector<MeshCluster> clusters_;
	Core::Vector<Math::AABB> clusterBounds_;

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

	Graphics::ShaderBindingSet objectBindings_;

	Graphics::ShaderTechniqueDesc techDesc_;
	Core::Vector<Testbed::ShaderTechniques> techs_;

	bool enableCulling_ = true;
};
