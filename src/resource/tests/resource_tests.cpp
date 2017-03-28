#include "catch.hpp"

#include "core/debug.h"
#include "core/file.h"
#include "core/timer.h"
#include "core/os.h"
#include "resource/manager.h"

namespace
{
}

TEST_CASE("resource-tests-manager")
{
	Resource::Manager manager;

	static const i64 TEST_BUFFER_SIZE = 256 * 1024 * 1024;
	const char* testFileName = "test_output.dat";

	{
		auto file = Core::File(testFileName, Core::FileFlags::WRITE | Core::FileFlags::CREATE);
		REQUIRE(file);

		u8* buffer = new u8[TEST_BUFFER_SIZE];
		Resource::AsyncResult result;

		Core::Timer timer;
		timer.Mark();
		manager.WriteFileData(file, TEST_BUFFER_SIZE, buffer, &result);
		do
		{
			Core::Log("%.2fms: Writing file %u%%...\n", timer.GetTime() * 1000.0,
			    (u32)((f32)result.bytesProcessed_ / (f32)file.Size() * 100.0f));
		} while(result.workRemaining_ > 0);
		Core::Log("%.2fms: Writing file complete!\n", timer.GetTime() * 1000.0);

		delete[] buffer;
	}

	REQUIRE(Core::FileExists(testFileName));

	{
		auto file = Core::File(testFileName, Core::FileFlags::READ);
		REQUIRE(file);

		REQUIRE(file.Size() == TEST_BUFFER_SIZE);

		u8* buffer = new u8[file.Size()];
		Resource::AsyncResult result;

		Core::Timer timer;
		timer.Mark();
		manager.ReadFileData(file, 0, file.Size(), buffer, &result);
		do
		{
			Core::Log("%.2fms: Reading file %u%%...\n", timer.GetTime() * 1000.0,
			    (u32)((f32)result.bytesProcessed_ / (f32)file.Size() * 100.0f));
		} while(result.workRemaining_ > 0);
		Core::Log("%.2fms: Reading file complete!\n", timer.GetTime() * 1000.0);

		delete[] buffer;
	}

	Core::FileRemove(testFileName);
}
