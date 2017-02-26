#include "client/window.h"

#include "catch.hpp"

#include "core/file.h"
#include "plugin/manager.h"

namespace
{
}

TEST_CASE("plugin-tests-scan")
{
	Plugin::Manager manager;

	char path[Core::MAX_PATH_LENGTH];
	Core::FileGetCurrDir(path, Core::MAX_PATH_LENGTH);
	i32 found = manager.Scan("../../output/bin/Debug");
	REQUIRE(found > 0);
}

TEST_CASE("plugin-tests-basic-plugin")
{
	Plugin::Manager manager;

	char path[Core::MAX_PATH_LENGTH];
	Core::FileGetCurrDir(path, Core::MAX_PATH_LENGTH);
	i32 found = manager.Scan("../../output/bin/Debug");
	REQUIRE(found > 0);
}

