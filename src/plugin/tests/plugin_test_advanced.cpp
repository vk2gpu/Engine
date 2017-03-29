#define PLUGIN_PRIVATE public
#include "plugin_test_advanced.h"
#include "core/debug.h"

static PluginTestAdvancedVtbl vtbl_;

struct PluginTestAdvancedImpl
{
	int number_ = 0;

};

extern "C" 
{
	EXPORT bool GetPlugin(struct Plugin::Plugin* outPlugin, Core::UUID uuid)
	{
		bool retVal = false;

		// TODO: Proper entrypoint.
		if(vtbl_.SetNumber_ == nullptr)
		{
			vtbl_.SetNumber_ = [](PluginTestAdvanced* _this, int num)
			{
				_this->impl_->number_ = num;
			};

			vtbl_.GetNumber_ = [](const PluginTestAdvanced* _this)
			{
				return _this->impl_->number_;
			};
		}

		// Fill in base info.
		if(uuid == Plugin::Plugin::GetUUID() || uuid == PluginTestAdvanced::GetUUID())
		{
			if(outPlugin)
			{
				outPlugin->systemVersion_ = Plugin::PLUGIN_SYSTEM_VERSION;
				outPlugin->pluginVersion_ = PluginTestAdvanced::PLUGIN_VERSION;
				outPlugin->uuid_ = PluginTestAdvanced::GetUUID();
				outPlugin->name_ = "PluginTestAdvanced";
				outPlugin->desc_ = "Advanced plugin test.";
			}
			retVal = true;
		}

		// Fill in plugin specific.
		if(uuid == PluginTestAdvanced::GetUUID())
		{
			auto* plugin = static_cast<PluginTestAdvanced*>(outPlugin);

			plugin->vtbl_ = &vtbl_;

			if(!plugin->impl_)
			{
				plugin->impl_ = new PluginTestAdvancedImpl;
			}
			
			retVal = true;
		}

		return retVal;
	}

}

