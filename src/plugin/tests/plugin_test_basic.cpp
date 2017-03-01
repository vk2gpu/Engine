#include "plugin/plugin.h"
#include "plugin/dll.h"

extern "C" 
{
	__declspec(dllexport) const char* GetPluginName()
	{
		return "plugin_test_basic";
	}

	EXPORT const char* GetPluginDesc()
	{
		return "Basic test for plugins.";
	}

	EXPORT i32 GetPluginVersion()
	{
		return Plugin::PLUGIN_VERSION;
	}

	EXPORT Plugin::IPlugin* CreatePlugin()
	{
		return nullptr;
	}

	EXPORT void DestroyPlugin(Plugin::IPlugin*)
	{

	}
}

