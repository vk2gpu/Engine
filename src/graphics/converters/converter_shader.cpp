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

extern "C" {
#include <cppdef.h>
#include <fpp.h>
}

#include <cstring>

namespace
{
	class FCPPInterface
	{
	public:
		FCPPInterface(const char* inputFile, const Core::String& inputData)
		    : allocator_(256 * 1024)
		{
			fppTag tag;
			tag.tag = FPPTAG_USERDATA;
			tag.data = this;
			tags_.push_back(tag);

			tag.tag = FPPTAG_ERROR;
			tag.data = (void*)cbError;
			tags_.push_back(tag);

			tag.tag = FPPTAG_INPUT;
			tag.data = (void*)cbInput;
			tags_.push_back(tag);

			tag.tag = FPPTAG_OUTPUT;
			tag.data = (void*)cbOutput;
			tags_.push_back(tag);

			tag.tag = FPPTAG_DEPENDENCY;
			tag.data = (void*)cbDependency;
			tags_.push_back(tag);

			tag.tag = FPPTAG_IGNOREVERSION;
			tag.data = (void*)0;
			tags_.push_back(tag);

			tag.tag = FPPTAG_LINE;
			tag.data = (void*)0;
			tags_.push_back(tag);

			tag.tag = FPPTAG_KEEPCOMMENTS;
			tag.data = (void*)0;
			tags_.push_back(tag);

			tag.tag = FPPTAG_INPUT_NAME;
			tag.data = (void*)inputFile;
			tags_.push_back(tag);

			// Unix line endings.
			Core::String fixedInputData = inputData.replace("\r\n", "\n");
			inputOffset_ = 0;
			inputSize_ = fixedInputData.size() + 1;
			inputData_ = allocator_.Allocate<char>(inputSize_);
			memset(inputData_, 0, inputSize_);
			strcpy_s(inputData_, inputSize_, fixedInputData.c_str());
		}

		~FCPPInterface() {}

		void AddInclude(const char* includePath)
		{
			i32 size = (i32)strlen(includePath) + 1;
			char* data = allocator_.Allocate<char>(size);
			strcpy_s(data, size, includePath);

			fppTag tag;
			tag.tag = FPPTAG_INCLUDE_DIR;
			tag.data = data;
			tags_.push_back(tag);
		}

		void AddDefine(const char* define, const char* value)
		{
			i32 size = (i32)strlen(define) + 1;
			if(value)
			{
				size += (i32)strlen(value) + 1;
			}
			char* data = allocator_.Allocate<char>(size);
			memset(data, 0, size);
			if(value)
			{
				sprintf_s(data, size, "%s=%s", define, value);
			}
			else
			{
				strcpy_s(data, size, define);
			}

			fppTag tag;
			tag.tag = FPPTAG_DEFINE;
			tag.data = data;
			tags_.push_back(tag);
		}

		bool Preprocess()
		{
			fppTag tag;

			tag.tag = FPPTAG_END;
			tag.data = (void*)0;
			tags_.push_back(tag);

			int result = fppPreProcess(tags_.data());

			tags_.pop_back();

			return result == 0;
		}

		const Core::String& GetOutput() { return output_; }

		static void cbError(void* userData, char* format, va_list varArgs)
		{
			Core::String errorStr;
			errorStr.Printfv(format, varArgs);
			DBG_LOG("ERROR: %s", errorStr.c_str());
		}

		static char* cbInput(char* buffer, int size, void* userData)
		{
			FCPPInterface* _this = static_cast<FCPPInterface*>(userData);
			int outIdx = 0;
			char inputChar = _this->inputData_[_this->inputOffset_];
			while(_this->inputOffset_ < _this->inputSize_ && outIdx < (size - 1))
			{
				buffer[outIdx] = inputChar;
				if(inputChar == '\n' || outIdx == (size - 1))
				{
					buffer[++outIdx] = '\0';
					_this->inputOffset_++;
					return buffer;
				}
				++outIdx;
				inputChar = _this->inputData_[++_this->inputOffset_];
			}
			return nullptr;
		}

		static void cbOutput(int inChar, void* userData)
		{
			FCPPInterface* _this = static_cast<FCPPInterface*>(userData);
			char inStr[] = {(char)inChar, '\0'};
			_this->output_.append(inStr);
		}

		static void cbDependency(char* dependency, void* userData)
		{
			//FCPPInterface* _this = static_cast< FCPPInterface* >( userData );
			//_this->Importer_.addDependency( Dependency );
		}

	private:
		Core::Vector<fppTag> tags_;
		char* inputData_;
		i32 inputOffset_;
		i32 inputSize_;
		Core::LinearAllocator allocator_;
		Core::String output_;
	};
}

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

				FCPPInterface preprocessor(sourceFile, shaderSource.data());

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
