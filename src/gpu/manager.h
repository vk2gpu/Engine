#pragma once

#include "core/types.h"
#include "gpu/dll.h"
#include "gpu/types.h"
#include "gpu/resources.h"

namespace GPU
{
	class GPU_DLL Manager final
	{
	public:
		Manager();
		~Manager();

		/**
		 * Is valid handle?
		 */
		bool IsValidHandle(Handle handle) const;


	private:
		Manager(const Manager&) = delete;
		struct ManagerImpl* impl_ = nullptr;
	};
} // namespace GPU
