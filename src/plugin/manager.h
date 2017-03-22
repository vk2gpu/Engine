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
		 * Reload plugin.
		 * @param inOutPlugin Plugin to reload.
		 */
		bool Reload(Plugin& inOutPlugin);

		 /**
		 * Get plugins for UUID.
		 * @param uuid Plugin UUID to search for.
		 * @param outPlugins Output array of plugin infos to fill.
		 * @param maxPlugins Maximum number of plugins to enumerate.
		 * @param Number of plugins enumerated.
		 */
		i32 GetPlugins(Core::UUID uuid, Plugin** outPlugins, i32 maxPlugins);

		/**
		 * Templated version of GetPlugins. See above.
		 */
		template<typename TYPE>
		i32 GetPlugins(TYPE** outPlugins, i32 maxPlugins)
		{
			return GetPlugins(TYPE::GetUUID(), reinterpret_cast<Plugin**>(outPlugins), maxPlugins);
		}

	private:
		Manager(const Manager&) = delete;
		struct ManagerImpl* impl_ = nullptr;
	};
} // namespace Plugin

