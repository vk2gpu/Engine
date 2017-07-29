#pragma once
#include "core/array_view.h"
#include "core/vector.h"
#include "math/aabb.h"
#include "math/mat44.h"
#include "gpu/resources.h"

namespace Graphics
{
	struct NodeDataAoS
	{
		i32 parent_ = 0;
		Math::Mat44 local_;
		Math::Mat44 world_;
	};

	struct NodeDataSoA
	{
		Core::Vector<Math::Mat44> local_;
		Core::Vector<Math::Mat44> world_;
		Core::Vector<i32> parents_;
	};

	struct ModelData
	{
		i32 numNodes_ = 0;
		i32 numMeshNodes_ = 0;
		i32 numMeshes_ = 0;
		u32 numAABBs_ = 0;
		i32 numBonePalettes_ = 0;
		i32 numInverseBindPoses_ = 0;
		i32 numMaterials_ = 0;
	};

	struct MeshNode
	{
		i32 nodeIdx_ = -1;
		i32 materialIdx_ = -1;
		i32 aabbIdx_ = -1;
		i32 noofBones_ = -1;
		i32 bonePaletteIdx_ = -1;
		i32 inverseBindPoseIdx_ = -1;
		i32 meshIdx_ = -1;
		i32 drawIdx_ = -1;
	};

	struct MeshNodeAABB
	{
		Math::AABB aabb_;
	};

	struct MeshNodeBonePalette
	{
		static MeshNodeBonePalette* Create(i32 numBones)
		{
			auto* retVal = (MeshNodeBonePalette*)(new Math::Mat44[numBones]);
			for(i32 idx = 0; idx < numBones; ++idx)
				new(&retVal->indices_[idx]) i32(-1);
			return retVal;
		}

		i32 indices_[1];
		// Overallocated.
	};

	struct MeshNodeInverseBindpose
	{
		static MeshNodeInverseBindpose* Create(i32 numBones)
		{
			auto* retVal = (MeshNodeInverseBindpose*)(new Math::Mat44[numBones]);
			for(i32 idx = 0; idx < numBones; ++idx)
				new(&retVal->transforms_[idx]) Math::Mat44();
			return retVal;
		}

		Math::Mat44 transforms_[1];
		// Overallocated.
	};

	struct ModelMeshData
	{
		GPU::PrimitiveTopology primTopology_ = GPU::PrimitiveTopology::TRIANGLE_LIST;
		i32 noofVertices_ = 0;
		i32 vertexStride_ = 0;
		i32 noofIndices_ = 0;
		i32 indexStride_ = 0;
		i32 startVertexElements_ = 0;
		i32 endVertexElements_ = 0;
		i32 startDraws_ = 0;
		i32 endDraws_ = 0;
	};

	struct ModelImpl
	{
		ModelData data_;
		NodeDataSoA nodeDatas_;

		// Mesh node data.
		Core::Vector<MeshNode> meshNodes_;
		Core::Vector<MeshNodeAABB> meshNodeAABBDatas_;
		Core::Vector<MeshNodeBonePalette> meshNodeBonePaletteDatas_;
		Core::Vector<MeshNodeInverseBindpose> meshNodeInverseBindposeDatas_;

		// Actual mesh data.
		Core::Vector<ModelMeshData> modelMeshes_;
		Core::Vector<GPU::VertexElement> elements_;
		Core::Vector<ModelMeshDraw> draws_;

		Core::Vector<GPU::Handle> vbs_;
		Core::Vector<GPU::Handle> ibs_;
		Core::Vector<GPU::Handle> dbs_;

		ModelImpl();
		~ModelImpl();
	};

} // namespace Graphics
