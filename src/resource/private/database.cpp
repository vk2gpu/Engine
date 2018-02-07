#include "resource/private/database.h"
#include "core/misc.h"

#include <cstring>

namespace Resource
{
	Database::Database(const char* resourceRoot, Core::IFilePathResolver& resolver)
	    : resourceRoot_(resourceRoot)
	    , resolver_(resolver)
	{
	}

	Database::~Database() {}

	void Database::ScanResources()
	{
		Core::ScopedWriteLock lock(uuidLock_);
		uuidToPath_.clear();

		InternalScanResources(resourceRoot_.c_str());
	}

	void Database::InternalScanResources(const char* path)
	{
		i32 numFiles = Core::FileFindInPath(path, nullptr, nullptr, -1);
		Core::Vector<Core::FileInfo> fileInfos(numFiles);
		Core::FileFindInPath(path, nullptr, fileInfos.data(), fileInfos.size());

		for(const auto& fileInfo : fileInfos)
		{
			Core::Vector<char> absolutePath(Core::MAX_PATH_LENGTH);
			Core::FileAppendPath(absolutePath.data(), absolutePath.size(), path);
			Core::FileAppendPath(absolutePath.data(), absolutePath.size(), fileInfo.fileName_);

			// Skip hidden files.
			if(Core::ContainsAllFlags(fileInfo.attribs_, Core::FileAttribs::HIDDEN))
				continue;

			// Recurse into subfolders.
			if(Core::ContainsAllFlags(fileInfo.attribs_, Core::FileAttribs::DIRECTORY))
			{
				if(strcmp(fileInfo.fileName_, ".") != 0 && strcmp(fileInfo.fileName_, "..") != 0)
				{
					InternalScanResources(absolutePath.data());
				}
				continue;
			}

			// Check if its a metadata file.
			i32 fileNameLen = (i32)strlen(fileInfo.fileName_);
			i32 metaDataPos = fileNameLen - 9;
			const char* str = fileInfo.fileName_;
			if(strcmp(str + metaDataPos, ".metadata") == 0)
				continue;

			// Find original path for file, and generate UUID.
			Core::Vector<char> origPath(Core::MAX_PATH_LENGTH);
			if(resolver_.OriginalPath(absolutePath.data(), origPath.data(), origPath.size()))
			{
				Core::UUID uuid = origPath.data();
				auto foundPath = uuidToPath_.find(uuid);
				if(foundPath != nullptr)
				{
					if(foundPath->c_str() != origPath.data())
					{
						DBG_LOG("Resource UUID Conflict: \"%s\" has conflicting entry \"%s\"\n", origPath.data(),
						    foundPath->c_str());
						continue;
					}
				}
				uuidToPath_.insert(uuid, origPath.data());
			}
		}
	}

	Core::String Database::GetPath(const Core::UUID& uuid) const
	{
		Core::ScopedReadLock lock(uuidLock_);
		auto path = uuidToPath_.find(uuid);
		if(path != nullptr)
			return *path;
		return Core::String();
	}

	Core::String Database::GetPathRescan(const Core::UUID& uuid)
	{
		auto retVal = GetPath(uuid);
		if(retVal.size() == 0)
		{
			ScanResources();
			retVal = GetPath(uuid);
		}
		return retVal;
	}

} // namespace Resource
