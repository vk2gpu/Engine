#include "resource/ref.h"
#include "resource/manager.h"

#include "core/debug.h"

namespace Resource
{
	RefBase::RefBase() {}

	RefBase::RefBase(const char* name, const Core::UUID& type) { Manager::RequestResource(resource_, name, type); }

	RefBase::RefBase(const Core::UUID& uuid, const Core::UUID& type)
	{
		Manager::RequestResource(resource_, uuid, type);
	}

	RefBase::~RefBase() { Reset(); }

	RefBase::RefBase(RefBase&& other) { std::swap(resource_, other.resource_); }

	RefBase& RefBase::operator=(RefBase&& other)
	{
		std::swap(resource_, other.resource_);
		return *this;
	}

	void RefBase::Reset()
	{
		if(resource_)
			Manager::ReleaseResource(resource_);
	}

	bool RefBase::IsReady() const
	{
		DBG_ASSERT(resource_);
		return Manager::IsResourceReady(resource_);
	}

	void RefBase::WaitUntilReady() const
	{
		DBG_ASSERT(resource_);
		Manager::WaitForResource(resource_);
	}

} // namespace Resource
