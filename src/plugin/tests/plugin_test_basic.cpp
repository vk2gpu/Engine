#include "plugin/plugin.h"
#include "plugin/dll.h"

extern "C" 
{
	PLUGIN_DLL const char* GetPluginName()
	{
		return "plugin_test_basic";
	}

	PLUGIN_DLL const char* GetPluginDesc()
	{
		return "Basic test for plugins.";
	}

	PLUGIN_DLL i32 GetPluginVersion()
	{
		return Plugin::PLUGIN_VERSION;
	}

	PLUGIN_DLL Plugin::IPlugin* CreatePlugin()
	{
		return nullptr;
	}

	PLUGIN_DLL void DestroyPlugin(Plugin::IPlugin*)
	{

	}
}

