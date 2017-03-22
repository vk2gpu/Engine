#include "client/window.h"

#include "catch.hpp"

#include "core/file.h"
#include "plugin/manager.h"


// Plugins.
#include "plugin_test_basic.h"

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

	PluginTestBasic plugin;
	PluginTestBasic* pluginArray = &plugin;

	found = manager.GetPlugins(&pluginArray, 1);
	REQUIRE(found > 0);
	REQUIRE(plugin.successfullyLoaded_ == true);
	REQUIRE(plugin.testMagic_ == PluginTestBasic::TEST_MAGIC);
}

TEST_CASE("plugin-tests-reload")
{
	Plugin::Manager manager;

	char path[Core::MAX_PATH_LENGTH];
	Core::FileGetCurrDir(path, Core::MAX_PATH_LENGTH);
	i32 found = manager.Scan("../../output/bin/Debug");
	REQUIRE(found > 0);

	PluginTestBasic plugin;
	PluginTestBasic* pluginArray = &plugin;

	found = manager.GetPlugins(&pluginArray, 1);
	REQUIRE(found > 0);
	REQUIRE(plugin.successfullyLoaded_ == true);
	REQUIRE(plugin.testMagic_ == PluginTestBasic::TEST_MAGIC);

	// Test initial state.
	REQUIRE(plugin.GetNumber() == 0);
	plugin.SetNumber(1);
	REQUIRE(plugin.GetNumber() == 1);

	bool reloaded = manager.Reload(plugin);
	REQUIRE(reloaded);

	// Test reloading has reset state.
	REQUIRE(plugin.GetNumber() == 0);
	plugin.SetNumber(1);
	REQUIRE(plugin.GetNumber() == 1);
}


