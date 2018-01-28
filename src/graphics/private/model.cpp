#include "graphics/model.h"
#include "graphics/private/model_impl.h"

#include "resource/factory.h"
#include "resource/manager.h"

#include "core/debug.h"
#include "core/file.h"
#include "core/hash.h"
#include "core/misc.h"
#include "gpu/manager.h"
#include "gpu/utils.h"

#include <algorithm>

namespace Graphics
{
	class ModelFactory : public Resource::IFactory
	{
	public:
		bool CreateResource(Resource::IFactoryContext& context, void** outResource, const Core::UUID& type) override
		{
			DBG_ASSERT(type == Model::GetTypeUUID());
			*outResource = new Model();
			return true;
		}

		bool DestroyResource(Resource::IFactoryContext& context, void** inResource, const Core::UUID& type) override
		{
			DBG_ASSERT(type == Model::GetTypeUUID());
			auto* shader = reinterpret_cast<Model*>(*inResource);
			delete shader;
			*inResource = nullptr;
			return true;
		}

		bool LoadResource(Resource::IFactoryContext& context, void** inResource, const Core::UUID& type,
		    const char* name, Core::File& inFile) override
		{
			Model* model = *reinterpret_cast<Model**>(inResource);
			i64 readBytes = 0;

			DBG_ASSERT(model);

			auto* impl = new ModelImpl();

			readBytes = sizeof(ModelData);
			if(inFile.Read(&impl->data_, readBytes) != readBytes)
			{
				delete impl;
				return false;
			}

			impl->nodeDatas_.local_.resize(impl->data_.numNodes_);
			impl->nodeDatas_.world_.resize(impl->data_.numNodes_);
			impl->nodeDatas_.parents_.resize(impl->data_.numNodes_);
			impl->meshNodes_.resize(impl->data_.numMeshNodes_);
			impl->meshNodeAABBDatas_.resize(impl->data_.numAABBs_);
			impl->meshNodeBonePaletteDatas_.resize(impl->data_.numBonePalettes_);
			impl->meshNodeInverseBindposeDatas_.resize(impl->data_.numInverseBindPoses_);

			readBytes = sizeof(Math::Mat44) * impl->data_.numNodes_;
			if(inFile.Read(impl->nodeDatas_.local_.data(), readBytes) != readBytes)
			{
				delete impl;
				return false;
			}

			readBytes = sizeof(Math::Mat44) * impl->data_.numNodes_;
			if(inFile.Read(impl->nodeDatas_.world_.data(), readBytes) != readBytes)
			{
				delete impl;
				return false;
			}

			readBytes = sizeof(i32) * impl->data_.numNodes_;
			if(inFile.Read(impl->nodeDatas_.parents_.data(), readBytes) != readBytes)
			{
				delete impl;
				return false;
			}

			readBytes = sizeof(MeshNode) * impl->data_.numMeshNodes_;
			if(inFile.Read(impl->meshNodes_.data(), readBytes) != readBytes)
			{
				delete impl;
				return false;
			}

			readBytes = sizeof(MeshNodeAABB) * impl->data_.numAABBs_;
			if(inFile.Read(impl->meshNodeAABBDatas_.data(), readBytes) != readBytes)
			{
				delete impl;
				return false;
			}

			readBytes = sizeof(MeshNodeBonePalette) * impl->data_.numBonePalettes_;
			if(inFile.Read(impl->meshNodeBonePaletteDatas_.data(), readBytes) != readBytes)
			{
				delete impl;
				return false;
			}

			readBytes = sizeof(MeshNodeInverseBindpose) * impl->data_.numInverseBindPoses_;
			if(inFile.Read(impl->meshNodeInverseBindposeDatas_.data(), readBytes) != readBytes)
			{
				delete impl;
				return false;
			}

			impl->modelMeshes_.resize(impl->data_.numMeshes_);
			readBytes = sizeof(ModelMeshData) * impl->data_.numMeshes_;
			if(inFile.Read(impl->modelMeshes_.data(), readBytes) != readBytes)
			{
				delete impl;
				return false;
			}

			impl->elements_.resize(impl->modelMeshes_.back().endVertexElements_);
			impl->draws_.resize(impl->modelMeshes_.back().endDraws_);

			readBytes = sizeof(GPU::VertexElement) * impl->elements_.size();
			if(inFile.Read(impl->elements_.data(), readBytes) != readBytes)
			{
				delete impl;
				return false;
			}

			readBytes = sizeof(ModelMeshDraw) * impl->draws_.size();
			if(inFile.Read(impl->draws_.data(), readBytes) != readBytes)
			{
				delete impl;
				return false;
			}

			// Create materials for mesh nodes.
			// Only load them one by one.
			// TODO: We need a better heuristic to control
			// the wait policy for certain resource types.
			impl->materials_.reserve(impl->meshNodes_.size());
			for(const auto& meshNode : impl->meshNodes_)
			{
				impl->materials_.emplace_back(meshNode.material_);
				impl->materials_.back().WaitUntilReady();
				DBG_ASSERT(impl->materials_.back());
			}

			// Now load in and create vertex + index buffers.
			if(GPU::Manager::IsInitialized())
			{
				// Calculate size to map.
				i64 totalDataSize = 0;
				for(const auto& mesh : impl->modelMeshes_)
				{
					totalDataSize += mesh.noofVertices_ * mesh.vertexSize_;
					totalDataSize += mesh.noofIndices_ * mesh.indexStride_;
				}

				impl->vbs_.reserve(impl->modelMeshes_.size());
				impl->ibs_.reserve(impl->modelMeshes_.size());

				// Map data.
				if(auto mapped = Core::MappedFile(inFile, inFile.Tell(), totalDataSize))
				{
					const u8* data = reinterpret_cast<const u8*>(mapped.GetAddress());
					i32 vbIdx = 0;
					for(const auto& mesh : impl->modelMeshes_)
					{
						GPU::BufferDesc desc;
						desc.size_ = mesh.noofVertices_ * mesh.vertexSize_;
						desc.bindFlags_ = GPU::BindFlags::VERTEX_BUFFER;
						GPU::Handle vb = GPU::Manager::CreateBuffer(desc, data, "%s/vb_%d", name, vbIdx++);
						impl->vbs_.push_back(vb);

						u32 dataCrc32 = Core::HashCRC32(0, data, desc.size_);
						DBG_ASSERT(mesh.vertexDataCrc32_ == dataCrc32);

						data += desc.size_;
					}

					i32 ibIdx = 0;
					for(const auto& mesh : impl->modelMeshes_)
					{
						GPU::BufferDesc desc;
						desc.size_ = mesh.noofIndices_ * mesh.indexStride_;
						desc.bindFlags_ = GPU::BindFlags::INDEX_BUFFER;
						GPU::Handle ib = GPU::Manager::CreateBuffer(desc, data, "%s/ib_%d", name, ibIdx++);
						impl->ibs_.push_back(ib);

						u32 dataCrc32 = Core::HashCRC32(0, data, desc.size_);
						DBG_ASSERT(mesh.indexDataCrc32_ == dataCrc32);

						data += desc.size_;
					}
				}
				else
				{
					DBG_ASSERT_MSG(false, "FATAL: Unable to map model data for \"%s\"\n", inFile.GetPath());
					delete impl;
					return false;
				}

				// Create draw binding sets.
				i32 dbsIdx = 0;
				for(const auto& mesh : impl->modelMeshes_)
				{
					GPU::DrawBindingSetDesc desc;
					desc.ib_.offset_ = 0;
					desc.ib_.resource_ = impl->ibs_[dbsIdx];
					desc.ib_.size_ = mesh.noofIndices_ * mesh.indexStride_;
					desc.ib_.stride_ = mesh.indexStride_;

					auto vertexElements =
					    Core::ArrayView<GPU::VertexElement>(impl->elements_.data() + mesh.startVertexElements_,
					        impl->elements_.data() + mesh.endVertexElements_);

					i32 offset = 0;
					for(i32 streamIdx = 0; streamIdx < GPU::MAX_VERTEX_ELEMENTS; ++streamIdx)
					{
						const i32 stride = GPU::GetStride(vertexElements.data(), vertexElements.size(), streamIdx);
						if(stride > 0)
						{
							auto& vb = desc.vbs_[streamIdx];

							vb.offset_ = offset;
							vb.resource_ = impl->vbs_[dbsIdx];
							vb.size_ = mesh.noofVertices_ * stride;
							vb.stride_ = stride;

							offset += vb.size_;
						}
					}

					GPU::Handle db = GPU::Manager::CreateDrawBindingSet(desc, "%s/dbs_%d", name, dbsIdx++);
					impl->dbs_.push_back(db);
				}
			}

			// Wait until dependencies are loaded.
			impl->WaitForDependencies();

			std::swap(model->impl_, impl);
			delete impl;

			return true;
		}

		bool SerializeSettings(Serialization::Serializer& ser) override { return true; }
	};

	DEFINE_RESOURCE(Model);

	Model::Model()
	{ //
	}

	Model::~Model()
	{
		DBG_ASSERT(impl_);
		delete impl_;
	}

	i32 Model::GetNumMeshes() const { return impl_->data_.numMeshNodes_; }

	Core::ArrayView<GPU::VertexElement> Model::GetMeshVertexElements(i32 meshIdx) const
	{
		DBG_ASSERT(meshIdx < impl_->data_.numMeshNodes_);
		const auto& meshNode = impl_->meshNodes_[meshIdx];
		const i32 idx = meshNode.meshIdx_;
		if(idx >= 0)
			return Core::ArrayView<GPU::VertexElement>(
			    impl_->elements_.data() + impl_->modelMeshes_[idx].startVertexElements_,
			    impl_->elements_.data() + impl_->modelMeshes_[idx].endVertexElements_);
		else
			return Core::ArrayView<GPU::VertexElement>();
	}

	GPU::Handle Model::GetMeshDrawBinding(i32 meshIdx) const
	{
		DBG_ASSERT(meshIdx < impl_->data_.numMeshNodes_);
		const auto& meshNode = impl_->meshNodes_[meshIdx];
		const i32 idx = meshNode.meshIdx_;
		if(idx >= 0)
			return impl_->dbs_[idx];
		return GPU::Handle();
	}

	ModelMeshDraw Model::GetMeshDraw(i32 meshIdx) const
	{
		DBG_ASSERT(meshIdx < impl_->data_.numMeshNodes_);
		const auto& meshNode = impl_->meshNodes_[meshIdx];
		const i32 idx = meshNode.meshIdx_;
		const i32 drawIdx = impl_->modelMeshes_[idx].startDraws_ + meshNode.drawIdx_;
		ModelMeshDraw retVal;
		if(idx >= 0)
			return impl_->draws_[drawIdx];
		return retVal;
	}

	Material* Model::GetMeshMaterial(i32 meshIdx) const
	{
		DBG_ASSERT(meshIdx < impl_->data_.numMeshNodes_);
		return impl_->materials_[meshIdx];
	}

	Math::Mat44 Model::GetMeshWorldTransform(i32 meshIdx) const
	{
		DBG_ASSERT(meshIdx < impl_->data_.numMeshNodes_);
		const auto& meshNode = impl_->meshNodes_[meshIdx];
		return impl_->nodeDatas_.world_[meshNode.nodeIdx_];
	}

	ModelImpl::ModelImpl() {}

	ModelImpl::~ModelImpl()
	{
		if(GPU::Manager::IsInitialized())
		{
			for(auto db : dbs_)
				GPU::Manager::DestroyResource(db);
			for(auto ib : ibs_)
				GPU::Manager::DestroyResource(ib);
			for(auto vb : vbs_)
				GPU::Manager::DestroyResource(vb);
		}
	}

	void ModelImpl::WaitForDependencies()
	{
		for(const auto& material : materials_)
			material.WaitUntilReady();
	}

} // namespace Graphics
