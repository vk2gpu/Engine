#include "graphics/model.h"
#include "graphics/private/model_impl.h"

#include "resource/factory.h"
#include "resource/manager.h"

#include "core/debug.h"
#include "core/file.h"
#include "core/hash.h"
#include "core/misc.h"
#include "gpu/manager.h"

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

			// Now load in and create vertex + index buffers.
			if(GPU::Manager::IsInitialized())
			{
				Core::Vector<u8> data;
				for(const auto& mesh : impl->modelMeshes_)
				{
					readBytes = mesh.noofVertices_ * mesh.vertexStride_;
					if(data.capacity() < readBytes)
						data.resize((i32)readBytes);

					if(inFile.Read(data.data(), readBytes) != readBytes)
					{
						delete impl;
						return false;
					}

					{
						GPU::BufferDesc desc;
						desc.size_ = readBytes;
						desc.bindFlags_ = GPU::BindFlags::VERTEX_BUFFER;
						GPU::Handle vb = GPU::Manager::CreateBuffer(desc, data.data(), name);
						impl->vbs_.push_back(vb);
					}
				}

				for(const auto& mesh : impl->modelMeshes_)
				{
					readBytes = mesh.noofIndices_ * mesh.indexStride_;
					if(data.capacity() < readBytes)
						data.resize((i32)readBytes);

					if(inFile.Read(data.data(), readBytes) != readBytes)
					{
						delete impl;
						return false;
					}

					{
						GPU::BufferDesc desc;
						desc.size_ = readBytes;
						desc.bindFlags_ = GPU::BindFlags::INDEX_BUFFER;
						GPU::Handle ib = GPU::Manager::CreateBuffer(desc, data.data(), name);
						impl->ibs_.push_back(ib);
					}
				}

				// Create draw binding sets.
				i32 idx = 0;
				for(const auto& mesh : impl->modelMeshes_)
				{
					GPU::DrawBindingSetDesc desc;
					desc.ib_.offset_ = 0;
					desc.ib_.resource_ = impl->ibs_[idx];
					desc.ib_.size_ = mesh.noofIndices_ * mesh.indexStride_;
					desc.ib_.stride_ = mesh.indexStride_;
					desc.vbs_[0].offset_ = 0;
					desc.vbs_[0].resource_ = impl->vbs_[idx];
					desc.vbs_[0].offset_ = mesh.noofVertices_ * mesh.vertexStride_;
					desc.vbs_[0].stride_ = mesh.vertexStride_;
					GPU::Handle db = GPU::Manager::CreateDrawBindingSet(desc, name);
					impl->dbs_.push_back(db);
				}
			}

			std::swap(model->impl_, impl);
			delete impl;

			return true;
		}
	};

	static ModelFactory* factory_ = nullptr;

	void Model::RegisterFactory()
	{
		DBG_ASSERT(factory_ == nullptr);
		factory_ = new ModelFactory();
		Resource::Manager::RegisterFactory<Model>(factory_);
	}

	void Model::UnregisterFactory()
	{
		DBG_ASSERT(factory_ != nullptr);
		Resource::Manager::UnregisterFactory(factory_);
		delete factory_;
		factory_ = nullptr;
	}

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

} // namespace Graphics
