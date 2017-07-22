#pragma once

#include "graphics/converters/shader_ast.h"
#include "core/hash.h"

namespace Graphics
{
	struct ShaderCompileOutput
	{
		const u8* byteCodeBegin_ = nullptr;
		const u8* byteCodeEnd_ = nullptr;
		const u8* strippedByteCodeBegin_ = nullptr;
		const u8* strippedByteCodeEnd_ = nullptr;
		const char* errorsBegin_ = nullptr;
		const char* errorsEnd_ = nullptr;

		Core::HashSHA1Digest byteCodeHash_;
		Core::HashSHA1Digest strippedByteCodeHash_;
		Core::HashSHA1Digest errorsHash_;

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
