#include "graphics/texture.h"
#include "resource/converter.h"
#include "core/debug.h"
#include "core/file.h"

#include <cstring>

namespace
{
	class ConverterTexture : public Resource::IConverter
	{
	public:
		ConverterTexture() {}

		virtual ~ConverterTexture() {}

		bool SupportsFileType(const char* fileExt, const Core::UUID& type) const override
		{
			return fileExt && (strcmp(fileExt, "png") == 0 || strcmp(fileExt, "jpg") == 0 || strcmp(fileExt, "dds") == 0);
		}

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
			outPlugin->name_ = "Graphics.Texture Converter";
			outPlugin->desc_ = "Texture converter plugin.";
		}
		retVal = true;
	}

	// Fill in plugin specific.
	if(uuid == Resource::ConverterPlugin::GetUUID())
	{
		if(outPlugin)
		{
			auto* plugin = static_cast<Resource::ConverterPlugin*>(outPlugin);
			plugin->CreateConverter = []() -> Resource::IConverter* { return new ConverterTexture(); };
			plugin->DestroyConverter = [](Resource::IConverter*& converter) {
				delete converter;
				converter = nullptr;
			};
		}
		retVal = true;
	}

	return retVal;
}
}
