#pragma once

#include "core/types.h"
#include "resource/dll.h"
#include "resource/types.h"

namespace Core
{
	class UUID;

} // namespace Core

namespace Resource
{
	/**
	 * Resource database.
	 */
	class RESOURCE_DLL Database final
	{
	public:
		Database();
		~Database();

		/**
		 * Add resource to database.
		 * @param outResourceUuid Output UUID for resource.
		 * @param fileName Resource name.
		 * @return true for success, false for failure.
		 */
		bool AddResource(Core::UUID& outResourceUuid, const char* name);

		/**
		 * Add resource dependencies.
		 * Will check for circular dependencies.
		 * @param resourceUuid Resource to add dependency to.
		 * @param depUuids Resource to add as dependencies.
		 * @param numDeps Number of dependencies.
		 * @return true if complete success. false if any failures.
		 */
		bool AddDependencies(const Core::UUID& resourceUuid, const Core::UUID* depUuids, i32 numDeps);

	private:
		struct DatabaseImpl* impl_ = nullptr;
	};

} // namespace Resource
