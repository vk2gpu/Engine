#pragma once

#include "plugin/plugin.h"
#include "plugin/manager.h"

struct PluginTestAdvancedVtbl
{
	typedef void (*SetNumberFn)(class PluginTestAdvanced*, int);
	typedef int (*GetNumberFn)(const class PluginTestAdvanced*);
	SetNumberFn SetNumber_ = nullptr;
	GetNumberFn GetNumber_ = nullptr;
};

class PluginTestAdvanced : Plugin::Plugin
{
public:
	DECLARE_PLUGININFO(PluginTestAdvanced, 0);

	void SetNumber(int num) { vtbl_->SetNumber_(this, num); }
	int GetNumber() const { return vtbl_->GetNumber_(this); }

	PLUGIN_PRIVATE : PluginTestAdvancedVtbl* vtbl_ = nullptr;
	struct PluginTestAdvancedImpl* impl_ = nullptr;
};
