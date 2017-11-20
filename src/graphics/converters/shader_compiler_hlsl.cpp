#include "graphics/converters/shader_compiler_hlsl.h"

#include "core/library.h"

#include <cstdarg>

#include <D3DCompiler.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

const GUID IID_ID3D11ShaderReflection = {0x8d536ca1, 0x0cca, 0x4956, {0xa8, 0x37, 0x78, 0x69, 0x63, 0x75, 0x55, 0x84}};

namespace Graphics
{
	struct ShaderCompilerHLSLImpl
	{
		Core::LibHandle d3dCompilerLib_;

		decltype(D3DCompile)* d3dCompile_;
		decltype(D3DStripShader)* d3dStripShader_;
		decltype(D3DReflect)* d3dReflect_;

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
				d3dReflect_ = (decltype(d3dReflect_))Core::LibrarySymbol(d3dCompilerLib_, "D3DReflect");
				DBG_ASSERT(d3dReflect_);
			}
		}

		~ShaderCompilerHLSLImpl()
		{
			byteCodes_.clear();
			strippedByteCodes_.clear();
			errors_.clear();

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
	    const char* shaderName, const char* shaderSource, const char* entryPoint, GPU::ShaderType type)
	{
		DBG_ASSERT(impl_);

		const char* target = nullptr;
		switch(type)
		{
		case GPU::ShaderType::VS:
			target = "vs_5_0";
			break;
		case GPU::ShaderType::GS:
			target = "gs_5_0";
			break;
		case GPU::ShaderType::HS:
			target = "hs_5_0";
			break;
		case GPU::ShaderType::DS:
			target = "ds_5_0";
			break;
		case GPU::ShaderType::PS:
			target = "ps_5_0";
			break;
		case GPU::ShaderType::CS:
			target = "cs_5_0";
			break;
		}

		ComPtr<ID3DBlob> byteCode;
		ComPtr<ID3DBlob> errors;
		HRESULT retVal = impl_->d3dCompile_(shaderSource, strlen(shaderSource), shaderName, nullptr, nullptr,
		    entryPoint, target, D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_DEBUG, 0, byteCode.ReleaseAndGetAddressOf(),
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

			output.type_ = type;

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

			// Shader reflection info.
			ComPtr<ID3D11ShaderReflection> reflection;
			retVal = impl_->d3dReflect_(output.byteCodeBegin_, output.byteCodeEnd_ - output.byteCodeBegin_,
			    IID_ID3D11ShaderReflection, (void**)&reflection);
			if(SUCCEEDED(retVal))
			{
				D3D11_SHADER_DESC desc;
				reflection->GetDesc(&desc);

				for(i32 i = 0; i < (i32)desc.BoundResources; ++i)
				{
					D3D11_SHADER_INPUT_BIND_DESC bindDesc;
					reflection->GetResourceBindingDesc(i, &bindDesc);
					switch(bindDesc.Type)
					{
					case D3D_SIT_CBUFFER:
						output.cbuffers_.emplace_back(bindDesc.BindPoint, bindDesc.Name);
						break;
					case D3D_SIT_SAMPLER:
						output.samplers_.emplace_back(bindDesc.BindPoint, bindDesc.Name);
						break;
					case D3D_SIT_TBUFFER:
					case D3D_SIT_TEXTURE:
					case D3D_SIT_STRUCTURED:
					case D3D_SIT_BYTEADDRESS:
						output.srvs_.emplace_back(bindDesc.BindPoint, bindDesc.Name);
						break;
					case D3D_SIT_UAV_RWTYPED:
					case D3D_SIT_UAV_RWSTRUCTURED:
					case D3D_SIT_UAV_RWBYTEADDRESS:
					case D3D_SIT_UAV_APPEND_STRUCTURED:
					case D3D_SIT_UAV_CONSUME_STRUCTURED:
					case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
						output.uavs_.emplace_back(bindDesc.BindPoint, bindDesc.Name);
						break;
					}
				}
			}
		}

		return output;
	}

} /// namespace Graphics
