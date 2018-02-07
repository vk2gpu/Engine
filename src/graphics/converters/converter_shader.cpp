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
#include "graphics/converters/import_shader.h"
#include "graphics/converters/shader_backend_hlsl.h"
#include "graphics/converters/shader_backend_metadata.h"
#include "graphics/converters/shader_compiler_hlsl.h"
#include "graphics/converters/shader_parser.h"
#include "graphics/converters/shader_preprocessor.h"

#include <cstring>

#define DEBUG_DUMP_SHADERS 1

#define DUMP_ESF_PATH "tmp.esf"
#define DUMP_HLSL_PATH "shader_dump\\%s-%s.hlsl"

namespace
{
	class ConverterShader : public Resource::IConverter
	{
	public:
		ConverterShader() {}

		virtual ~ConverterShader() {}

		bool SupportsFileType(const char* fileExt, const Core::UUID& type) const override
		{
			return (type == Graphics::Shader::GetTypeUUID()) || (fileExt && strcmp(fileExt, "esf") == 0);
		}

		bool Convert(Resource::IConverterContext& context, const char* sourceFile, const char* destPath) override
		{
			auto metaData = context.GetMetaData<Graphics::MetaDataShader>();
			auto* pathResolver = context.GetPathResolver();
			char fullPath[Core::MAX_PATH_LENGTH];
			memset(fullPath, 0, sizeof(fullPath));
			pathResolver->ResolvePath(sourceFile, fullPath, sizeof(fullPath));

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

			bool retVal = false;

			//
			Core::File shaderFile(sourceFile, Core::FileFlags::DEFAULT_READ, pathResolver);
			if(shaderFile)
			{
				Core::Vector<char> shaderSource;
				shaderSource.resize((i32)shaderFile.Size() + 1, '\0');
				shaderFile.Read(shaderSource.data(), shaderFile.Size());

				Graphics::ShaderPreprocessor preprocessor;

				// Setup include path to root of shader.
				preprocessor.AddInclude(path);

				if(!preprocessor.Preprocess(fullPath, shaderSource.data()))
					return false;

#if DEBUG_DUMP_SHADERS
				if(Core::FileExists(DUMP_ESF_PATH))
					Core::FileRemove(DUMP_ESF_PATH);
				if(auto outTmpFile = Core::File(DUMP_ESF_PATH, Core::FileFlags::DEFAULT_WRITE))
				{
					outTmpFile.Write(preprocessor.GetOutput().c_str(), preprocessor.GetOutput().size());
				}
#endif

				// Add dependencies from preprocessor stage.
				Core::Array<char, Core::MAX_PATH_LENGTH> originalPath;
				for(const char* dep : preprocessor.GetDependencies())
				{
					if(pathResolver->OriginalPath(dep, originalPath.data(), originalPath.size()))
						context.AddDependency(originalPath.data());
					else if(Core::FileExists(dep))
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

				// Get all technique entrypoints.
				Graphics::FunctionExports functionExports;
				for(const auto& tech : backendMetadata.GetTechniques())
				{
					if(tech.vs_.size() > 0)
						functionExports.push_back(tech.vs_);
					if(tech.hs_.size() > 0)
						functionExports.push_back(tech.hs_);
					if(tech.ds_.size() > 0)
						functionExports.push_back(tech.ds_);
					if(tech.hs_.size() > 0)
						functionExports.push_back(tech.hs_);
					if(tech.ps_.size() > 0)
						functionExports.push_back(tech.ps_);
					if(tech.cs_.size() > 0)
						functionExports.push_back(tech.cs_);
				}

				// Gather all unique shaders referenced by techniques.
				const auto& techniques = backendMetadata.GetTechniques();
				Core::Array<Core::Set<Core::String>, (i32)GPU::ShaderType::MAX> shaders;

				for(const auto& technique : techniques)
				{
					if(technique.vs_.size() > 0)
						shaders[(i32)GPU::ShaderType::VS].insert(technique.vs_);
					if(technique.hs_.size() > 0)
						shaders[(i32)GPU::ShaderType::HS].insert(technique.hs_);
					if(technique.ds_.size() > 0)
						shaders[(i32)GPU::ShaderType::DS].insert(technique.ds_);
					if(technique.gs_.size() > 0)
						shaders[(i32)GPU::ShaderType::GS].insert(technique.gs_);
					if(technique.ps_.size() > 0)
						shaders[(i32)GPU::ShaderType::PS].insert(technique.ps_);
					if(technique.cs_.size() > 0)
						shaders[(i32)GPU::ShaderType::CS].insert(technique.cs_);
				}

				// Grab sampler states.
				const auto& samplerStates = backendMetadata.GetSamplerStates();

				struct CompileInfo
				{
					CompileInfo(const Core::String& name, const Core::String& code, const Core::String& entryPoint,
					    GPU::ShaderType type, const Core::String& target)
					    : name_(name)
					    , code_(code)
					    , entryPoint_(entryPoint)
					    , type_(type)
					    , target_(target)
					{
					}

					Core::String name_;
					Core::String code_;
					Core::String entryPoint_;
					GPU::ShaderType type_;
					Core::String target_;
				};

				Graphics::ShaderCompilerHLSL compilerHLSL;
				auto GenerateAndCompile = [&](const Graphics::BindingMap& bindingMap,
				    Core::Vector<CompileInfo>& outCompileInfo,
				    Core::Vector<Graphics::ShaderCompileOutput>& outCompileOutput) {
					outCompileInfo.clear();
					outCompileOutput.clear();

					// Generate HLSL for the whole ESF.
					Graphics::ShaderBackendHLSL backendHLSL(bindingMap, functionExports, true);
					node->Visit(&backendHLSL);

#if DEBUG_DUMP_SHADERS
					Core::String hlslFileName;

					hlslFileName.Printf(DUMP_HLSL_PATH, file, "all");
					if(Core::FileExists(hlslFileName.c_str()))
						Core::FileRemove(hlslFileName.c_str());
					if(auto outTmpFile = Core::File(hlslFileName.c_str(), Core::FileFlags::DEFAULT_WRITE))
					{
						outTmpFile.Write(backendHLSL.GetOutputCode().c_str(), backendHLSL.GetOutputCode().size());
					}
#endif

					auto GetTarget = [](GPU::ShaderType type, i32 major, i32 minor) {
						DBG_ASSERT(major >= 4 && major <= 5);
						DBG_ASSERT(minor >= 0 && minor <= 1);
						switch(type)
						{
						case GPU::ShaderType::VS:
							return Core::String().Printf("vs_%i_%i", major, minor);
						case GPU::ShaderType::HS:
							return Core::String().Printf("hs_%i_%i", major, minor);
						case GPU::ShaderType::DS:
							return Core::String().Printf("ds_%i_%i", major, minor);
						case GPU::ShaderType::GS:
							return Core::String().Printf("gs_%i_%i", major, minor);
						case GPU::ShaderType::PS:
							return Core::String().Printf("ps_%i_%i", major, minor);
						case GPU::ShaderType::CS:
							return Core::String().Printf("cs_%i_%i", major, minor);
						}
						DBG_ASSERT(false);
						return Core::String();
					};

					// Compile HLSL.
					const i32 majorVer = 5;
					const i32 minorVer = 1;
					for(i32 idx = 0; idx < shaders.size(); ++idx)
						for(const auto& shader : shaders[idx])
							outCompileInfo.emplace_back(sourceFile, backendHLSL.GetOutputCode(), shader,
							    (GPU::ShaderType)idx, GetTarget((GPU::ShaderType)idx, majorVer, minorVer));

					for(const auto& compile : outCompileInfo)
					{
						auto outCompile = compilerHLSL.Compile(compile.name_.c_str(), compile.code_.c_str(),
						    compile.entryPoint_.c_str(), compile.type_, compile.target_.c_str());
						if(outCompile)
						{
							outCompileOutput.emplace_back(outCompile);
						}
						else
						{
							Core::String errStr(outCompile.errorsBegin_, outCompile.errorsEnd_);
							Core::Log("%s", errStr.c_str());
							return false;
						}
					}

					return true;
				};

				// Generate and compile initial pass.
				Core::Vector<CompileInfo> compileInfo;
				Core::Vector<Graphics::ShaderCompileOutput> compileOutput;
				if(!GenerateAndCompile(Graphics::BindingMap(), compileInfo, compileOutput))
				{
					// ERROR.
					return false;
				}

				// Get list of all used bindings.
				Graphics::BindingMap usedBindings;
				const auto AddBindings = [](
				    const Core::Vector<Graphics::ShaderBinding>& inBindings, Graphics::BindingMap& outBindings) {
					for(const auto& binding : inBindings)
					{
						if(outBindings.find(binding.name_) == nullptr)
						{
							outBindings.insert(binding.name_, binding.slot_);
						}
					}
				};

				for(const auto& compile : compileOutput)
					AddBindings(compile.cbuffers_, usedBindings);
				for(const auto& compile : compileOutput)
					AddBindings(compile.srvs_, usedBindings);
				for(const auto& compile : compileOutput)
					AddBindings(compile.uavs_, usedBindings);
				for(const auto& compile : compileOutput)
					AddBindings(compile.samplers_, usedBindings);

				// Regenerate HLSL with only the used bindings.
				if(!GenerateAndCompile(usedBindings, compileInfo, compileOutput))
				{
					// ERROR.
					return false;
				}

				// Build set of all bindings used.
				Graphics::BindingMap allBindings;
				Graphics::BindingMap cbvs;
				Graphics::BindingMap srvs;
				Graphics::BindingMap uavs;
				Graphics::BindingMap samplers;

				for(const auto& compile : compileOutput)
					AddBindings(compile.cbuffers_, allBindings);
				for(const auto& compile : compileOutput)
					AddBindings(compile.srvs_, allBindings);
				for(const auto& compile : compileOutput)
					AddBindings(compile.uavs_, allBindings);
				for(const auto& compile : compileOutput)
					AddBindings(compile.samplers_, allBindings);

				for(const auto& compile : compileOutput)
					AddBindings(compile.cbuffers_, cbvs);
				for(const auto& compile : compileOutput)
					AddBindings(compile.srvs_, srvs);
				for(const auto& compile : compileOutput)
					AddBindings(compile.uavs_, uavs);
				for(const auto& compile : compileOutput)
					AddBindings(compile.samplers_, samplers);

				Core::Vector<Graphics::ShaderBindingHeader> outBindingHeaders;
				outBindingHeaders.reserve(cbvs.size() + srvs.size() + uavs.size() + samplers.size());

				const auto PopulateOutBindingHeaders = [&outBindingHeaders](
				    const Graphics::ShaderBindingSetInfo& bindingSet, const Graphics::BindingMap& bindings,
				    Graphics::ShaderBindingFlags flags, i32 baseIdx, i32 num) {

					bool bindingSetUsed = false;
					for(i32 idx = 0; idx < num; ++idx)
					{
						const auto& member = bindingSet.members_[idx + baseIdx];

						if(bindings.find(member) != nullptr)
						{
							bindingSetUsed = true;
						}

						Graphics::ShaderBindingHeader bindingHeader;
						memset(&bindingHeader, 0, sizeof(bindingHeader));
						strcpy_s(bindingHeader.name_, sizeof(bindingHeader.name_), member.c_str());
						bindingHeader.handle_ = (Graphics::ShaderBindingHandle)(
						    flags | (Graphics::ShaderBindingFlags(idx) & Graphics::ShaderBindingFlags::INDEX_MASK));
						outBindingHeaders.push_back(bindingHeader);
					}
					return bindingSetUsed;
				};

				const auto& inBindingSets = backendMetadata.GetBindingSets();
				Core::Vector<Graphics::ShaderBindingSetHeader> outBindingSets;
				Core::Vector<i32> outBindingSetMapping;
				outBindingSets.reserve(inBindingSets.size());
				for(i32 idx = 0; idx < inBindingSets.size(); ++idx)
				{
					const auto& bindingSet = inBindingSets[idx];
					Graphics::ShaderBindingSetHeader outBindingSet;
					strcpy_s(outBindingSet.name_, sizeof(outBindingSet), bindingSet.name_.c_str());

					outBindingSet.isShared_ = bindingSet.shared_;
					if(bindingSet.frequency_ == "LOW")
						outBindingSet.frequency_ = 0;
					else if(bindingSet.frequency_ == "MEDIUM")
						outBindingSet.frequency_ = 128;
					else if(bindingSet.frequency_ == "HIGH")
						outBindingSet.frequency_ = 255;
					else if(bindingSet.frequency_.c_str())
						outBindingSet.frequency_ = Core::Clamp(atoi(bindingSet.frequency_.c_str()), 0, 255);

					outBindingSet.numCBVs_ = bindingSet.numCBVs_;
					outBindingSet.numSRVs_ = bindingSet.numSRVs_;
					outBindingSet.numUAVs_ = bindingSet.numUAVs_;
					outBindingSet.numSamplers_ = bindingSet.numSamplers_;

					i32 baseIdx = 0;
					bool bindingSetUsed = false;

					bindingSetUsed |= PopulateOutBindingHeaders(
					    bindingSet, allBindings, Graphics::ShaderBindingFlags::CBV, baseIdx, bindingSet.numCBVs_);
					baseIdx += bindingSet.numCBVs_;

					bindingSetUsed |= PopulateOutBindingHeaders(
					    bindingSet, allBindings, Graphics::ShaderBindingFlags::SRV, baseIdx, bindingSet.numSRVs_);
					baseIdx += bindingSet.numSRVs_;

					bindingSetUsed |= PopulateOutBindingHeaders(
					    bindingSet, allBindings, Graphics::ShaderBindingFlags::UAV, baseIdx, bindingSet.numUAVs_);
					baseIdx += bindingSet.numUAVs_;

					bindingSetUsed |= PopulateOutBindingHeaders(bindingSet, allBindings,
					    Graphics::ShaderBindingFlags::SAMPLER, baseIdx, bindingSet.numSamplers_);
					baseIdx += bindingSet.numSamplers_;

					// Only add used binding sets.
					if(bindingSetUsed)
					{
						outBindingSets.push_back(outBindingSet);
						outBindingSetMapping.push_back(idx);
					}
					else
					{
						// Remove binding headers.
						// TODO: Don't actually push them into the vector...
						for(i32 i = 0; i < baseIdx; ++i)
							outBindingHeaders.pop_back();
					}
				}

				// Determine binding sets per technique.
				auto FindShaderIdx = [&](const char* name) -> i32 {
					if(!name)
						return -1;
					i32 idx = 0;
					for(const auto& compile : compileInfo)
					{
						if(compile.entryPoint_ == name)
							return idx;
						++idx;
					}
					return -1;
				};

				// Setup data ready to serialize.

				Core::Vector<Graphics::ShaderSamplerStateHeader> outSamplerStateHeaders;
				outSamplerStateHeaders.reserve(samplerStates.size());
				for(const auto& samplerState : samplerStates)
				{
					Graphics::ShaderSamplerStateHeader outSamplerState;
					strcpy_s(outSamplerState.name_, sizeof(outSamplerState.name_), samplerState.name_.c_str());
					outSamplerState.state_ = samplerState.state_;
					outSamplerStateHeaders.emplace_back(outSamplerState);
				}

				Core::Vector<Graphics::ShaderBytecodeHeader> outBytecodeHeaders;
				i32 bytecodeOffset = 0;
				for(const auto& compile : compileOutput)
				{
					Graphics::ShaderBytecodeHeader bytecodeHeader;
					bytecodeHeader.type_ = compile.type_;
					bytecodeHeader.offset_ = bytecodeOffset;
					bytecodeHeader.numBytes_ = (i32)(compile.byteCodeEnd_ - compile.byteCodeBegin_);

					bytecodeOffset += bytecodeHeader.numBytes_;
					outBytecodeHeaders.push_back(bytecodeHeader);
				};

				Core::Vector<Graphics::ShaderTechniqueHeader> outTechniqueHeaders;

				for(i32 techIdx = 0; techIdx < techniques.size(); ++techIdx)
				{
					const auto& technique = techniques[techIdx];

					Graphics::ShaderTechniqueHeader techniqueHeader;
					memset(&techniqueHeader, 0, sizeof(techniqueHeader));
					strcpy_s(techniqueHeader.name_, sizeof(techniqueHeader.name_), technique.name_.c_str());

					techniqueHeader.vs_ = FindShaderIdx(technique.vs_.c_str());
					techniqueHeader.gs_ = FindShaderIdx(technique.gs_.c_str());
					techniqueHeader.hs_ = FindShaderIdx(technique.hs_.c_str());
					techniqueHeader.ds_ = FindShaderIdx(technique.ds_.c_str());
					techniqueHeader.ps_ = FindShaderIdx(technique.ps_.c_str());
					techniqueHeader.cs_ = FindShaderIdx(technique.cs_.c_str());
					techniqueHeader.rs_ = technique.rs_.state_;

					// Calculate used binding sets.
					Graphics::BindingMap techBindings;

					auto AddShaderBindings = [&](i32 shaderIdx) {
						if(shaderIdx != -1)
						{
							const Graphics::ShaderCompileOutput& compile = compileOutput[shaderIdx];
							AddBindings(compile.cbuffers_, techBindings);
							AddBindings(compile.srvs_, techBindings);
							AddBindings(compile.uavs_, techBindings);
							AddBindings(compile.samplers_, techBindings);
						}
					};

					AddShaderBindings(techniqueHeader.vs_);
					AddShaderBindings(techniqueHeader.hs_);
					AddShaderBindings(techniqueHeader.ds_);
					AddShaderBindings(techniqueHeader.gs_);
					AddShaderBindings(techniqueHeader.ps_);
					AddShaderBindings(techniqueHeader.cs_);

					techniqueHeader.bindingSlots_.fill(Graphics::ShaderTechniqueBindingSlot());

					i32 numBindingSets = 0;
					i32 cbvReg = 0;
					i32 srvReg = 0;
					i32 uavReg = 0;
					i32 samplerReg = 0;
					for(i32 bsIdx = 0; bsIdx < outBindingSets.size(); ++bsIdx)
					{
						const auto inBindingSet = inBindingSets[outBindingSetMapping[bsIdx]];
						const auto& bindingSet = outBindingSets[bsIdx];
						auto& bindingSlot = techniqueHeader.bindingSlots_[numBindingSets];
						for(auto member : inBindingSet.members_)
						{
							if(techBindings.find(member) != nullptr)
							{
								bindingSlot.idx_ = bsIdx;
								bindingSlot.cbvReg_ = cbvReg;
								bindingSlot.srvReg_ = srvReg;
								bindingSlot.uavReg_ = uavReg;
								bindingSlot.samplerReg_ = samplerReg;
								numBindingSets++;
								break;
							}
						}

						cbvReg += bindingSet.numCBVs_;
						srvReg += bindingSet.numSRVs_;
						uavReg += bindingSet.numUAVs_;
						samplerReg += bindingSet.numSamplers_;
					}
					techniqueHeader.numBindingSets_ = numBindingSets;

					DBG_ASSERT(techniqueHeader.vs_ != -1 || techniqueHeader.cs_ != -1);

					outTechniqueHeaders.push_back(techniqueHeader);
				}

				Graphics::ShaderHeader outHeader;
				outHeader.numShaders_ = compileOutput.size();
				outHeader.numTechniques_ = techniques.size();
				outHeader.numSamplerStates_ = samplerStates.size();
				outHeader.numBindingSets_ = outBindingSets.size();

				auto WriteShader = [&](const char* outFilename) {
					// Write out shader data.
					if(auto outFile = Core::File(outFilename, Core::FileFlags::DEFAULT_WRITE))
					{
						outFile.Write(&outHeader, sizeof(outHeader));
						if(outBindingSets.size() > 0)
							outFile.Write(outBindingSets.data(),
							    outBindingSets.size() * sizeof(Graphics::ShaderBindingSetHeader));
						if(outBindingHeaders.size() > 0)
							outFile.Write(outBindingHeaders.data(),
							    outBindingHeaders.size() * sizeof(Graphics::ShaderBindingHeader));
						if(outBytecodeHeaders.size() > 0)
							outFile.Write(outBytecodeHeaders.data(),
							    outBytecodeHeaders.size() * sizeof(Graphics::ShaderBytecodeHeader));
						if(outTechniqueHeaders.size() > 0)
							outFile.Write(outTechniqueHeaders.data(),
							    outTechniqueHeaders.size() * sizeof(Graphics::ShaderTechniqueHeader));
						if(outSamplerStateHeaders.size() > 0)
							outFile.Write(outSamplerStateHeaders.data(),
							    outSamplerStateHeaders.size() * sizeof(Graphics::ShaderSamplerStateHeader));

						i64 outBytes = 0;
						for(const auto& compile : compileOutput)
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
