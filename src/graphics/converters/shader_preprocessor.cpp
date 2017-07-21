#include "graphics/converters/shader_preprocessor.h"

extern "C" {
#undef INLINE
#include <cppdef.h>
#include <fpp.h>
}

namespace Graphics
{
	ShaderPreprocessor::ShaderPreprocessor()
	    : allocator_(256 * 1024)
	{
	}

	ShaderPreprocessor::~ShaderPreprocessor() {}

	void ShaderPreprocessor::AddInclude(const char* includePath)
	{
		i32 size = (i32)strlen(includePath) + 1;
		char* data = allocator_.Allocate<char>(size);
		strcpy_s(data, size, includePath);

		fppTag tag;
		tag.tag = FPPTAG_INCLUDE_DIR;
		tag.data = data;
		tags_.push_back(tag);
	}

	void ShaderPreprocessor::AddDefine(const char* define, const char* value)
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

	bool ShaderPreprocessor::Preprocess(const char* inputFile, const char* inputData)
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

		tag.tag = FPPTAG_LINE;
		tag.data = (void*)1;
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

		tag.tag = FPPTAG_END;
		tag.data = (void*)0;
		tags_.push_back(tag);

		// Unix line endings.
		Core::String fixedInputData = Core::String(inputData).replace("\r\n", "\n");
		inputOffset_ = 0;
		inputSize_ = fixedInputData.size() + 1;
		inputData_ = allocator_.Allocate<char>(inputSize_);
		memset(inputData_, 0, inputSize_);
		strcpy_s(inputData_, inputSize_, fixedInputData.c_str());

		int result = fppPreProcess(tags_.data());

		allocator_.Reset();

		return result == 0;
	}

	void ShaderPreprocessor::cbError(void* userData, char* format, va_list varArgs)
	{
		Core::String errorStr;
		errorStr.Printfv(format, varArgs);
		DBG_LOG("%s", errorStr.c_str());
	}

	char* ShaderPreprocessor::cbInput(char* buffer, int size, void* userData)
	{
		auto* _this = static_cast<Graphics::ShaderPreprocessor*>(userData);
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

	void ShaderPreprocessor::cbOutput(int inChar, void* userData)
	{
		auto* _this = static_cast<Graphics::ShaderPreprocessor*>(userData);
		char inStr[] = {(char)inChar, '\0'};
		_this->output_.append(inStr);
	}

	void ShaderPreprocessor::cbDependency(char* dependency, void* userData)
	{
		auto* _this = static_cast<Graphics::ShaderPreprocessor*>(userData);
		i32 len = (i32)strlen((char*)dependency) + 1;
		char* str = _this->allocator_.Allocate<char>(len);
		strcpy_s(str, len, (char*)dependency);
		_this->dependencies_.push_back(str);
	}
} // namespace Graphics
