#pragma once

#include "core/types.h"
#include "resource/dll.h"
#include "resource/types.h"

namespace Core
{
	class File;
}

namespace Resource
{
	class RESOURCE_DLL Manager final
	{
	public:
		Manager();
		~Manager();

		/**
		 * Read file data either synchronously or asynchronously.
		 * @param file File to read from.
		 * @param offset Offset to read from.
		 * @param size Size to read from.
		 * @param dest Destination address.
		 * @param result Async result. If nullptr, read is immediate.
		 */
		bool ReadFileData(Core::File& file, i64 offset, i64 size, void* dest, AsyncResult* result = nullptr);

		/**
		 * Write file data either synchronously or asynchronously.
		 * @param file File to write to.
		 * @param size Size to write out.
		 * @param src Source address.
		 * @param result Async result. If nullptr, write is immediate.
		 */
		bool WriteFileData(Core::File& file, i64 size, void* src, AsyncResult* result = nullptr);


	private:
		Manager(const Manager&) = delete;
		struct ManagerImpl* impl_ = nullptr;
	};
} // namespace Plugin
