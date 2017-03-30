#pragma once

#include "core/types.h"
#include "core/uuid.h"
#include "resource/dll.h"

namespace Core
{
	class File;
} // namespace Core

namespace Resource
{
	/**
	 * Resource loading context.
	 * Passed into ILoader during some operations for error reporting,
	 * handle allocation, or even loading other resources.
	 */
	class ILoaderContext
	{
	public:
		virtual ~ILoaderContext() {}
	};

	/**
	 * Resource loading interface.
	 * Can be used to implement loading of multiple types of resources.
	 * 
	 */
	class ILoader
	{
	public:
		virtual ~ILoader() {}

		/**
		 * Load resource from file.
		 * @param context Loader context.
		 * @param uuid Resource type UUID
		 * @param outResource Output resource.
		 * @param inFile File to load resource from.
		 */
		virtual bool LoadResource(ILoaderContext& context, Core::UUID uuid, void** outResource, Core::File& inFile) = 0;

		/**
		 * Unload resource.	
		 * @param context Loader context.
		 * @param uuid Resource type UUID
		 * @param inResource Input resource to unload.
		 */
		virtual bool UnloadResource(ILoaderContext& context, Core::UUID uuid, void** inResource) = 0;

	private:

	};
} // namespace Resource
