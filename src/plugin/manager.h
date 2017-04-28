#pragma once

#include "core/types.h"
#include "plugin/dll.h"
#include "plugin/plugin.h"

namespace Plugin
{
	class PLUGIN_DLL Manager final
	{
	public:
		/**
		 * Initialize plugin manager.
		 */
		static void Initialize();

		/**
		 * Finalize plugin manager.
		 */
		static void Finalize();

		/**
		 * Is plugin manager initialized?
		 */
		static bool IsInitialized();

		/**
		 * Scan for plugins.
		 * @param path Path to scan for plugins.
		 * @return Number of plugins found.
		 */
		static i32 Scan(const char* path);

		/**
		 * Has plugin changed?
		 * @param plugin Plugin to check.
		 */
		static bool HasChanged(const Plugin& plugin);

		/**
		 * Reload plugin.
		 * @param inOutPlugin Plugin to reload.
		 */
		static bool Reload(Plugin& inOutPlugin);

		/**
		 * Get plugins for UUID.
		 * @param uuid Plugin UUID to search for.
		 * @param outPlugins Output array of plugin infos to fill.
		 * @param maxPlugins Maximum number of plugins to enumerate.
		 * @param Number of plugins enumerated.
		 */
		static i32 GetPlugins(Core::UUID uuid, Plugin* outPlugins, i32 maxPlugins);

		/**
		 * Templated version of GetPlugins. See above.
		 */
		template<typename TYPE>
		static i32 GetPlugins(TYPE* outPlugins, i32 maxPlugins)
		{
			return GetPlugins(TYPE::GetUUID(), reinterpret_cast<Plugin*>(outPlugins), maxPlugins);
		}

		/**
		 * Scoped manager init/fini.
		 * Mostly a convenience for unit tests.
		 */
		class Scoped
		{
		public:
			Scoped() { Initialize(); }
			~Scoped() { Finalize(); }
		};

	private:
		Manager() = delete;
		~Manager() = delete;
		Manager(const Manager&) = delete;
	};
} // namespace Plugin
