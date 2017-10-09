#pragma once

#include "core/concurrency.h"
#include "core/file.h"
#include "core/map.h"
#include "core/string.h"
#include "core/uuid.h"

namespace Resource
{
	class Database
	{
	public:
		Database(const char* resourceRoot, Core::IFilePathResolver& resolver);
		~Database();

		/**
		 * Scan resources.
		 */
		void ScanResources();

		/**
		 * Get path from UUID.
		 */
		Core::String GetPath(const Core::UUID& uuid) const;

		/**
		 * Get path from UUID, but rescan if it can't be found.
		 */
		Core::String GetPathRescan(const Core::UUID& uuid);

	private:
		void InternalScanResources(const char* path);

		Core::String resourceRoot_;
		Core::IFilePathResolver& resolver_;
		Core::Map<Core::UUID, Core::String> uuidToPath_;
		mutable Core::RWLock uuidLock_;
	};

} // namespace Resource
