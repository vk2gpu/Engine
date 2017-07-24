#pragma once
#include "resource/factory.h"

namespace Resource
{
	/// Factory context to use during creation of resources.
	class FactoryContext : public Resource::IFactoryContext
	{
	public:
		FactoryContext();
		virtual ~FactoryContext();
	};
} // namespace Resource
