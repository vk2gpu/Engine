#include "graphics/material.h"
#include "graphics/shader.h"
#include "graphics/texture.h"
#include "graphics/private/material_impl.h"
#include "graphics/converters/import_material.h"

#include "resource/converter.h"
#include "resource/manager.h"
#include "core/array.h"
#include "core/debug.h"
#include "core/enum.h"
#include "core/file.h"
#include "core/hash.h"
#include "core/map.h"
#include "core/misc.h"
#include "core/vector.h"

#include "gpu/enum.h"
#include "gpu/resources.h"
#include "gpu/utils.h"

#include "serialization/serializer.h"

#include <cstring>
#include <utility>

namespace
{
	class ConverterMaterial : public Resource::IConverter
	{
	public:
		ConverterMaterial() {}

		virtual ~ConverterMaterial() {}

		bool SupportsFileType(const char* fileExt, const Core::UUID& type) const override
		{
			return (type == Graphics::Material::GetTypeUUID()) || (fileExt && (strcmp(fileExt, "material") == 0));
		}

		bool Convert(Resource::IConverterContext& context, const char* sourceFile, const char* destPath) override
		{
			auto metaData = context.GetMetaData<Graphics::MetaDataMaterial>();

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

			auto materialFile = Core::File(sourceFile, Core::FileFlags::READ, context.GetPathResolver());
			auto materialSer = Serialization::Serializer(materialFile, Serialization::Flags::TEXT);

			Graphics::ImportMaterial material;
			if(!material.Serialize(materialSer))
			{
				context.AddError(__FILE__, __LINE__, "ERROR: Failed to serialize material \"%s\"", sourceFile);
				return false;
			}

			Graphics::MaterialData materialData;

			context.AddResourceDependency(material.shader_.c_str(), Graphics::Shader::GetTypeUUID());
			materialData.shader_ = material.shader_.c_str();
			materialData.numTextures_ = material.textures_.size();

			Core::Vector<Graphics::MaterialTexture> matTexs;
			matTexs.reserve(materialData.numTextures_);
			for(auto& texParam : material.textures_)
			{
				Graphics::MaterialTexture matTex;
				strcpy_s(matTex.bindingName_.data(), matTex.bindingName_.size(), texParam.first.c_str());
				matTex.resourceName_ = texParam.second.c_str();
				matTexs.push_back(matTex);
			}

			Core::File outFile(outFilename, Core::FileFlags::CREATE | Core::FileFlags::WRITE);
			if(!outFile)
				return false;

			outFile.Write(&materialData, sizeof(materialData));
			if(matTexs.size() > 0)
				outFile.Write(matTexs.data(), matTexs.size() * sizeof(Graphics::MaterialTexture));

			context.AddOutput(outFilename);

			// Setup metadata.
			context.SetMetaData(metaData);

			return true;
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
			outPlugin->name_ = "Graphics.Material Converter";
			outPlugin->desc_ = "Material converter plugin.";
		}
		retVal = true;
	}

	// Fill in plugin specific.
	if(uuid == Resource::ConverterPlugin::GetUUID())
	{
		if(outPlugin)
		{
			auto* plugin = static_cast<Resource::ConverterPlugin*>(outPlugin);
			plugin->CreateConverter = []() -> Resource::IConverter* { return new ConverterMaterial(); };
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
