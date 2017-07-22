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
		decltype(D3DStripShader)* d3dStripShader_;


		Core::Vector<ComPtr<ID3DBlob>> byteCodes_;
		Core::Vector<ComPtr<ID3DBlob>> strippedByteCodes_;
		Core::Vector<ComPtr<ID3DBlob>> errors_;

		ShaderCompilerHLSLImpl()
		{
			d3dCompilerLib_ = Core::LibraryOpen(D3DCOMPILER_DLL_A);
			DBG_ASSERT(d3dCompilerLib_);
			if(d3dCompilerLib_)
			{
				d3dCompile_ = (decltype(d3dCompile_))Core::LibrarySymbol(d3dCompilerLib_, "D3DCompile");
				DBG_ASSERT(d3dCompile_);
				d3dStripShader_ = (decltype(d3dStripShader_))Core::LibrarySymbol(d3dCompilerLib_, "D3DStripShader");
				DBG_ASSERT(d3dStripShader_);
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
		HRESULT retVal = impl_->d3dCompile_(shaderSource, strlen(shaderSource), shaderName, nullptr, nullptr,
		    entryPoint, target, D3DCOMPILE_OPTIMIZATION_LEVEL2 | D3DCOMPILE_DEBUG, 0, byteCode.ReleaseAndGetAddressOf(),
		    errors.ReleaseAndGetAddressOf());

		ShaderCompileOutput output;

		if(errors)
		{
			impl_->errors_.push_back(errors);

			output.errorsBegin_ = (const char*)errors->GetBufferPointer();
			output.errorsEnd_ = output.errorsBegin_ + errors->GetBufferSize();
			output.errorsHash_ = Core::HashSHA1(output.errorsBegin_, errors->GetBufferSize());
		}

		if(SUCCEEDED(retVal))
		{
			impl_->byteCodes_.push_back(byteCode);

			output.byteCodeBegin_ = (const u8*)byteCode->GetBufferPointer();
			output.byteCodeEnd_ = output.byteCodeBegin_ + byteCode->GetBufferSize();
			output.byteCodeHash_ = Core::HashSHA1(output.byteCodeBegin_, byteCode->GetBufferSize());

			ComPtr<ID3DBlob> strippedByteCode;
			retVal = impl_->d3dStripShader_(output.byteCodeBegin_, byteCode->GetBufferSize(),
			    D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_DEBUG_INFO,
			    strippedByteCode.ReleaseAndGetAddressOf());
			if(SUCCEEDED(retVal))
			{
				impl_->strippedByteCodes_.push_back(strippedByteCode);

				output.strippedByteCodeBegin_ = (const u8*)strippedByteCode->GetBufferPointer();
				output.strippedByteCodeEnd_ = output.strippedByteCodeBegin_ + strippedByteCode->GetBufferSize();
				output.strippedByteCodeHash_ =
				    Core::HashSHA1(output.strippedByteCodeBegin_, strippedByteCode->GetBufferSize());
			}
		}

		return output;
	}

} /// namespace Graphics
