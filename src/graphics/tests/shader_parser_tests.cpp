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
			const char* paths[] = {resolvePath_};

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
		ShaderParserCallbacks(const char* logFile)
		    : logFile_(logFile, Core::FileFlags::WRITE | Core::FileFlags::CREATE)
		{
		}

		void OnError(Graphics::ErrorType errorType, const char* fileName, i32 lineNumber, i32 lineOffset,
		    const char* line, const char* str) override
		{
			auto outLine = Core::String().Printf(
			    "%s(%i-%i): error: %u: %s\n", fileName, lineNumber, lineOffset, (u32)errorType, str);

			outLine.Appendf("> %s\n> ", line);
			for(i32 idx = 1; idx < lineOffset; ++idx)
				outLine.Appendf(" ");
			outLine.Appendf("^\n");

			Core::Log(" %s", outLine.c_str());
			logFile_.Write(outLine.data(), outLine.size());
		}

		Core::File logFile_;
	};

	bool compareFiles(const char* a, const char* b)
	{
		Core::File fileA(a, Core::FileFlags::READ);
		Core::File fileB(b, Core::FileFlags::READ);

		if(fileA.Size() != fileB.Size())
			return false;

		Core::Vector<char> dataA;
		Core::Vector<char> dataB;
		dataA.resize((i32)fileA.Size());
		dataB.resize((i32)fileB.Size());
		fileA.Read(dataA.data(), dataA.size());
		fileB.Read(dataB.data(), dataB.size());

		return memcmp(dataA.data(), dataB.data(), dataA.size()) == 0;
	}
}

TEST_CASE("graphics-tests-shader-parser")
{
	const char* testPath = "../../../../res/shader_tests/parser";
	PathResolver pathResolver(testPath);

	// Gather all esf files in "res/shader_tests/parser"
	Core::Vector<Core::FileInfo> fileInfos;
	i32 numFiles = Core::FileFindInPath(pathResolver.resolvePath_, "esf", nullptr, 0);
	fileInfos.resize(numFiles);
	Core::FileFindInPath(pathResolver.resolvePath_, "esf", fileInfos.data(), fileInfos.size());

#if 0
	fileInfos.clear();
	Core::FileInfo testFileInfo;
	strcpy_s(testFileInfo.fileName_, sizeof(testFileInfo.fileName_), "attribute-03.esf");
	fileInfos.push_back(testFileInfo);
#endif

	// Create temporary log path.
	auto logPath = Core::String().Printf("%s/logs/tmp", testPath);
	Core::FileCreateDir(logPath.c_str());

	for(const auto& fileInfo : fileInfos)
	{
		Core::File shaderFile(fileInfo.fileName_, Core::FileFlags::READ, &pathResolver);
		if(shaderFile)
		{
			Core::Log("Parsing %s...\n", fileInfo.fileName_);

			auto compareFileName = Core::String().Printf("%s/logs/%s.log", testPath, fileInfo.fileName_);
			auto logFileName = Core::String().Printf("%s/logs/tmp/%s.log", testPath, fileInfo.fileName_);

			// If log exists, remove it.
			if(Core::FileExists(logFileName.c_str()))
			{
				Core::FileRemove(logFileName.c_str());
			}

			Core::String shaderCode;
			shaderCode.resize((i32)shaderFile.Size());
			shaderFile.Read(shaderCode.data(), shaderCode.size());

			{
				Graphics::ShaderParser shaderParser;
				ShaderParserCallbacks callbacks(logFileName.c_str());
				shaderParser.Parse(fileInfo.fileName_, shaderCode.c_str(), &callbacks);
			}

			CHECK(compareFiles(compareFileName.c_str(), logFileName.c_str()));
		}
	}
}
