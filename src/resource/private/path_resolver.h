#pragma once

#include "core/file.h"
#include "core/string.h"
#include "core/vector.h"

namespace Resource
{
	class PathResolver : public Core::IFilePathResolver
	{
	public:
		PathResolver();
		virtual ~PathResolver();

		/**
		 * Add path to use for resolution.
		 */
		bool AddPath(const char* path);

		bool ResolvePath(const char* inPath, char* outPath, i32 maxOutPath) override;

	private:
		Core::Array<char, Core::MAX_PATH_LENGTH> rootPath_;
		Core::Vector<Core::String> searchPaths_;
	};

} // namespace Resource
