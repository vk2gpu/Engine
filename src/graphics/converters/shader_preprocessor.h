#pragma once

#include "core/types.h"
#include "core/linear_allocator.h"
#include "core/string.h"

extern "C" {
struct fppTag;
}

namespace Graphics
{
	class ShaderPreprocessor
	{
	public:
		ShaderPreprocessor(const char* inputFile, const char* inputData);
		~ShaderPreprocessor();

		void AddInclude(const char* includePath);
		void AddDefine(const char* define, const char* value);
		bool Preprocess();
		const Core::String& GetOutput() { return output_; }

	private:
		static void ShaderPreprocessor::cbError(void* userData, char* format, va_list varArgs);
		static char* ShaderPreprocessor::cbInput(char* buffer, int size, void* userData);
		static void ShaderPreprocessor::cbOutput(int inChar, void* userData);
		static void ShaderPreprocessor::cbDependency(char* dependency, void* userData);

		Core::Vector<fppTag> tags_;
		char* inputData_;
		i32 inputOffset_;
		i32 inputSize_;
		Core::LinearAllocator allocator_;
		Core::String output_;
	};

} // namespace Graphics
