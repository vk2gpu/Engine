#include "catch.hpp"

#include "core/debug.h"
#include "core/file.h"
#include "core/random.h"
#include "core/timer.h"
#include "core/vector.h"
#include "core/os.h"
#include "job/manager.h"
#include "plugin/manager.h"
#include "resource/manager.h"

#include "graphics/converters/shader_parser.h"

namespace
{
	class PathResolver : public Core::IFilePathResolver
	{
	public:
		PathResolver(const char* resolvePath)
		    : resolvePath_(resolvePath)
		{
		}

		virtual ~PathResolver() {}

		bool ResolvePath(const char* inPath, char* outPath, i32 maxOutPath)
		{
			const char* paths[] = {"", resolvePath_};

			char intPath[Core::MAX_PATH_LENGTH];
			for(i32 i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i)
			{
				memset(intPath, 0, sizeof(intPath));
				Core::FileAppendPath(intPath, sizeof(intPath), paths[i]);
				Core::FileAppendPath(intPath, sizeof(intPath), inPath);
				if(Core::FileExists(intPath))
				{
					strcpy_s(outPath, maxOutPath, intPath);
					return true;
				}
			}

			return false;
		}

		const char* resolvePath_ = nullptr;
	};

	class ShaderParserCallbacks : public Graphics::IShaderParserCallbacks
	{
	public:
		void OnError(Graphics::ErrorType errorType, const char* fileName, i32 lineNumber, i32 lineOffset,
		    const char* str) override
		{
			Core::Log(" - %s(%i-%i): error: %u: %s\n", fileName, lineNumber, lineOffset, (u32)errorType, str);
		}
	};
}

TEST_CASE("graphics-tests-shader-parser")
{
	PathResolver pathResolver("../../../../res/shader_tests/parser");

	// Gather all esf files in "res/shader_tests/parser"
	Core::Vector<Core::FileInfo> fileInfos;
	i32 numFiles = Core::FileFindInPath(pathResolver.resolvePath_, "esf", nullptr, 0);
	fileInfos.resize(numFiles);
	Core::FileFindInPath(pathResolver.resolvePath_, "esf", fileInfos.data(), fileInfos.size());

#if 0
	fileInfos.clear();
	Core::FileInfo testFileInfo;
	strcpy_s(testFileInfo.fileName_, sizeof(testFileInfo.fileName_), "function-05-err.esf");
	fileInfos.push_back(testFileInfo);
#endif

	for(const auto& fileInfo : fileInfos)
	{
		Core::File shaderFile(fileInfo.fileName_, Core::FileFlags::READ, &pathResolver);
		if(shaderFile)
		{
			Core::Log("Parsing %s...\n", fileInfo.fileName_);

			Core::String shaderCode;
			shaderCode.resize((i32)shaderFile.Size());
			shaderFile.Read(shaderCode.data(), shaderCode.size());

			Graphics::ShaderParser shaderParser;
			ShaderParserCallbacks callbacks;
			shaderParser.Parse(fileInfo.fileName_, shaderCode.c_str(), &callbacks);
		}
	}
}
