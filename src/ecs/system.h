#pragma once

#include "ecs/dll.h"
#include "core/types.h"

namespace ECS
{
	class ECS_DLL System
	{
	public:
		System();
		~System();


	private:
		System(const System&) = delete;
		System& operator=(const System&) = delete;

		struct SystemImpl* impl_ = nullptr;
	};

} // namespace ECS
