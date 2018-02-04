#pragma once

#include "graphics/converters/shader_ast.h"
#include "core/hash.h"
#include "gpu/types.h"

namespace Graphics
{
	struct ShaderBinding
	{
		ShaderBinding(i32 slot, const char* name)
		    : slot_(slot)
		    , name_(name)
		{
		}

		i32 slot_;
		Core::String name_;
	};

	struct ShaderCompileOutput
	{
		const u8* byteCodeBegin_ = nullptr;
		const u8* byteCodeEnd_ = nullptr;
		const u8* strippedByteCodeBegin_ = nullptr;
		const u8* strippedByteCodeEnd_ = nullptr;
		const char* errorsBegin_ = nullptr;
		const char* errorsEnd_ = nullptr;

		GPU::ShaderType type_;

		Core::HashSHA1Digest byteCodeHash_;
		Core::HashSHA1Digest strippedByteCodeHash_;
		Core::HashSHA1Digest errorsHash_;

		Core::Vector<ShaderBinding> cbuffers_;
		Core::Vector<ShaderBinding> samplers_;
		Core::Vector<ShaderBinding> srvs_;
		Core::Vector<ShaderBinding> uavs_;

		explicit operator bool() const { return !!byteCodeBegin_; }
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
		ShaderCompileOutput Compile(const char* shaderName, const char* shaderSource, const char* entryPoint,
		    GPU::ShaderType type, const char* target);

	private:
		struct ShaderCompilerHLSLImpl* impl_ = nullptr;
	};
} // namespace Graphics
