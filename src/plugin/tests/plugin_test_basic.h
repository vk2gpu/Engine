#pragma once

#include "plugin/plugin.h"

struct PluginTestBasic : Plugin::Plugin
{
	DECLARE_PLUGININFO(PluginTestBasic, 0);

	static const u32 TEST_MAGIC = 0x1e8c6a9b;

	bool successfullyLoaded_ = false;
	u32 testMagic_ = 0x00000000;

	typedef void (*SetNumberFn)(int);
	typedef int (*GetNumberFn)();

	SetNumberFn SetNumber = nullptr;
	GetNumberFn GetNumber = nullptr;
};
