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

			char destDir[Core::MAX_PATH_LENGTH];
			memset(destDir, 0, sizeof(destDir));
			if(!Core::FileSplitPath(destPath, destDir, sizeof(destDir), nullptr, 0, nullptr, 0))
			{
				context.AddError(__FILE__, __LINE__, "INTERNAL ERROR: Core::FileSplitPath failed.");
				return false;
			}

			Core::FileCreateDir(destDir);

			char outFilename[Core::MAX_PATH_LENGTH];
			memset(outFilename, 0, sizeof(outFilename));
			strcat_s(outFilename, sizeof(outFilename), destPath);
			Core::FileNormalizePath(outFilename, sizeof(outFilename), true);

			bool retVal = false;

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
				Core::Array<Core::Set<Core::String>, (i32)GPU::ShaderType::MAX> shaders;

				for(const auto& technique : techniques)
				{
					if(technique.vs_.size() > 0)
						shaders[(i32)GPU::ShaderType::VS].insert(technique.vs_);
					if(technique.gs_.size() > 0)
						shaders[(i32)GPU::ShaderType::GS].insert(technique.gs_);
					if(technique.hs_.size() > 0)
						shaders[(i32)GPU::ShaderType::HS].insert(technique.hs_);
					if(technique.ds_.size() > 0)
						shaders[(i32)GPU::ShaderType::DS].insert(technique.ds_);
					if(technique.ps_.size() > 0)
						shaders[(i32)GPU::ShaderType::PS].insert(technique.ps_);
					if(technique.cs_.size() > 0)
						shaders[(i32)GPU::ShaderType::CS].insert(technique.cs_);
				}

				// Grab sampler states.
				//const auto& samplerStates = backendMetadata.GetSamplerStates();

				// Generate HLSL for the whole ESF.
				Graphics::ShaderBackendHLSL backendHLSL;
				node->Visit(&backendHLSL);

				// Compile HLSL.
				Graphics::ShaderCompilerHLSL compilerHLSL;

				struct CompileInfo
				{
					CompileInfo(const char* name, const char* code, const char* entryPoint, GPU::ShaderType type)
					    : name_(name)
					    , code_(code)
					    , entryPoint_(entryPoint)
					    , type_(type)
					{
					}

					const char* name_;
					const char* code_;
					const char* entryPoint_;
					GPU::ShaderType type_;
				};

				Core::Vector<CompileInfo> compiles;
				for(i32 idx = 0; idx < shaders.size(); ++idx)
					for(const auto& shader : shaders[idx])
						compiles.emplace_back(
						    sourceFile, backendHLSL.GetOutputCode().c_str(), shader.c_str(), (GPU::ShaderType)idx);

				Core::Vector<Graphics::ShaderCompileOutput> outputCompiles;
				for(const auto& compile : compiles)
				{
					auto outCompile =
					    compilerHLSL.Compile(compile.name_, compile.code_, compile.entryPoint_, compile.type_);
					if(outCompile)
					{
						outputCompiles.emplace_back(outCompile);
					}
					else
					{
						Core::String errStr(outCompile.errorsBegin_, outCompile.errorsEnd_);
						Core::Log("%s", errStr.c_str());
						return false;
					}
				}

				// Build set of all bindings used.
				using BindingMap = Core::Map<Core::String, i32>;
				BindingMap cbuffers;
				BindingMap samplers;
				BindingMap srvs;
				BindingMap uavs;
				i32 bindingIdx = 0;

				for(const auto& compile : outputCompiles)
				{
					const auto AddBindings = [&](
					    const Core::Vector<Graphics::ShaderBinding>& inBindings, BindingMap& outBindings) {
						for(const auto& binding : inBindings)
						{
							if(outBindings.find(binding.name_) == outBindings.end())
							{
								outBindings.insert(binding.name_, bindingIdx++);
							}
						}
					};

					AddBindings(compile.cbuffers_, cbuffers);
					AddBindings(compile.samplers_, samplers);
					AddBindings(compile.srvs_, srvs);
					AddBindings(compile.uavs_, uavs);
				}

				// Setup data ready to serialize.
				Graphics::ShaderHeader outHeader;
				outHeader.numCBuffers_ = cbuffers.size();
				outHeader.numSamplers_ = samplers.size();
				outHeader.numSRVs_ = srvs.size();
				outHeader.numUAVs_ = uavs.size();
				outHeader.numShaders_ = outputCompiles.size();
				outHeader.numTechniques_ = techniques.size();
				Core::Vector<Graphics::ShaderBindingHeader> outBindingHeaders;
				outBindingHeaders.reserve(cbuffers.size() + samplers.size() + srvs.size() + uavs.size());

				const auto PopulateoutBindingHeaders = [&outBindingHeaders](const BindingMap& bindings) {
					for(const auto& binding : bindings)
					{
						Graphics::ShaderBindingHeader bindingHeader;
						memset(&bindingHeader, 0, sizeof(bindingHeader));
						strcpy_s(bindingHeader.name_, sizeof(bindingHeader.name_), binding.first.c_str());
						outBindingHeaders.push_back(bindingHeader);
					}
				};

				PopulateoutBindingHeaders(cbuffers);
				PopulateoutBindingHeaders(samplers);
				PopulateoutBindingHeaders(srvs);
				PopulateoutBindingHeaders(uavs);
				Core::Vector<Graphics::ShaderBytecodeHeader> outBytecodeHeaders;
				Core::Vector<Graphics::ShaderBindingMapping> outBindingMappings;
				i32 bytecodeOffset = 0;
				for(const auto& compile : outputCompiles)
				{
					Graphics::ShaderBytecodeHeader bytecodeHeader;
					bytecodeHeader.numCBuffers_ = compile.cbuffers_.size();
					bytecodeHeader.numSamplers_ = compile.samplers_.size();
					bytecodeHeader.numSRVs_ = compile.srvs_.size();
					bytecodeHeader.numUAVs_ = compile.uavs_.size();
					bytecodeHeader.type_ = compile.type_;
					bytecodeHeader.offset_ = bytecodeOffset;
					bytecodeHeader.numBytes_ = (i32)(compile.byteCodeEnd_ - compile.byteCodeBegin_);

					bytecodeOffset += bytecodeHeader.numBytes_;
					outBytecodeHeaders.push_back(bytecodeHeader);

					const auto AddBindingMapping = [&](
					    const BindingMap& bindingMap, const Core::Vector<Graphics::ShaderBinding>& bindings) {
						for(const auto& binding : bindings)
						{
							auto it = bindingMap.find(binding.name_);
							DBG_ASSERT(it != bindingMap.end());
							Graphics::ShaderBindingMapping mapping;
							mapping.binding_ = it->second;
							mapping.dstSlot_ = binding.slot_;
							outBindingMappings.push_back(mapping);
						}
					};

					AddBindingMapping(cbuffers, compile.cbuffers_);
					AddBindingMapping(samplers, compile.samplers_);
					AddBindingMapping(srvs, compile.srvs_);
					AddBindingMapping(uavs, compile.uavs_);
				};

				Core::Vector<Graphics::ShaderTechniqueHeader> outTechniqueHeaders;
				for(const auto& technique : backendMetadata.GetTechniques())
				{
					Graphics::ShaderTechniqueHeader techniqueHeader;
					memset(&techniqueHeader, 0, sizeof(techniqueHeader));
					strcpy_s(techniqueHeader.name_, sizeof(techniqueHeader.name_), technique.name_.c_str());

					auto FindShaderIdx = [&](const char* name) -> i32 {
						if(!name)
							return -1;
						i32 idx = 0;
						for(const auto& compile : compiles)
						{
							if(strcmp(compile.entryPoint_, name) == 0)
								return idx;
							++idx;
						}
						return -1;
					};
					techniqueHeader.vs_ = FindShaderIdx(technique.vs_.c_str());
					techniqueHeader.gs_ = FindShaderIdx(technique.gs_.c_str());
					techniqueHeader.hs_ = FindShaderIdx(technique.hs_.c_str());
					techniqueHeader.ds_ = FindShaderIdx(technique.ds_.c_str());
					techniqueHeader.ps_ = FindShaderIdx(technique.ps_.c_str());
					techniqueHeader.cs_ = FindShaderIdx(technique.cs_.c_str());
					techniqueHeader.rs_ = technique.rs_.state_;
					outTechniqueHeaders.push_back(techniqueHeader);
				}

				auto WriteShader = [&](const char* outFilename) {
					// Write out shader data.
					Core::File outFile(outFilename, Core::FileFlags::CREATE | Core::FileFlags::WRITE);
					if(outFile)
					{
						outFile.Write(&outHeader, sizeof(outHeader));
						if(outBindingHeaders.size() > 0)
							outFile.Write(outBindingHeaders.data(),
							    outBindingHeaders.size() * sizeof(Graphics::ShaderBindingHeader));
						if(outBytecodeHeaders.size() > 0)
							outFile.Write(outBytecodeHeaders.data(),
							    outBytecodeHeaders.size() * sizeof(Graphics::ShaderBytecodeHeader));
						if(outBindingMappings.size() > 0)
							outFile.Write(outBindingMappings.data(),
							    outBindingMappings.size() * sizeof(Graphics::ShaderBindingMapping));
						if(outTechniqueHeaders.size() > 0)
							outFile.Write(outTechniqueHeaders.data(),
							    outTechniqueHeaders.size() * sizeof(Graphics::ShaderTechniqueHeader));

						i64 outBytes = 0;
						for(const auto& compile : outputCompiles)
						{
							outBytes +=
							    outFile.Write(compile.byteCodeBegin_, compile.byteCodeEnd_ - compile.byteCodeBegin_);
						}

						return true;
					}
					return false;
				};

				retVal = WriteShader(outFilename);
			}
			context.AddDependency(sourceFile);

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
