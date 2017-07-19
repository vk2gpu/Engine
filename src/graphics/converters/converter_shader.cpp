#include "graphics/shader.h"
#include "resource/converter.h"
#include "core/array.h"
#include "core/debug.h"
#include "core/enum.h"
#include "core/file.h"
#include "core/hash.h"
#include "core/linear_allocator.h"
#include "core/misc.h"
#include "core/string.h"
#include "core/vector.h"

#include "gpu/enum.h"
#include "gpu/resources.h"
#include "gpu/utils.h"

#include "serialization/serializer.h"
#include "graphics/converters/shader_parser.h"
#include "graphics/converters/shader_preprocessor.h"

#include <cstring>

namespace
{
	class ConverterShader : public Resource::IConverter
	{
	public:
		ConverterShader() {}

		virtual ~ConverterShader() {}

		struct MetaData
		{
			bool isInitialized_ = false;

			bool Serialize(Serialization::Serializer& serializer)
			{
				isInitialized_ = true;

				bool retVal = true;
				//retVal &= serializer.Serialize("format", format_);
				//retVal &= serializer.Serialize("generateMipLevels", generateMipLevels_);
				return retVal;
			}
		};

		bool SupportsFileType(const char* fileExt, const Core::UUID& type) const override
		{
			return (type == Graphics::Shader::GetTypeUUID()) || (fileExt && strcmp(fileExt, "esf") == 0);
		}

		bool Convert(Resource::IConverterContext& context, const char* sourceFile, const char* destPath) override
		{
			MetaData metaData = context.GetMetaData<MetaData>();

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


			//
			Core::File shaderFile(sourceFile, Core::FileFlags::READ, context.GetPathResolver());
			if(shaderFile)
			{
				Core::Vector<char> shaderSource;
				shaderSource.resize((i32)shaderFile.Size() + 1, '\0');
				shaderFile.Read(shaderSource.data(), shaderFile.Size());

				Graphics::ShaderPreprocessor preprocessor(sourceFile, shaderSource.data());

				// Add defines.
				//for( auto Define : Params.Permutation_.Defines_ )
				//{
				//	Preprocessor.addDefine( Define.first.c_str(), Define.second.c_str() );
				//}
				//auto ShaderDefine = ShaderDefines[ (int)Entry.Type_ ];
				//Preprocessor.addDefine( ShaderDefine, "1" );

				// Add entry point macro.
				//if( Entrypoint != "main" )
				//{
				//	Preprocessor.addDefine( Entrypoint.c_str(), "main" );
				//}

				// Setup include paths.
				//preprocessor.addInclude( "." );
				//preprocessor.addInclude( "./Content/Engine" );
				//preprocessor.addInclude( "../Psybrus/Dist/Content/Engine" );
				//preprocessor.addInclude( "./Intermediate/GeneratedShaderData" );

				if(preprocessor.Preprocess())
				{
					Graphics::ShaderParser shaderParser;
					shaderParser.Parse(sourceFile, preprocessor.GetOutput().c_str());

					// TODO: Parse.
				}
				Core::String processedSourceData = preprocessor.GetOutput();
			}
			context.AddDependency(sourceFile);

			bool retVal = false;

			if(retVal)
			{
				context.AddOutput(outFilename);
			}

			// Setup metadata.
			context.SetMetaData(metaData);

			return retVal;
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
			outPlugin->name_ = "Graphics.Shader Converter";
			outPlugin->desc_ = "Shader converter plugin.";
		}
		retVal = true;
	}

	// Fill in plugin specific.
	if(uuid == Resource::ConverterPlugin::GetUUID())
	{
		if(outPlugin)
		{
			auto* plugin = static_cast<Resource::ConverterPlugin*>(outPlugin);
			plugin->CreateConverter = []() -> Resource::IConverter* { return new ConverterShader(); };
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
