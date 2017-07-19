#pragma once

#include "core/types.h"
#include "resource/dll.h"

namespace Core
{
	class File;
	class UUID;
} // namespace Core

namespace Resource
{
	/**
	 * Resource factory context.
	 * Passed into IFactory during some operations for error reporting,
	 * handle allocation, or even loading other resources.
	 */
	class RESOURCE_DLL IFactoryContext
	{
	public:
		virtual ~IFactoryContext() {}
	};

	/**
	 * Resource factor interface.
	 * Can be used to implement loading of multiple types of resources.
	 * 
	 */
	class RESOURCE_DLL IFactory
	{
	public:
		virtual ~IFactory() {}

		/**
		 * Create empty resource.
		 * Implementation must be thread-safe.
		 * @param context Factory context.
		 * @param outResource Output resource.
		 * @param type Type UUID.
		 */
		virtual bool CreateResource(IFactoryContext& context, void** outResource, const Core::UUID& type) = 0;

		/**
		 * Load resource from file.
		 * Implementation must be thread-safe.
		 * @param context Factory context.
		 * @param inResource Resource to load into.
		 * @param type Type UUID.
		 * @param name Name of resource. Mostly for debug purposes.
		 * @param inFile File to load resource from.
		 */
		virtual bool LoadResource(IFactoryContext& context, void** inResource, const Core::UUID& type, const char* name,
		    Core::File& inFile) = 0;

		/**
		 * Destroy resource.	
		 * Implementation must be thread-safe.
		 * @param context Factory context.
		 * @param inResource Input resource to unload.
		 * @param type Type UUID.
		 */
		virtual bool DestroyResource(IFactoryContext& context, void** inResource, const Core::UUID& type) = 0;

	private:
	};
} // namespace Resource
