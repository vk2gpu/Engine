#include "catch.hpp"

#include "core/debug.h"
#include "core/file.h"
#include "core/random.h"
#include "core/timer.h"
#include "core/vector.h"
#include "core/os.h"
#include "resource/manager.h"

namespace
{
}

TEST_CASE("resource-tests-file-io")
{
	Resource::Manager manager;

	static const i32 TEST_BUFFER_SIZE = 256 * 1024 * 1024;
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
			    (u32)((f32)result.bytesProcessed_ / (f32)TEST_BUFFER_SIZE * 100.0f));
		} while(result.workRemaining_ > 0);
		Core::Log("%.2fms: Writing file complete!\n", timer.GetTime() * 1000.0);
	}

	REQUIRE(Core::FileExists(testFileName));

	{
		auto file = Core::File(testFileName, Core::FileFlags::READ);
		REQUIRE(file);

		REQUIRE(file.Size() == TEST_BUFFER_SIZE);

		Core::Vector<u8> inBuffer;
		inBuffer.resize(TEST_BUFFER_SIZE);
		Resource::AsyncResult result;

		Core::Timer timer;
		timer.Mark();
		manager.ReadFileData(file, 0, file.Size(), inBuffer.data(), &result);
		do
		{
			Core::Log("%.2fms: Reading file %u%%...\n", timer.GetTime() * 1000.0,
			    (u32)((f32)result.bytesProcessed_ / (f32)TEST_BUFFER_SIZE * 100.0f));
		} while(result.workRemaining_ > 0);
		Core::Log("%.2fms: Reading file complete!\n", timer.GetTime() * 1000.0);

		REQUIRE(memcmp(outBuffer.data(), inBuffer.data(), TEST_BUFFER_SIZE) == 0);
	}

	Core::FileRemove(testFileName);
}
