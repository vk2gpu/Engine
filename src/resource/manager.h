#pragma once

#include "core/types.h"
#include "resource/dll.h"
#include "resource/types.h"
#include "job/concurrency.h"

namespace Core
{
	class File;
	class UUID;
} // namespace Core

namespace Job
{
	class Manager;
} // namespace Job

namespace Plugin
{
	class Manager;
} // namespace Plugin

namespace Resource
{
	class IFactory;

	class RESOURCE_DLL Manager final
	{
	public:
		/**
		 * Initialize resource manager.
		 */
		static void Initialize();

		/**
		 * Finalize resource manager.
		 */
		static void Finalize();

		/**
		 * Is resource manager initialized?
		 */
		static bool IsInitialized();

		/**
		 * Wait for reloading to complete.
		 */
		static void WaitOnReload();

		/**
		 * Take reload lock.
		 */
		static Job::ScopedWriteLock TakeReloadLock();

		/**
		 * Request resource by name & type.
		 * If a resource is loaded, it
		 * @param outResource Output resource.
		 * @param name Name of resource.
		 * @param type Type of resource.
		 * @return true if success.
		 */
		static bool RequestResource(void*& outResource, const char* name, const Core::UUID& type);
		template<typename TYPE>
		static bool RequestResource(TYPE*& outResource, const char* name)
		{
			return RequestResource(reinterpret_cast<void*&>(outResource), name, TYPE::GetTypeUUID());
		}

		/**
		 * Release resource.
		 * @param inResource Resource to release.
		 * @return true if success.
		 */
		static bool ReleaseResource(void*& inResource);
		template<typename TYPE>
		static bool ReleaseResource(TYPE*& inResource)
		{
			return ReleaseResource(reinterpret_cast<void*&>(inResource));
		}

		/**
		 * Is resource ready?
		 * @retunr true if resource is ready.
		 */
		static bool IsResourceReady(void* inResource);

		/**
		 * Wait for resource to become ready.
		 */
		static void WaitForResource(void* inResource);

		/**
		 * Register factory.
		 * @param type Type to register factory for.
		 * @param factory Factory for resource creation.
		 * @return true for success. false if can't register (i.e. already registered).
		 */
		static bool RegisterFactory(const Core::UUID& type, IFactory* factory);
		template<typename TYPE>
		static bool RegisterFactory(IFactory* factory)
		{
			return RegisterFactory(TYPE::GetTypeUUID(), factory);
		}

		/**
		 * Unregister factory.
		 * @param factory Factory to unregister. Will unregister for all types it references.
		 * @param true for success. false if not registered.
		 */
		static bool UnregisterFactory(IFactory* factory);

		/**
		 * Read file data either synchronously or asynchronously.
		 * @param file File to read from.
		 * @param offset Offset to read from.
		 * @param size Size to read from.
		 * @param dest Destination address.
		 * @param result Async result. If nullptr, read is immediate.
		 * @pre @a file is valid for reading.
		 * @pre offset >= 0.
		 * @pre size > 0.
		 * @pre dest != nullptr.
		 */
		static Result ReadFileData(Core::File& file, i64 offset, i64 size, void* dest, AsyncResult* result = nullptr);

		/**
		 * Write file data either synchronously or asynchronously.
		 * @param file File to write to.
		 * @param size Size to write out.
		 * @param src Source address.
		 * @param result Async result. If nullptr, write is immediate.
		 * @pre @a file is valid for writing.
		 * @pre size > 0.
		 * @pre src != nullptr.
		 */
		static Result WriteFileData(Core::File& file, i64 size, void* src, AsyncResult* result = nullptr);

		/**
		 * Scoped manager init/fini.
		 * Mostly a convenience for unit tests.
		 */
		class Scoped
		{
		public:
			Scoped() { Initialize(); }
			~Scoped() { Finalize(); }
		};

	private:
		Manager() = delete;
		~Manager() = delete;
	};

} // namespace Plugin
