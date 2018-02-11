#include "ecs/system.h"

namespace ECS
{
	struct SystemImpl
	{
	};

	System::System() { impl_ = new SystemImpl(); }

	System::~System() { delete impl_; }

} // namespace ECS
