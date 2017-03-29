#include "resource/converter.h"
#include "core/debug.h"
#include "core/file.h"

#include <cstring>

namespace
{
	class ConverterBasic : public Resource::IConverter
	{
	public:
		ConverterBasic() {}

		virtual ~ConverterBasic() {}

		bool SupportsFileType(const char* extension) const override { return strcmp(extension, "test") == 0; }

		bool Convert(Resource::IConverterContext& context, const char* sourceFile, const char* destPath) override
		{
			if(!Core::FileExists(sourceFile))
			{
				context.AddError(__FILE__, __LINE__, "ERROR: Source file does not exist.");
				return false;
			}

			char file[Core::MAX_PATH_LENGTH];
			memset(file, 0, sizeof(file));
			if(!Core::FileSplitPath(sourceFile, nullptr, 0, file, sizeof(file), nullptr, 0))
			{
				context.AddError(__FILE__, __LINE__, "INTERNAL ERROR: Core::FileSplitPath failed.");
				return false;
			}

			char outFilename[Core::MAX_PATH_LENGTH];
			memset(outFilename, 0, sizeof(outFilename));

			strcat_s(outFilename, sizeof(outFilename), destPath);
			Core::FileNormalizePath(outFilename, sizeof(outFilename), true);
			strcat_s(outFilename, sizeof(outFilename), "/");
			strcat_s(outFilename, sizeof(outFilename), file);
			strcat_s(outFilename, sizeof(outFilename), ".test.converted");
			Core::FileNormalizePath(outFilename, sizeof(outFilename), true);
			return Core::FileCopy(sourceFile, outFilename);
		}
	};
}


extern "C" {
EXPORT bool GetPlugin(struct Plugin::Plugin* outPlugin, Core::UUID uuid)
{
	bool retVal = false;

	// Fill in base info.
	if(uuid == Plugin::Plugin::GetUUID() || uuid == Resource::ConverterPlugin::GetUUID())
	{
		if(outPlugin)
		{
			outPlugin->systemVersion_ = Plugin::PLUGIN_SYSTEM_VERSION;
			outPlugin->pluginVersion_ = Resource::ConverterPlugin::PLUGIN_VERSION;
			outPlugin->uuid_ = Resource::ConverterPlugin::GetUUID();
			outPlugin->name_ = "ConverterTestPlugin";
			outPlugin->desc_ = "Converter test plugin.";
		}
		retVal = true;
	}

	// Fill in plugin specific.
	if(uuid == Resource::ConverterPlugin::GetUUID())
	{
		auto* plugin = static_cast<Resource::ConverterPlugin*>(outPlugin);
		plugin->CreateConverter = []() -> Resource::IConverter* { return new ConverterBasic(); };
		plugin->DestroyConverter = [](Resource::IConverter* converter) { delete converter; };

		retVal = true;
	}

	return retVal;
}
}
