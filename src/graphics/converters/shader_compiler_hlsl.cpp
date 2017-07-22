#include "graphics/converters/shader_compiler_hlsl.h"

#include "core/library.h"

#include <cstdarg>

#include <D3DCompiler.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

namespace Graphics
{
	struct ShaderCompilerHLSLImpl
	{
		Core::LibHandle d3dCompilerLib_;

		decltype(D3DCompile)* d3dCompile_;

		Core::Vector<ComPtr<ID3DBlob>> byteCodes_;
		Core::Vector<ComPtr<ID3DBlob>> errors_;

		ShaderCompilerHLSLImpl()
		{
			d3dCompilerLib_ = Core::LibraryOpen(D3DCOMPILER_DLL_A);
			DBG_ASSERT(d3dCompilerLib_);
			if(d3dCompilerLib_)
			{
				d3dCompile_ = (decltype(d3dCompile_))Core::LibrarySymbol(d3dCompilerLib_, "D3DCompile");
				DBG_ASSERT(d3dCompile_);
			}
		}

		~ShaderCompilerHLSLImpl()
		{
			if(d3dCompilerLib_)
			{
				Core::LibraryClose(d3dCompilerLib_);
			}
		}
	};

	ShaderCompilerHLSL::ShaderCompilerHLSL() { impl_ = new ShaderCompilerHLSLImpl(); }

	ShaderCompilerHLSL::~ShaderCompilerHLSL()
	{
		delete impl_;
		impl_ = nullptr;
	}

	ShaderCompileOutput ShaderCompilerHLSL::Compile(
	    const char* shaderName, const char* shaderSource, const char* entryPoint, const char* target)
	{
		DBG_ASSERT(impl_);

		ComPtr<ID3DBlob> byteCode;
		ComPtr<ID3DBlob> errors;
		HRESULT retVal =
		    impl_->d3dCompile_(shaderSource, strlen(shaderSource), shaderName, nullptr, nullptr, entryPoint, target,
		        D3DCOMPILE_DEBUG, 0, byteCode.ReleaseAndGetAddressOf(), errors.ReleaseAndGetAddressOf());

		ShaderCompileOutput output;

		if(errors)
		{
			impl_->errors_.push_back(errors);

			output.errorsBegin_ = (const char*)errors->GetBufferPointer();
			output.errorsEnd_ = output.errorsBegin_ + errors->GetBufferSize();
		}

		if(SUCCEEDED(retVal))
		{
			impl_->byteCodes_.push_back(byteCode);

			output.byteCodeBegin_ = (const u8*)byteCode->GetBufferPointer();
			output.byteCodeEnd_ = output.byteCodeBegin_ + byteCode->GetBufferSize();
		}

		return output;
	}

} /// namespace Graphics
