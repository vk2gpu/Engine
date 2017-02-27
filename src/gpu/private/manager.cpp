#include "gpu/manager.h"
#include "gpu/types.h"
#include "gpu/resources.h"
#include "core/array.h"
#include "core/debug.h"
#include "core/handle.h"


namespace GPU
{
	struct ManagerImpl
	{
		Core::HandleAllocator handles_ = Core::HandleAllocator(ResourceType::MAX);
	};

	Manager::Manager()
	{ //
		impl_ = new ManagerImpl();
	}

	Manager::~Manager()
	{ //
		delete impl_;
	}

	bool Manager::IsValidHandle(Handle handle) const
	{ //
		return impl_->handles_.IsValid(handle);
	}

} // namespace GPU
