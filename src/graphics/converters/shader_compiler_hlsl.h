#pragma once

#include "graphics/converters/shader_ast.h"

namespace Graphics
{
	struct ShaderCompileOutput
	{
		const u8* byteCodeBegin_ = nullptr;
		const u8* byteCodeEnd_ = nullptr;
		const char* errorsBegin_ = nullptr;
		const char* errorsEnd_ = nullptr;

		operator bool() const { return !!byteCodeBegin_; }
	};

	class ShaderCompilerHLSL
	{
	public:
		ShaderCompilerHLSL();
		virtual ~ShaderCompilerHLSL();

		/**
		 * Compile shader.
		 * @return Compile output. Contains errors and bytecode (if successful). Only valid to use while this ShaderCompilerHLSL object remains in scope.
		 */
		ShaderCompileOutput Compile(
		    const char* shaderName, const char* shaderSource, const char* entryPoint, const char* target);

	private:
		struct ShaderCompilerHLSLImpl* impl_ = nullptr;
	};
} // namespace Graphics
