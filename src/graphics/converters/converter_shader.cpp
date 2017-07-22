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
#include "graphics/converters/shader_backend_hlsl.h"
#include "graphics/converters/shader_backend_metadata.h"
#include "graphics/converters/shader_compiler_hlsl.h"
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

			char fullPath[Core::MAX_PATH_LENGTH];
			memset(fullPath, 0, sizeof(fullPath));
			context.GetPathResolver()->ResolvePath(sourceFile, fullPath, sizeof(fullPath));

			char file[Core::MAX_PATH_LENGTH];
			memset(file, 0, sizeof(file));
			char path[Core::MAX_PATH_LENGTH];
			memset(path, 0, sizeof(path));
			if(!Core::FileSplitPath(fullPath, path, sizeof(path), file, sizeof(file), nullptr, 0))
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

				Graphics::ShaderPreprocessor preprocessor;

				// Setup include path to root of shader.
				preprocessor.AddInclude(path);

				if(!preprocessor.Preprocess(sourceFile, shaderSource.data()))
					return false;

				// Add dependencies from preprocessor stage.
				for(const char* dep : preprocessor.GetDependencies())
				{
					context.AddDependency(dep);
				}

				// Parse shader into an AST.
				Graphics::ShaderParser shaderParser;
				auto node = shaderParser.Parse(sourceFile, preprocessor.GetOutput().c_str());
				if(node == nullptr)
					return false;

				// Parse shader metadata from AST to determine what needs to be compiled.
				Graphics::ShaderBackendMetadata backendMetadata;
				node->Visit(&backendMetadata);

				const auto& samplerStates = backendMetadata.GetSamplerStates();
				const auto& techniques = backendMetadata.GetTechniques();
				Core::Set<Core::String> vs;
				Core::Set<Core::String> gs;
				Core::Set<Core::String> hs;
				Core::Set<Core::String> ds;
				Core::Set<Core::String> ps;
				Core::Set<Core::String> cs;

				for(const auto& technique : techniques)
				{
					if(technique.vs_.size() > 0)
						vs.insert(technique.vs_);
					if(technique.gs_.size() > 0)
						gs.insert(technique.gs_);
					if(technique.hs_.size() > 0)
						hs.insert(technique.hs_);
					if(technique.ds_.size() > 0)
						ds.insert(technique.ds_);
					if(technique.ps_.size() > 0)
						ps.insert(technique.ps_);
					if(technique.cs_.size() > 0)
						cs.insert(technique.cs_);
				}

				Core::Log("To build:\n");
				Core::Log(" - VS: %u\n", vs.size());
				Core::Log(" - GS: %u\n", gs.size());
				Core::Log(" - HS: %u\n", hs.size());
				Core::Log(" - DS: %u\n", ds.size());
				Core::Log(" - PS: %u\n", ps.size());
				Core::Log(" - CS: %u\n", cs.size());
				Core::Log(" - Sampler states: %u\n", samplerStates.size());
				Core::Log(" - Techniques:\n");
				for(const auto& technique : techniques)
				{
					Core::Log(" - - %s\n", technique.name_.c_str());
				}

				// Generate HLSL for the whole ESF.
				Graphics::ShaderBackendHLSL backendHLSL;
				node->Visit(&backendHLSL);

				// Compile HLSL.
				Graphics::ShaderCompilerHLSL compilerHLSL;

				struct CompileInfo
				{
					CompileInfo(const char* name, const char* code, const char* entryPoint, const char* target)
					    : name_(name)
					    , code_(code)
					    , entryPoint_(entryPoint)
					    , target_(target)
					{
					}

					const char* name_;
					const char* code_;
					const char* entryPoint_;
					const char* target_;
				};

				Core::Vector<CompileInfo> compiles;
				for(const auto& entry : vs)
					compiles.emplace_back(sourceFile, backendHLSL.GetOutputCode().c_str(), entry.c_str(), "vs_5_0");
				for(const auto& entry : gs)
					compiles.emplace_back(sourceFile, backendHLSL.GetOutputCode().c_str(), entry.c_str(), "gs_5_0");
				for(const auto& entry : hs)
					compiles.emplace_back(sourceFile, backendHLSL.GetOutputCode().c_str(), entry.c_str(), "hs_5_0");
				for(const auto& entry : ds)
					compiles.emplace_back(sourceFile, backendHLSL.GetOutputCode().c_str(), entry.c_str(), "ds_5_0");
				for(const auto& entry : ps)
					compiles.emplace_back(sourceFile, backendHLSL.GetOutputCode().c_str(), entry.c_str(), "ps_5_0");
				for(const auto& entry : cs)
					compiles.emplace_back(sourceFile, backendHLSL.GetOutputCode().c_str(), entry.c_str(), "cs_5_0");


				Core::Vector<Graphics::ShaderCompileOutput> outputCompiles;
				for(const auto& compile : compiles)
				{
					outputCompiles.emplace_back(
					    compilerHLSL.Compile(compile.name_, compile.code_, compile.entryPoint_, compile.target_));
				}

				int a = 0;
				++a;
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
