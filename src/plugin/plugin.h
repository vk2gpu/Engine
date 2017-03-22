#pragma once

#include "core/types.h"
#include "plugin/dll.h"

#include "core/uuid.h"

namespace Plugin
{
	/**
	 * Get plugin info.
	 * @param outPlugin Pointer to plugin structure to fill in. Must be valid for given @a uuid.
	 * @param uuid UUID of plugin we wish to get info for.
	 * @return true if success, false if failure.
	 */
	typedef bool (*GetPluginFn)(struct Plugin* outPlugin, Core::UUID uuid);

	/**
	 * Current plugin system version.
	 */
	static const u32 PLUGIN_SYSTEM_VERSION = 0x00000001;


#define DECLARE_PLUGININFO(NAME, VERSION)                                                                              \
	static Core::UUID GetUUID() { return Core::UUID(#NAME); }                                                          \
	static const u32 PLUGIN_VERSION = VERSION;


	/**
	 * Plugin.
	 * This should always be the base struct of any user plugins.
	 */
	struct Plugin
	{
		DECLARE_PLUGININFO("Plugin", 0);

		/// Plugin system version.
		u32 systemVersion_ = 0x00000000;
		/// Plugin version.
		u32 pluginVersion_ = 0x00000000;
		/// Plugin UUID.
		Core::UUID uuid_;
		/// Plugin name.
		const char* name_ = nullptr;
		/// Plugin description.
		const char* desc_ = nullptr;
		/// File name.
		const char* fileName_ = nullptr;
		/// File UUID.
		Core::UUID fileUuid_;
	};

} // namespace Plugin
