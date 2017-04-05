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
		 * Add resource to database by name.
		 * @param outResourceUuid Output UUID for resource.
		 * @param fileName Resource name.
		 * @return true for success, false for failure.
		 */
		bool AddResource(Core::UUID& outResourceUuid, const char* name);

		/**
		 * Remove resource.
		 * @param resourceUuid Resource to remove.
		 * @param removeDependents Remove resource that depend upon this one.
		 */
		bool RemoveResource(const Core::UUID& resourceUuid, bool removeDependents);

		/**
		 * Add resource dependencies.
		 * Will check for circular dependencies.
		 * @param resourceUuid Resource to add dependency to.
		 * @param depUuids Resource to add as dependencies.
		 * @param numDeps Number of dependencies.
		 * @return true if complete success. false if any failures.
		 */
		bool AddDependencies(const Core::UUID& resourceUuid, const Core::UUID* depUuids, i32 numDeps);

		/**
		 * Remove dependencies.
		 * @param depUuid Dependency to remove.
		 * @param resourceUuids Resource to remove dependency from.
		 * @param numResources Number of resources.
		 */
		bool RemoveDependency(const Core::UUID& depUuid, const Core::UUID* resourceUuids, i32 numResources);

		/**
		 * Get dependencies for a resource.
		 * @param Pointer to array of UUIDs to fill.
		 * @param maxDeps Maximum number of dependencies to get.
		 * @param resourceUuid Resource to get deps for.
		 * @param recursivelyGather Recursively gather entire list of dependencies.
		 * @return Number of dependencies.
		 */
		i32 GetDependencies(
		    Core::UUID* outDeps, i32 maxDeps, const Core::UUID& resourceUuid, bool recursivelyGather) const;

		/**
		 * Get dependents for a resource.
		 * @param Pointer to array of UUIDs to fill.
		 * @param maxDeps Maximum number of dependents to get.
		 * @param resourceUuid Resource to find all dependents for.
		 * @return Number of dependents.
		 */
		i32 GetDependents(Core::UUID* outDeps, i32 maxDeps, const Core::UUID& resourceUuid) const;

		/**
		 * Set resource data in database.
		 * @param inData Input resource data.
		 * @param resourceUuid Resource UUID.
		 * @return true for success, false for failure.
		 */
		bool SetResourceData(void* inData, const Core::UUID& resourceUuid);

		/**
		 * Get resource data from database.
		 * @param outData Output resource data.
		 * @param resourceUuid Resource UUID.
		 * @return true for success, false for failure.
		 */
		bool GetResourceData(void** outData, const Core::UUID& resourceUuid) const;


	private:
		struct DatabaseImpl* impl_ = nullptr;
	};

} // namespace Resource
