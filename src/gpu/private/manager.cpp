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
		void* deviceWindow_ = nullptr;
		Core::HandleAllocator handles_ = Core::HandleAllocator(ResourceType::MAX);
	};

	Manager::Manager(void* deviceWindow)
	{ //
		impl_ = new ManagerImpl();
		impl_->deviceWindow_ = deviceWindow;
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
