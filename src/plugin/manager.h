#pragma once

#include "core/types.h"
#include "plugin/dll.h"
#include "plugin/plugin.h"

namespace Plugin
{
	class PLUGIN_DLL Manager final
	{
	public:
		Manager();
		~Manager();

		/**
		 * Scan for plugins.
		 * @param path Path to scan for plugins.
		 * @return Number of plugins found.
		 */
		i32 Scan(const char* path);

		/**
		 * Get plugin.
		 * @param name Name of plugin.
		 */
		IPlugin* GetPlugin(const char* name);

	private:
		Manager(const Manager&) = delete;
		struct ManagerImpl* impl_ = nullptr;
	};
} // namespace Plugin

