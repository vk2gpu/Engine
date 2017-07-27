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
			Model* shader = *reinterpret_cast<Model**>(inResource);
			DBG_ASSERT(shader);

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

	ModelImpl::ModelImpl() {}

	ModelImpl::~ModelImpl() {}

} // namespace Graphics
