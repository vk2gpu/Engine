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
#include "resource/converter.h"

namespace
{
	class ConverterContext : public Resource::IConverterContext
	{
	public:
		ConverterContext() {}

		virtual ~ConverterContext() {}

		void AddDependency(const char* fileName) override { Core::Log("AddDependency: %s\n", fileName); }

		void AddOutput(const char* fileName) override { Core::Log("AddOutput: %s\n", fileName); }

		void AddError(const char* errorFile, int errorLine, const char* errorMsg) override
		{
			if(errorFile)
			{
				Core::Log("%s(%d): %s\n", errorFile, errorLine, errorMsg);
			}
			else
			{
				Core::Log("%s\n", errorMsg);
			}
		}
	};
}

TEST_CASE("resource-tests-file-io")
{
	Job::Manager jobManager(1, 256, 32 * 1024);
	Plugin::Manager pluginManager;
	Resource::Manager manager(jobManager, pluginManager);

	static const i32 TEST_BUFFER_SIZE = 32 * 1024 * 1024;
	const char* testFileName = "test_output.dat";

	// Fill buffer with random data.
	Core::Random rng;
	Core::Vector<u8> outBuffer;
	outBuffer.resize(TEST_BUFFER_SIZE);
	u32* outData = reinterpret_cast<u32*>(outBuffer.data());
	for(i32 i = 0; i < (TEST_BUFFER_SIZE / 4); ++i)
	{
		*outData++ = rng.Generate();
	}

	{
		auto file = Core::File(testFileName, Core::FileFlags::WRITE | Core::FileFlags::CREATE);
		REQUIRE(file);

		Resource::AsyncResult result;

		Core::Timer timer;
		timer.Mark();
		manager.WriteFileData(file, TEST_BUFFER_SIZE, outBuffer.data(), &result);
		do
		{
			Core::Log("%.2fms: Writing file %u%%...\n", timer.GetTime() * 1000.0,
			    (u32)((f32)(TEST_BUFFER_SIZE - result.workRemaining_) / (f32)TEST_BUFFER_SIZE * 100.0f));
		} while(!result.IsComplete());
		Core::Log("%.2fms: Writing file complete!\n", timer.GetTime() * 1000.0);

		REQUIRE(result.result_ == Resource::Result::SUCCESS);
	}

	REQUIRE(Core::FileExists(testFileName));

	// Check success.
	{
		auto file = Core::File(testFileName, Core::FileFlags::READ);
		REQUIRE(file);

		REQUIRE(file.Size() == TEST_BUFFER_SIZE);

		Core::Vector<u8> inBuffer;
		inBuffer.resize(TEST_BUFFER_SIZE);
		Resource::AsyncResult result;

		Core::Timer timer;
		timer.Mark();
		manager.ReadFileData(file, 0, inBuffer.size(), inBuffer.data(), &result);
		do
		{
			Core::Log("%.2fms: Reading file %u%%...\n", timer.GetTime() * 1000.0,
			    (u32)((f32)(TEST_BUFFER_SIZE - result.workRemaining_) / (f32)TEST_BUFFER_SIZE * 100.0f));
		} while(!result.IsComplete());
		Core::Log("%.2fms: Reading file complete!\n", timer.GetTime() * 1000.0);

		REQUIRE(result.result_ == Resource::Result::SUCCESS);
		REQUIRE(memcmp(outBuffer.data(), inBuffer.data(), TEST_BUFFER_SIZE) == 0);
	}

	// Check failure.
	// NOTE: It will assert internally anyway.
#if 0
	{
		auto file = Core::File(testFileName, Core::FileFlags::READ);
		REQUIRE(file);

		REQUIRE(file.Size() == TEST_BUFFER_SIZE);

		Core::Vector<u8> inBuffer;
		inBuffer.resize(TEST_BUFFER_SIZE * 2);
		Resource::AsyncResult result;

		Core::Timer timer;
		timer.Mark();
		manager.ReadFileData(file, 0, inBuffer.size(), inBuffer.data(), &result);
		do
		{
			Core::Log("%.2fms: Reading file %u%%...\n", timer.GetTime() * 1000.0,
			    (u32)((f32)(TEST_BUFFER_SIZE - result.workRemaining_) / (f32)TEST_BUFFER_SIZE * 100.0f));
		} while(!result.IsComplete());
		Core::Log("%.2fms: Reading file complete!\n", timer.GetTime() * 1000.0);

		REQUIRE(result.result_ == Resource::Result::FAILURE);
		REQUIRE(memcmp(outBuffer.data(), inBuffer.data(), TEST_BUFFER_SIZE) == 0);
	}
#endif

	Core::FileRemove(testFileName);
}

TEST_CASE("resource-tests-converter")
{
	Plugin::Manager pluginManager;

	char path[Core::MAX_PATH_LENGTH];
	Core::FileGetCurrDir(path, Core::MAX_PATH_LENGTH);
	i32 found = pluginManager.Scan(".");
	REQUIRE(found > 0);

	// Get number of converter plugins.
	found = pluginManager.GetPlugins<Resource::ConverterPlugin>(nullptr, 0);
	REQUIRE(found > 0);

	// Allocate + get all converter plugins.
	Core::Vector<Resource::ConverterPlugin> converterPlugins;
	converterPlugins.resize(found);
	found = pluginManager.GetPlugins<Resource::ConverterPlugin>(converterPlugins.data(), converterPlugins.size());
	REQUIRE(found > 0);

	// Find one that supports our file type.
	Resource::IConverter* converter = nullptr;
	Resource::ConverterPlugin converterPlugin;
	for(i32 i = 0; i < converterPlugins.size(); ++i)
	{
		converterPlugin = converterPlugins[i];
		converter = converterPlugin.CreateConverter();
		if(converter->SupportsFileType("test", Core::UUID()))
			break;
		converterPlugin.DestroyConverter(converter);
		converter = nullptr;
	}

	// Check we found one.
	REQUIRE(converter);

	// Check failure.
	ConverterContext context;
	REQUIRE(!converter->Convert(context, "failure.test", "."));

	// Setup converter input + output.
	REQUIRE(Core::FileCreateDir("converter_output"));
	Core::File testFile("converter.test", Core::FileFlags::CREATE | Core::FileFlags::WRITE);
	REQUIRE(testFile);
	const char* data = "converter.test data";
	testFile.Write(data, strlen(data));
	testFile = Core::File();

	REQUIRE(converter->Convert(context, "converter.test", "converter_output/converter.test.converted"));

	converterPlugin.DestroyConverter(converter);
}
