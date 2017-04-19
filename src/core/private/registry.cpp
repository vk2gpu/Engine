#include "core/registry.h"
#include "core/debug.h"
#include "core/map.h"
#include "core/uuid.h"

#include <utility>

namespace Core
{
	struct RegistryImpl
	{
		Core::Map<Core::UUID, void*> entries_;
	};

	Registry::Registry()
	{
		impl_ = new RegistryImpl();
	}

	Registry::~Registry()
	{
		delete impl_;
	}

	void Registry::Set(const Core::UUID& uuid, void* value)
	{
		DBG_ASSERT(impl_);
		impl_->entries_.insert(uuid, value);
	}
		
	void* Registry::Get(const Core::UUID& uuid)
	{
		DBG_ASSERT(impl_);
		auto it = impl_->entries_.find(uuid);
		if(it != impl_->entries_.end())
			return it->second;
		return nullptr;
	}

} // namespace Core
