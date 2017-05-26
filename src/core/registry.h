#pragma once

#include "core/dll.h"
#include "core/types.h"

namespace Core
{
	class UUID;

	/**
	 * Registry for the storage/retrieval of objects.
	 */
	class Registry
	{
	public:
		Registry();
		~Registry();

		/**
		 * Set entry in registry.
		 * @param uuid UUID of object to store as.
		 * @param value Value to store in registry.
		 */
		void Set(const Core::UUID& uuid, void* value);

		/**
		 * Get entry from registry.
		 * @param uuid UUID of entry.
		 * @return nullptr if not set, or object.
		 */
		void* Get(const Core::UUID& uuid);

		template<typename TYPE>
		TYPE* Get(const Core::UUID& uuid)
		{
			return reinterpret_cast<TYPE*>(Get(uuid));
		}

	private:
		Registry(const Registry&) = delete;
		Registry& operator=(const Registry&) = delete;

		struct RegistryImpl* impl_ = nullptr;
	};
} // namespace Core
