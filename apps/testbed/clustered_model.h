#pragma once

#include "core/types.h"
#include "core/vector.h"
#include "math/aabb.h"
#include "gpu/resources.h"
#include "graphics/shader.h"
#include "render_packets.h"

/**
 * Prototype of clustered model.
 * This will split a model up into a bunch of clusters trying multiple methods:
 * 1) Split up into fixed sized clusters of triangles by locality.
 * 2) tbd.
 *
 * All of these will build up a buffer of indirect draw parameters on GPU, culling more
 * finely than would be efficient to do on CPU.
 *
 * This will load directly from a model file, and flatten the entire hierarchy.
 */
class ClusteredModel
{
public:
	ClusteredModel(const char* fileName);
	~ClusteredModel();

	void DrawClusters(GPU::CommandList& cmdList, const char* passName, const GPU::DrawState& drawState, GPU::Handle fbs,
	    GPU::Handle viewCBHandle, GPU::Handle objectSBHandle, Testbed::CustomBindFn customBindFn,
	    Testbed::ObjectConstants object);

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
	GPU::BufferDesc boundsDesc_;
	GPU::BufferDesc clusterDesc_;
	GPU::BufferDesc drawArgsDesc_;
	GPU::BufferDesc drawCountDesc_;
	GPU::BufferDesc culledIndexDesc_;
	GPU::BufferDesc culledDrawArgDesc_;

	Core::Vector<GPU::VertexElement> elements_;

	GPU::Handle vertexBuffer_;
	GPU::Handle indexBuffer_;
	GPU::Handle boundsBuffer_;
	GPU::Handle clusterBuffer_;
	GPU::Handle drawArgsBuffer_;
	GPU::Handle drawCountBuffer_;

	GPU::Handle dbs_;

	Graphics::ShaderRef coreShader_;
	Core::Vector<Graphics::MaterialRef> materials_;

	Graphics::ShaderTechnique cullClusterTech_;


	Graphics::ShaderTechniqueDesc techDesc_;
	Core::Vector<Testbed::ShaderTechniques> techs_;

	bool enableCulling_ = true;
};
