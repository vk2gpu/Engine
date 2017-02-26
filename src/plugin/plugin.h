#pragma once

#include "core/types.h"
#include "plugin/dll.h"

namespace Plugin
{
	/**
	 * Plugin export functions.
	 */
	typedef const char*(*GetPluginNameFn)();
	typedef const char*(*GetPluginDescFn)();
	typedef i32(*GetPluginVersionFn)();
	typedef class IPlugin*(*CreatePluginFn)();
	typedef void(*DestroyPluginFn)(class IPlugin*);

	/**
	 * Current plugin version.
	 */
	static const i32 PLUGIN_VERSION = 1;

	/**
	 * Plugin interface.
	 */
	class PLUGIN_DLL IPlugin
	{
	public:
		virtual ~IPlugin() {}
	};

} // namespace Plugin
