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
#include "graphics/private/shader_impl.h"
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

				// Gather all unique shaders referenced by techniques.
				const auto& techniques = backendMetadata.GetTechniques();
				Core::Map<Core::String, Core::Set<Core::String>> shaders;

				for(const auto& technique : techniques)
				{
					if(technique.vs_.size() > 0)
						shaders["vs_5_0"].insert(technique.vs_);
					if(technique.gs_.size() > 0)
						shaders["gs_5_0"].insert(technique.gs_);
					if(technique.hs_.size() > 0)
						shaders["hs_5_0"].insert(technique.hs_);
					if(technique.ds_.size() > 0)
						shaders["ds_5_0"].insert(technique.ds_);
					if(technique.ps_.size() > 0)
						shaders["ps_5_0"].insert(technique.ps_);
					if(technique.cs_.size() > 0)
						shaders["cs_5_0"].insert(technique.cs_);
				}

				// Grab sampler states.
				const auto& samplerStates = backendMetadata.GetSamplerStates();

				Core::Log("To build:\n");
				Core::Log(" - VS: %u\n", shaders["vs_5_0"].size());
				Core::Log(" - GS: %u\n", shaders["gs_5_0"].size());
				Core::Log(" - HS: %u\n", shaders["hs_5_0"].size());
				Core::Log(" - DS: %u\n", shaders["ds_5_0"].size());
				Core::Log(" - PS: %u\n", shaders["ps_5_0"].size());
				Core::Log(" - CS: %u\n", shaders["cs_5_0"].size());
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
				for(const auto& entry : shaders)
					for(const auto& shader : entry.second)
						compiles.emplace_back(
						    sourceFile, backendHLSL.GetOutputCode().c_str(), shader.c_str(), entry.first.c_str());

				Core::Vector<Graphics::ShaderCompileOutput> outputCompiles;
				for(const auto& compile : compiles)
				{
					outputCompiles.emplace_back(
					    compilerHLSL.Compile(compile.name_, compile.code_, compile.entryPoint_, compile.target_));
				}

				// Build set of all bindings used.
				Core::Set<Core::String> cbuffers;
				Core::Set<Core::String> samplers;
				Core::Set<Core::String> srvs;
				Core::Set<Core::String> uavs;

				for(const auto& compile : outputCompiles)
				{
					for(const auto& binding : compile.cbuffers_)
						cbuffers.insert(binding.name_);
					for(const auto& binding : compile.samplers_)
						samplers.insert(binding.name_);
					for(const auto& binding : compile.srvs_)
						srvs.insert(binding.name_);
					for(const auto& binding : compile.uavs_)
						uavs.insert(binding.name_);
				}

				// Setup shader header.
				Graphics::ShaderHeader outHeader;
				outHeader.numCBuffers_ = cbuffers.size();
				outHeader.numSamplers_ = samplers.size();
				outHeader.numSRVs_ = srvs.size();
				outHeader.numUAVs_ = uavs.size();
				outHeader.numShaders_ = outputCompiles.size();
				outHeader.numTechniques_ = techniques.size();

				// Calculate required memory for shader metadata.
				i32 outSize = 0;
				outSize += sizeof(Graphics::ShaderHeader);
				outSize += sizeof(Graphics::ShaderBindingHeader) * cbuffers.size();
				outSize += sizeof(Graphics::ShaderBindingHeader) * samplers.size();
				outSize += sizeof(Graphics::ShaderBindingHeader) * srvs.size();
				outSize += sizeof(Graphics::ShaderBindingHeader) * uavs.size();
				outSize += sizeof(Graphics::ShaderBytecodeHeader) * outputCompiles.size();
				outSize += sizeof(Graphics::ShaderTechniqueHeader) * techniques.size();

				for(const auto& compile : outputCompiles)
				{
					outSize += sizeof(Graphics::ShaderBindingMapping) * compile.cbuffers_.size();
					outSize += sizeof(Graphics::ShaderBindingMapping) * compile.samplers_.size();
					outSize += sizeof(Graphics::ShaderBindingMapping) * compile.srvs_.size();
					outSize += sizeof(Graphics::ShaderBindingMapping) * compile.uavs_.size();

					//outSize += (i32)(compile.strippedByteCodeEnd_ - compile.strippedByteCodeBegin_);
					outSize += (i32)(compile.byteCodeEnd_ - compile.byteCodeBegin_);
				}

				DBG_BREAK;
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
