#include "plugin_test_basic.h"
#include "core/debug.h"
#include "core/allocator_overrides.h"

DECLARE_MODULE_ALLOCATOR("General/" MODULE_NAME);

namespace
{
	int number_ = 0;
}

extern "C" {
EXPORT bool GetPlugin(struct Plugin::Plugin* outPlugin, Core::UUID uuid)
{
	bool retVal = false;

	// Fill in base info.
	if(uuid == Plugin::Plugin::GetUUID() || uuid == PluginTestBasic::GetUUID())
	{
		if(outPlugin)
		{
			outPlugin->systemVersion_ = Plugin::PLUGIN_SYSTEM_VERSION;
			outPlugin->pluginVersion_ = PluginTestBasic::PLUGIN_VERSION;
			outPlugin->uuid_ = PluginTestBasic::GetUUID();
			outPlugin->name_ = "PluginTestBasic";
			outPlugin->desc_ = "Basic plugin test.";
		}
		retVal = true;
	}

	// Fill in plugin specific.
	if(uuid == PluginTestBasic::GetUUID())
	{
		if(outPlugin)
		{
			auto* plugin = static_cast<PluginTestBasic*>(outPlugin);
			plugin->successfullyLoaded_ = true;
			plugin->testMagic_ = PluginTestBasic::TEST_MAGIC;

			plugin->SetNumber = [](int num) { number_ = num; };

			plugin->GetNumber = []() -> int { return number_; };
		}

		retVal = true;
	}

	return retVal;
}
}
