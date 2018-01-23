#pragma once

#include "graphics/dll.h"
#include "graphics/fwd_decls.h"
#include "core/array.h"
#include "gpu/fwd_decls.h"
#include "gpu/types.h"
#include "resource/ref.h"
#include "resource/resource.h"
#include "math/mat44.h"


namespace Graphics
{
	struct ModelMeshDraw
	{
		i32 vertexOffset_ = 0;
		i32 indexOffset_ = 0;
		i32 noofVertices_ = 0;
		i32 noofIndices_ = 0;
	};

	using ModelRef = Resource::Ref<class Model>;

	class GRAPHICS_DLL Model
	{
	public:
		DECLARE_RESOURCE(Model, "Graphics.Model", 1);

		/// @return Number of meshes.
		i32 GetNumMeshes() const;

		/// @return Vertex elements for @a meshIdx.
		Core::ArrayView<GPU::VertexElement> GetMeshVertexElements(i32 meshIdx) const;

		/// @return Draw binding for @a meshIdx.
		GPU::Handle GetMeshDrawBinding(i32 meshIdx) const;

		/// @return Draw info for @a meshIdx.
		ModelMeshDraw GetMeshDraw(i32 meshIdx) const;

		/// @return Material for @a meshIdx.
		Material* Model::GetMeshMaterial(i32 meshIdx) const;

		/// @return Mesh world transform.
		Math::Mat44 GetMeshWorldTransform(i32 meshIdx) const;

		/// @return Is model ready for use?
		bool IsReady() const { return !!impl_; }

	private:
		friend class ModelFactory;

		Model();
		~Model();
		Model(const Model&) = delete;
		Model& operator=(const Model&) = delete;

		struct ModelImpl* impl_ = nullptr;
	};

} // namespace Graphics
