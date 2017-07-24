#include "resource/private/path_resolver.h"
#include "serialization/serializer.h"

namespace Resource
{
	PathResolver::PathResolver() { Core::FileGetCurrDir(rootPath_.data(), rootPath_.size()); }

	PathResolver::~PathResolver() {}

	bool PathResolver::AddPath(const char* path)
	{
		if(Core::FileExists(path))
		{
			searchPaths_.push_back(path);
			return true;
		}

		return false;
	}

	bool PathResolver::ResolvePath(const char* inPath, char* outPath, i32 maxOutPath)
	{
		char intPath[Core::MAX_PATH_LENGTH];
		for(const auto& searchPath : searchPaths_)
		{
			memset(intPath, 0, sizeof(intPath));
			Core::FileAppendPath(intPath, sizeof(intPath), searchPath.c_str());
			Core::FileAppendPath(intPath, sizeof(intPath), inPath);
			if(Core::FileExists(intPath))
			{
				strcpy_s(outPath, maxOutPath, intPath);
				return true;
			}
		}

		return false;
	}

} // namespace Resource
