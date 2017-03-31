#include "client/window.h"

#include "catch.hpp"

#include "core/debug.h"
#include "core/file.h"
#include "core/os.h"
#include "plugin/manager.h"


// Plugins.
#include "plugin_test_basic.h"
#include "plugin_test_advanced.h"

namespace
{
}

TEST_CASE("plugin-tests-scan")
{
	Plugin::Manager manager;

	char path[Core::MAX_PATH_LENGTH];
	Core::FileGetCurrDir(path, Core::MAX_PATH_LENGTH);
	i32 found = manager.Scan(".");
	REQUIRE(found > 0);
}

TEST_CASE("plugin-tests-basic-plugin")
{
	Plugin::Manager manager;

	char path[Core::MAX_PATH_LENGTH];
	Core::FileGetCurrDir(path, Core::MAX_PATH_LENGTH);
	i32 found = manager.Scan(".");
	REQUIRE(found > 0);

	PluginTestBasic plugin;

	found = manager.GetPlugins(&plugin, 1);
	REQUIRE(found > 0);
	REQUIRE(plugin.successfullyLoaded_ == true);
	REQUIRE(plugin.testMagic_ == PluginTestBasic::TEST_MAGIC);
}

TEST_CASE("plugin-tests-advanced-plugin")
{
	Plugin::Manager manager;

	char path[Core::MAX_PATH_LENGTH];
	Core::FileGetCurrDir(path, Core::MAX_PATH_LENGTH);
	i32 found = manager.Scan(".");
	REQUIRE(found > 0);

	PluginTestAdvanced plugin;

	found = manager.GetPlugins(&plugin, 1);
	REQUIRE(found > 0);
	
	REQUIRE(plugin.GetNumber() == 0);
	plugin.SetNumber(1);
	REQUIRE(plugin.GetNumber() == 1);
}

TEST_CASE("plugin-tests-basic-reload")
{
	Plugin::Manager manager;

	char path[Core::MAX_PATH_LENGTH];
	Core::FileGetCurrDir(path, Core::MAX_PATH_LENGTH);
	i32 found = manager.Scan(".");
	REQUIRE(found > 0);

	PluginTestBasic plugin;

	found = manager.GetPlugins(&plugin, 1);
	REQUIRE(found > 0);
	REQUIRE(plugin.successfullyLoaded_ == true);
	REQUIRE(plugin.testMagic_ == PluginTestBasic::TEST_MAGIC);

	// Test initial state.
	REQUIRE(plugin.GetNumber() == 0);
	plugin.SetNumber(1);
	REQUIRE(plugin.GetNumber() == 1);

#if 0 
	while(manager.HasChanged(plugin) == false)
	{
		DBG_LOG("Waiting for recompilation...");
		::Sleep(5000);
	}
#endif

	bool reloaded = manager.Reload(plugin);
	REQUIRE(reloaded);

	// Test reloading has reset state.
	REQUIRE(plugin.GetNumber() == 0);
	plugin.SetNumber(1);
	REQUIRE(plugin.GetNumber() == 1);
}
