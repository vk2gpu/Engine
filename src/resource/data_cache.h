#pragma once

#include "resource/dll.h"
#include "resource/types.h"
#include "core/hash.h"

namespace Resource
{
	/**
	 * Hash to use with the data cache.
	 */
	using DataHash = Core::HashSHA1Digest;

	/**
	 * 
	 */

	/**
	 * Data cache.
	 * Used for arbitrarily storing and retrieving data from a set of
	 * locations (network, web address, file system) based upon a hash.
	 */
	class RESOURCE_DLL DataCache
	{
	public:
		DataCache();
		~DataCache();

		/**
		 * Does data exist in cache?
		 * @param hash Hash used to store.
		 */
		bool Exists(const DataHash& hash);

		/**
		 * Get data size.
		 * @param hash Hash used to store.
		 * @param >= 0 if success.
		 */
		i64 GetSize(const DataHash& hash);

		/**
		 * Write data to cache.
		 * @param hash Hash to store with.
		 * @param data Pointer to data.
		 * @param size Size of data.
		 * @return true if successfully written.
		 */
		bool Write(const DataHash& hash, const void* data, i64 size);

		/**
		 * Read data from cache.
		 * @param hash Hash used to store.
		 * @param data Pointer to data.
		 * @param size Size of data.
		 * @return true if successfully read.
		 */
		bool Read(const DataHash& hash, void* data, i64 size);


	private:
		struct DataCacheImpl* impl_ = nullptr;

	};

} // namespace Resource