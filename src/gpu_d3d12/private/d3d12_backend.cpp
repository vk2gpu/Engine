#include "gpu_d3d12/d3d12_backend.h"
#include "gpu_d3d12/d3d12_device.h"
#include "gpu_d3d12/d3d12_linear_heap_allocator.h"
#include "core/debug.h"
#include "core/string.h"

#include <utility>

extern "C" {
EXPORT GPU::IBackend* CreateBackend(void* deviceWindow)
{
	return new GPU::D3D12Backend(deviceWindow);
}

EXPORT void DestroyBackend(GPU::IBackend* backend)
{
	delete backend;
}
}

namespace GPU
{
	D3D12Backend::D3D12Backend(void* deviceWindow)
	{
		auto retVal = LoadLibraries();
		DBG_ASSERT(retVal == ErrorCode::OK);
		UINT flags = 0;

#if !defined(FINAL)
		HRESULT hr = S_OK;
		// Setup debug interfaces.
		if(DXGIGetDebugInterface1Fn)
		{
			hr = DXGIGetDebugInterface1Fn(0, IID_IDXGIDebug, (void**)dxgiDebug_.GetAddressOf());
			ComPtr<IDXGIInfoQueue> infoQueue;
			hr = dxgiDebug_.As(&infoQueue);
			if(SUCCEEDED(hr))
			{
				infoQueue->SetMuteDebugOutput(DXGI_DEBUG_ALL, FALSE);
				CHECK_D3D(
				    infoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE));
				CHECK_D3D(infoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE));
				CHECK_D3D(
				    infoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, FALSE));

				infoQueue->AddApplicationMessage(
				    DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, "DXGI error reporting ENABLED.");
			}
		}

		if(D3D12GetDebugInterfaceFn)
		{
			hr = D3D12GetDebugInterfaceFn(IID_ID3D12Debug, (void**)d3dDebug_.GetAddressOf());
			if(SUCCEEDED(hr))
			{
				d3dDebug_->EnableDebugLayer();
			}
		}

		flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

		DXGICreateDXGIFactory2Fn(flags, IID_IDXGIFactory4, (void**)dxgiFactory_.ReleaseAndGetAddressOf());
	}

	D3D12Backend::~D3D12Backend()
	{
		delete device_;
		dxgiAdapters_.clear();
		adapterInfos_.clear();
		dxgiFactory_ = nullptr;

#if !defined(FINAL)
		if(dxgiDebug_)
		{
			dxgiDebug_->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		}
#endif
	}

	i32 D3D12Backend::EnumerateAdapters(AdapterInfo* outAdapters, i32 maxAdapters)
	{
		if(adapterInfos_.size() == 0)
		{
			ComPtr<IDXGIAdapter1> dxgiAdapter;
			while(SUCCEEDED(dxgiFactory_->EnumAdapters1(dxgiAdapters_.size(), dxgiAdapter.ReleaseAndGetAddressOf())))
			{
				DXGI_ADAPTER_DESC1 desc;
				dxgiAdapter->GetDesc1(&desc);

				AdapterInfo outAdapter;
				Core::StringConvertUTF16toUTF8(desc.Description, 128, outAdapter.description_, sizeof(outAdapter));
				outAdapter.deviceIdx_ = dxgiAdapters_.size();
				outAdapter.vendorId_ = desc.VendorId;
				outAdapter.deviceId_ = desc.DeviceId;
				outAdapter.subSysId_ = desc.SubSysId;
				outAdapter.revision_ = desc.Revision;
				outAdapter.dedicatedVideoMemory_ = desc.DedicatedVideoMemory;
				outAdapter.dedicatedSystemMemory_ = desc.DedicatedSystemMemory;
				outAdapter.sharedSystemMemory_ = desc.SharedSystemMemory;
				adapterInfos_.push_back(outAdapter);
				dxgiAdapters_.push_back(dxgiAdapter);
			}
		}

		for(i32 idx = 0; idx < maxAdapters; ++idx)
		{
			outAdapters[idx] = adapterInfos_[idx];
		}

		return adapterInfos_.size();
	}

	bool D3D12Backend::IsInitialized() const { return !!device_; }

	ErrorCode D3D12Backend::Initialize(i32 adapterIdx)
	{
		device_ = new D3D12Device(dxgiFactory_.Get(), dxgiAdapters_[adapterIdx].Get());
		if(*device_ == false)
		{
			delete device_;
			device_ = nullptr;
			return ErrorCode::FAIL;
		}
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::CreateSwapChain(Handle handle, const SwapChainDesc& desc, const char* debugName)
	{
		D3D12SwapChain swapChain;
		ErrorCode retVal = device_->CreateSwapChain(swapChain, desc, debugName);
		if(retVal != ErrorCode::OK)
			return retVal;
		Core::ScopedMutex lock(resourceMutex_);
		swapchainResources_[handle.GetIndex()] = swapChain;
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::CreateBuffer(
	    Handle handle, const BufferDesc& desc, const void* initialData, const char* debugName)
	{
		D3D12Resource buffer;
		ErrorCode retVal = device_->CreateBuffer(buffer, desc, initialData, debugName);
		if(retVal != ErrorCode::OK)
			return retVal;
		Core::ScopedMutex lock(resourceMutex_);
		bufferResources_[handle.GetIndex()] = buffer;
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::CreateTexture(
	    Handle handle, const TextureDesc& desc, const TextureSubResourceData* initialData, const char* debugName)
	{
		D3D12Resource texture;
		ErrorCode retVal = device_->CreateTexture(texture, desc, initialData, debugName);
		if(retVal != ErrorCode::OK)
			return retVal;
		Core::ScopedMutex lock(resourceMutex_);
		textureResources_[handle.GetIndex()] = texture;
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::CreateSamplerState(Handle handle, const SamplerState& state, const char* debugName)
	{
		D3D12SamplerState samplerState;

		auto GetAddessingMode = [](AddressingMode addressMode) {
			switch(addressMode)
			{
			case AddressingMode::WRAP:
				return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			case AddressingMode::MIRROR:
				return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			case AddressingMode::CLAMP:
				return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			case AddressingMode::BORDER:
				return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			default:
				DBG_BREAK;
				return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			}
		};

		auto GetFilteringMode = [](FilteringMode min, FilteringMode mag, u32 anisotropy) {
			if(min == FilteringMode::NEAREST && mag == FilteringMode::NEAREST)
			{
				return D3D12_FILTER_MIN_MAG_MIP_POINT;
			}
			else if(min == FilteringMode::NEAREST_MIPMAP_LINEAR && mag == FilteringMode::NEAREST)
			{
				return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
			}
			else if(min == FilteringMode::LINEAR && mag == FilteringMode::NEAREST)
			{
				return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
			}
			else if(min == FilteringMode::LINEAR_MIPMAP_LINEAR && mag == FilteringMode::NEAREST)
			{
				return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
			}
			else if(min == FilteringMode::LINEAR && mag == FilteringMode::LINEAR)
			{
				return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			}
			else if(min == FilteringMode::LINEAR_MIPMAP_LINEAR && mag == FilteringMode::LINEAR)
			{
				if(anisotropy > 1)
				{
					return D3D12_FILTER_ANISOTROPIC;
				}
				else
				{
					return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
				}
			}
			return D3D12_FILTER_MIN_MAG_MIP_POINT;
		};

		memset(&samplerState, 0, sizeof(samplerState));
		samplerState.desc_.AddressU = GetAddessingMode(state.addressU_);
		samplerState.desc_.AddressV = GetAddessingMode(state.addressV_);
		samplerState.desc_.AddressW = GetAddessingMode(state.addressW_);
		samplerState.desc_.Filter = GetFilteringMode(state.minFilter_, state.magFilter_, state.maxAnisotropy_);
		samplerState.desc_.MipLODBias = state.mipLODBias_;
		samplerState.desc_.MaxAnisotropy = state.maxAnisotropy_;
		samplerState.desc_.BorderColor[0] = state.borderColor_[0];
		samplerState.desc_.BorderColor[1] = state.borderColor_[1];
		samplerState.desc_.BorderColor[2] = state.borderColor_[2];
		samplerState.desc_.BorderColor[3] = state.borderColor_[3];
		samplerState.desc_.MinLOD = state.minLOD_;
		samplerState.desc_.MaxLOD = state.maxLOD_;

		Core::ScopedMutex lock(resourceMutex_);
		samplerStates_[handle.GetIndex()] = samplerState;
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::CreateShader(Handle handle, const ShaderDesc& desc, const char* debugName)
	{
		D3D12Shader shader;
		shader.byteCode_ = new u8[desc.dataSize_];
		shader.byteCodeSize_ = desc.dataSize_;
		memcpy(shader.byteCode_, desc.data_, desc.dataSize_);
		Core::ScopedMutex lock(resourceMutex_);
		shaders_[handle.GetIndex()] = shader;
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::CreateGraphicsPipelineState(
	    Handle handle, const GraphicsPipelineStateDesc& desc, const char* debugName)
	{
		D3D12GraphicsPipelineState gps;
		D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

		auto GetShaderBytecode = [&](ShaderType shaderType) {
			D3D12_SHADER_BYTECODE byteCode = {};
			auto shaderHandle = desc.shaders_[(i32)shaderType];
			if(shaderHandle)
			{
				const auto& shader = shaders_[shaderHandle.GetIndex()];
				byteCode.pShaderBytecode = shader.byteCode_;
				byteCode.BytecodeLength = shader.byteCodeSize_;
			}
			return byteCode;
		};

		auto GetBlendState = [](BlendState blendState) {
			auto GetBlend = [](BlendType type) {
				switch(type)
				{
				case BlendType::ZERO:
					return D3D12_BLEND_ZERO;
				case BlendType::ONE:
					return D3D12_BLEND_ONE;
				case BlendType::SRC_COLOR:
					return D3D12_BLEND_SRC_COLOR;
				case BlendType::INV_SRC_COLOR:
					return D3D12_BLEND_INV_SRC_COLOR;
				case BlendType::SRC_ALPHA:
					return D3D12_BLEND_SRC_ALPHA;
				case BlendType::INV_SRC_ALPHA:
					return D3D12_BLEND_INV_SRC_ALPHA;
				case BlendType::DEST_COLOR:
					return D3D12_BLEND_DEST_COLOR;
				case BlendType::INV_DEST_COLOR:
					return D3D12_BLEND_INV_DEST_COLOR;
				case BlendType::DEST_ALPHA:
					return D3D12_BLEND_DEST_ALPHA;
				case BlendType::INV_DEST_ALPHA:
					return D3D12_BLEND_INV_DEST_ALPHA;
				default:
					DBG_BREAK;
					return D3D12_BLEND_ZERO;
				}
			};

			auto GetBlendOp = [](BlendFunc func) {
				switch(func)
				{
				case BlendFunc::ADD:
					return D3D12_BLEND_OP_ADD;
				case BlendFunc::SUBTRACT:
					return D3D12_BLEND_OP_SUBTRACT;
				case BlendFunc::REV_SUBTRACT:
					return D3D12_BLEND_OP_REV_SUBTRACT;
				case BlendFunc::MINIMUM:
					return D3D12_BLEND_OP_MIN;
				case BlendFunc::MAXIMUM:
					return D3D12_BLEND_OP_MAX;
				default:
					DBG_BREAK;
					return D3D12_BLEND_OP_ADD;
				}
			};

			D3D12_RENDER_TARGET_BLEND_DESC state;
			state.BlendEnable = blendState.enable_ ? TRUE : FALSE;
			state.LogicOpEnable = FALSE;
			state.SrcBlend = GetBlend(blendState.srcBlend_);
			state.DestBlend = GetBlend(blendState.destBlend_);
			state.BlendOp = GetBlendOp(blendState.blendOp_);
			state.SrcBlendAlpha = GetBlend(blendState.srcBlendAlpha_);
			state.DestBlendAlpha = GetBlend(blendState.destBlendAlpha_);
			state.BlendOpAlpha = GetBlendOp(blendState.blendOpAlpha_);
			state.LogicOp = D3D12_LOGIC_OP_CLEAR;
			state.RenderTargetWriteMask = blendState.writeMask_;
			return state;
		};

		auto GetRasterizerState = [](const RenderState& renderState) {
			auto GetFillMode = [](FillMode fillMode) {
				switch(fillMode)
				{
				case FillMode::SOLID:
					return D3D12_FILL_MODE_SOLID;
				case FillMode::WIREFRAME:
					return D3D12_FILL_MODE_WIREFRAME;
				default:
					DBG_BREAK;
					return D3D12_FILL_MODE_SOLID;
				}
			};

			auto GetCullMode = [](CullMode cullMode) {
				switch(cullMode)
				{
				case CullMode::NONE:
					return D3D12_CULL_MODE_NONE;
				case CullMode::CCW:
					return D3D12_CULL_MODE_FRONT;
				case CullMode::CW:
					return D3D12_CULL_MODE_BACK;
				default:
					DBG_BREAK;
					return D3D12_CULL_MODE_NONE;
				}
			};

			D3D12_RASTERIZER_DESC desc;
			desc.FillMode = GetFillMode(renderState.fillMode_);
			desc.CullMode = GetCullMode(renderState.cullMode_);
			desc.FrontCounterClockwise = TRUE;
			desc.DepthBias = (u32)(renderState.depthBias_ * 0xffffff); // TODO: Use depth format.
			desc.SlopeScaledDepthBias = renderState.slopeScaledDepthBias_;
			desc.DepthClipEnable = FALSE;
			desc.DepthBiasClamp = 0.0f;
			desc.MultisampleEnable = FALSE; // TODO:
			desc.AntialiasedLineEnable = renderState.antialiasedLineEnable_ ? TRUE : FALSE;
			desc.ForcedSampleCount = 0;
			desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

			return desc;
		};

		auto GetDepthStencilState = [](const RenderState& renderState) {
			auto GetCompareMode = [](CompareMode mode) {
				switch(mode)
				{
				case CompareMode::NEVER:
					return D3D12_COMPARISON_FUNC_NEVER;
				case CompareMode::LESS:
					return D3D12_COMPARISON_FUNC_LESS;
				case CompareMode::EQUAL:
					return D3D12_COMPARISON_FUNC_EQUAL;
				case CompareMode::LESS_EQUAL:
					return D3D12_COMPARISON_FUNC_LESS_EQUAL;
				case CompareMode::GREATER:
					return D3D12_COMPARISON_FUNC_GREATER;
				case CompareMode::NOT_EQUAL:
					return D3D12_COMPARISON_FUNC_NOT_EQUAL;
				case CompareMode::GREATER_EQUAL:
					return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
				case CompareMode::ALWAYS:
					return D3D12_COMPARISON_FUNC_ALWAYS;
				default:
					DBG_BREAK;
					return D3D12_COMPARISON_FUNC_NEVER;
				}
			};

			auto GetStencilFaceState = [&GetCompareMode](StencilFaceState stencilFaceState) {
				auto GetStencilOp = [](StencilFunc func) {
					switch(func)
					{
					case StencilFunc::KEEP:
						return D3D12_STENCIL_OP_KEEP;
					case StencilFunc::ZERO:
						return D3D12_STENCIL_OP_ZERO;
					case StencilFunc::REPLACE:
						return D3D12_STENCIL_OP_REPLACE;
					case StencilFunc::INCR:
						return D3D12_STENCIL_OP_INCR_SAT;
					case StencilFunc::INCR_WRAP:
						return D3D12_STENCIL_OP_INCR;
					case StencilFunc::DECR:
						return D3D12_STENCIL_OP_DECR_SAT;
					case StencilFunc::DECR_WRAP:
						return D3D12_STENCIL_OP_DECR;
					case StencilFunc::INVERT:
						return D3D12_STENCIL_OP_INVERT;
					default:
						DBG_BREAK;
						return D3D12_STENCIL_OP_KEEP;
					}
				};

				D3D12_DEPTH_STENCILOP_DESC desc;
				desc.StencilFailOp = GetStencilOp(stencilFaceState.fail_);
				desc.StencilDepthFailOp = GetStencilOp(stencilFaceState.depthFail_);
				desc.StencilPassOp = GetStencilOp(stencilFaceState.pass_);
				desc.StencilFunc = GetCompareMode(stencilFaceState.func_);

				return desc;
			};

			D3D12_DEPTH_STENCIL_DESC desc;

			desc.DepthEnable = renderState.depthEnable_;
			desc.DepthWriteMask =
			    renderState.depthWriteMask_ ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = GetCompareMode(renderState.depthFunc_);
			desc.StencilEnable = renderState.stencilEnable_;
			desc.StencilReadMask = renderState.stencilRead_;
			desc.StencilWriteMask = renderState.stencilWrite_;
			desc.BackFace = GetStencilFaceState(renderState.stencilBack_);
			desc.FrontFace = GetStencilFaceState(renderState.stencilFront_);

			return desc;
		};

		auto GetTopologyType = [](TopologyType type) {
			switch(type)
			{
			case TopologyType::POINT:
				return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			case TopologyType::LINE:
				return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			case TopologyType::TRIANGLE:
				return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			case TopologyType::PATCH:
				return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
			default:
				DBG_BREAK;
				return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			}
		};

		auto GetSemanticName = [](VertexUsage usage) {
			switch(usage)
			{
			case VertexUsage::POSITION:
				return "POSITION";
			case VertexUsage::BLENDWEIGHTS:
				return "BLENDWEIGHTS";
			case VertexUsage::BLENDINDICES:
				return "BLENDINDICES";
			case VertexUsage::NORMAL:
				return "NORMAL";
			case VertexUsage::TEXCOORD:
				return "TEXCOORD";
			case VertexUsage::TANGENT:
				return "TANGENT";
			case VertexUsage::BINORMAL:
				return "BINORMAL";
			case VertexUsage::COLOR:
				return "COLOR";
			default:
				DBG_BREAK;
				return "";
			}
		};

		gpsDesc.VS = GetShaderBytecode(ShaderType::VERTEX);
		gpsDesc.GS = GetShaderBytecode(ShaderType::GEOMETRY);
		gpsDesc.HS = GetShaderBytecode(ShaderType::HULL);
		gpsDesc.DS = GetShaderBytecode(ShaderType::DOMAIN);
		gpsDesc.PS = GetShaderBytecode(ShaderType::PIXEL);

		gpsDesc.NodeMask = 0x0;

		gpsDesc.NumRenderTargets = desc.renderState_.numRenderTargets_;
		gpsDesc.BlendState.AlphaToCoverageEnable = FALSE;
		gpsDesc.BlendState.IndependentBlendEnable = TRUE;
		for(i32 i = 0; i < MAX_BOUND_RTVS; ++i)
			gpsDesc.BlendState.RenderTarget[i] = GetBlendState(desc.renderState_.blendStates_[i]);

		gpsDesc.RasterizerState = GetRasterizerState(desc.renderState_);
		gpsDesc.DepthStencilState = GetDepthStencilState(desc.renderState_);

		gpsDesc.PrimitiveTopologyType = GetTopologyType(desc.topology_);

		Core::Array<D3D12_INPUT_ELEMENT_DESC, 16> elementDesc;
		gpsDesc.InputLayout.NumElements = desc.numVertexElements_;
		gpsDesc.InputLayout.pInputElementDescs = elementDesc.data();
		for(i32 i = 0; i < desc.numVertexElements_; ++i)
		{
			elementDesc[i].SemanticName = GetSemanticName(desc.vertexElements_[i].usage_);
			elementDesc[i].SemanticIndex = desc.vertexElements_[i].usageIdx_;
			elementDesc[i].Format = GetFormat(desc.vertexElements_[i].format_);
			elementDesc[i].InputSlot = desc.vertexElements_[i].streamIdx_;
			elementDesc[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; // TODO: expose outside of here.
			elementDesc[i].InstanceDataStepRate = 0;                                    // TODO: expose outside of here.
		}
		gps.stencilRef_ = desc.renderState_.stencilRef_;

		ErrorCode retVal = device_->CreateGraphicsPipelineState(gps, gpsDesc, debugName);
		if(retVal != ErrorCode::OK)
			return retVal;

		Core::ScopedMutex lock(resourceMutex_);
		graphicsPipelineStates_[handle.GetIndex()] = gps;
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::CreateComputePipelineState(
	    Handle handle, const ComputePipelineStateDesc& desc, const char* debugName)
	{
		D3D12ComputePipelineState cps;
		D3D12_COMPUTE_PIPELINE_STATE_DESC cpsDesc = {};

		const auto& shader = shaders_[desc.shader_.GetIndex()];
		cpsDesc.CS.BytecodeLength = shader.byteCodeSize_;
		cpsDesc.CS.pShaderBytecode = shader.byteCode_;
		cpsDesc.NodeMask = 0x0;

		ErrorCode retVal = device_->CreateComputePipelineState(cps, cpsDesc, debugName);
		if(retVal != ErrorCode::OK)
			return retVal;

		Core::ScopedMutex lock(resourceMutex_);
		computePipelineStates_[handle.GetIndex()] = cps;
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::CreatePipelineBindingSet(
	    Handle handle, const PipelineBindingSetDesc& desc, const char* debugName)
	{
		Core::ScopedMutex lock(resourceMutex_);
		D3D12PipelineBindingSet pbs;

		// Grab all resources to create pipeline binding set with.
		Core::Array<ID3D12Resource*, MAX_SRV_BINDINGS> srvResources;
		Core::Array<D3D12_SHADER_RESOURCE_VIEW_DESC, MAX_UAV_BINDINGS> srvs;
		Core::Array<ID3D12Resource*, MAX_SRV_BINDINGS> uavResources;
		Core::Array<D3D12_UNORDERED_ACCESS_VIEW_DESC, MAX_UAV_BINDINGS> uavs;
		Core::Array<D3D12_CONSTANT_BUFFER_VIEW_DESC, MAX_CBV_BINDINGS> cbvs;
		Core::Array<D3D12_SAMPLER_DESC, MAX_SAMPLER_BINDINGS> samplers;

		memset(srvResources.data(), 0, sizeof(srvResources));
		memset(srvs.data(), 0, sizeof(srvs));
		memset(uavResources.data(), 0, sizeof(uavResources));
		memset(uavs.data(), 0, sizeof(uavs));
		memset(cbvs.data(), 0, sizeof(cbvs));
		memset(samplers.data(), 0, sizeof(samplers));

		for(i32 i = 0; i < desc.numSRVs_; ++i)
		{
			auto srvHandle = desc.srvs_[i].resource_;
			DBG_ASSERT(srvHandle.GetType() == ResourceType::BUFFER || srvHandle.GetType() == ResourceType::TEXTURE);
			DBG_ASSERT(srvHandle);

			if(srvHandle.GetType() == ResourceType::BUFFER)
			{
				srvResources[i] = bufferResources_[srvHandle.GetIndex()].resource_.Get();
			}
			else if(srvHandle.GetType() == ResourceType::TEXTURE)
			{
				srvResources[i] = textureResources_[srvHandle.GetIndex()].resource_.Get();
			}
			
			const auto& srv = desc.srvs_[i];
			srvs[i].Format = GetFormat(srv.format_);
			srvs[i].ViewDimension = GetSRVDimension(srv.dimension_);
			switch(srv.dimension_)
			{
			case ViewDimension::BUFFER:
				srvs[i].Buffer.FirstElement = srv.mostDetailedMip_FirstElement_;
				srvs[i].Buffer.NumElements = srv.mipLevels_NumElements_;
				srvs[i].Buffer.StructureByteStride = 0;
				srvs[i].Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
				break;
			case ViewDimension::TEX1D:
				srvs[i].Texture1D.MostDetailedMip = srv.mostDetailedMip_FirstElement_;
				srvs[i].Texture1D.MipLevels = srv.mipLevels_NumElements_;
				srvs[i].Texture1D.ResourceMinLODClamp = srv.resourceMinLODClamp_;
				break;
			case ViewDimension::TEX1D_ARRAY:
				srvs[i].Texture1DArray.MostDetailedMip = srv.mostDetailedMip_FirstElement_;
				srvs[i].Texture1DArray.MipLevels = srv.mipLevels_NumElements_;
				srvs[i].Texture1DArray.ArraySize = srv.arraySize_;
				srvs[i].Texture1DArray.FirstArraySlice = srv.firstArraySlice_;
				srvs[i].Texture1DArray.ResourceMinLODClamp = srv.resourceMinLODClamp_;
				break;
			case ViewDimension::TEX2D:
				srvs[i].Texture2D.MostDetailedMip = srv.mostDetailedMip_FirstElement_;
				srvs[i].Texture2D.MipLevels = srv.mipLevels_NumElements_;
				srvs[i].Texture2D.PlaneSlice = srv.planeSlice_;
				srvs[i].Texture2D.ResourceMinLODClamp = srv.resourceMinLODClamp_;
				break;
			case ViewDimension::TEX2D_ARRAY:
				srvs[i].Texture2DArray.MostDetailedMip = srv.mostDetailedMip_FirstElement_;
				srvs[i].Texture2DArray.MipLevels = srv.mipLevels_NumElements_;
				srvs[i].Texture2DArray.ArraySize = srv.arraySize_;
				srvs[i].Texture2DArray.FirstArraySlice = srv.firstArraySlice_;
				srvs[i].Texture2DArray.PlaneSlice = srv.planeSlice_;
				srvs[i].Texture2DArray.ResourceMinLODClamp = srv.resourceMinLODClamp_;
				break;
			case ViewDimension::TEX3D:
				srvs[i].Texture3D.MostDetailedMip = srv.mostDetailedMip_FirstElement_;
				srvs[i].Texture3D.MipLevels = srv.mipLevels_NumElements_;
				srvs[i].Texture3D.ResourceMinLODClamp = srv.resourceMinLODClamp_;
				break;
			case ViewDimension::TEXCUBE:
				srvs[i].TextureCube.MostDetailedMip = srv.mostDetailedMip_FirstElement_;
				srvs[i].TextureCube.MipLevels = srv.mipLevels_NumElements_;
				srvs[i].TextureCube.ResourceMinLODClamp = srv.resourceMinLODClamp_;
				break;
			case ViewDimension::TEXCUBE_ARRAY:
				srvs[i].TextureCubeArray.MostDetailedMip = srv.mostDetailedMip_FirstElement_;
				srvs[i].TextureCubeArray.MipLevels = srv.mipLevels_NumElements_;
				srvs[i].TextureCubeArray.NumCubes = srv.arraySize_;
				srvs[i].TextureCubeArray.First2DArrayFace = srv.firstArraySlice_;
				srvs[i].TextureCubeArray.ResourceMinLODClamp = srv.resourceMinLODClamp_;
				break;
			default:
				DBG_BREAK;
				return ErrorCode::FAIL;
				break;
			}
		}

		for(i32 i = 0; i < desc.numUAVs_; ++i)
		{
			auto uavHandle = desc.uavs_[i].resource_;
			DBG_ASSERT(uavHandle.GetType() == ResourceType::BUFFER || uavHandle.GetType() == ResourceType::TEXTURE);
			DBG_ASSERT(uavHandle);

			if(uavHandle.GetType() == ResourceType::BUFFER)
			{
				uavResources[i] = bufferResources_[uavHandle.GetIndex()].resource_.Get();
			}
			else if(uavHandle.GetType() == ResourceType::TEXTURE)
			{
				uavResources[i] = textureResources_[uavHandle.GetIndex()].resource_.Get();
			}
			
			const auto& uav = desc.uavs_[i];
			uavs[i].Format = GetFormat(uav.format_);
			uavs[i].ViewDimension = GetUAVDimension(uav.dimension_);
			switch(uav.dimension_)
			{
			case ViewDimension::BUFFER:
				uavs[i].Buffer.FirstElement = uav.mipSlice_FirstElement_;
				uavs[i].Buffer.NumElements = uav.firstArraySlice_FirstWSlice_NumElements_;
				uavs[i].Buffer.StructureByteStride = 0;
				break;
			case ViewDimension::TEX1D:
				uavs[i].Texture1D.MipSlice = uav.mipSlice_FirstElement_;
				break;
			case ViewDimension::TEX1D_ARRAY:
				uavs[i].Texture1DArray.MipSlice = uav.mipSlice_FirstElement_;
				uavs[i].Texture1DArray.ArraySize = uav.arraySize_PlaneSlice_WSize_;
				uavs[i].Texture1DArray.FirstArraySlice = uav.firstArraySlice_FirstWSlice_NumElements_;
				break;
			case ViewDimension::TEX2D:
				uavs[i].Texture2D.MipSlice = uav.mipSlice_FirstElement_;
				uavs[i].Texture2D.PlaneSlice = uav.arraySize_PlaneSlice_WSize_;
				break;
			case ViewDimension::TEX2D_ARRAY:
				uavs[i].Texture2DArray.MipSlice = uav.mipSlice_FirstElement_;
				uavs[i].Texture2DArray.ArraySize = uav.arraySize_PlaneSlice_WSize_;
				uavs[i].Texture2DArray.FirstArraySlice = uav.firstArraySlice_FirstWSlice_NumElements_;
				uavs[i].Texture2DArray.PlaneSlice = uav.arraySize_PlaneSlice_WSize_;
				break;
			case ViewDimension::TEX3D:
				uavs[i].Texture3D.MipSlice = uav.mipSlice_FirstElement_;
				uavs[i].Texture3D.FirstWSlice = uav.firstArraySlice_FirstWSlice_NumElements_;
				uavs[i].Texture3D.WSize = uav.arraySize_PlaneSlice_WSize_;
				break;
			default:
				DBG_BREAK;
				return ErrorCode::FAIL;
				break;
			}
		}
		
		for(i32 i = 0; i < desc.numCBVs_; ++i)
		{
			auto cbvHandle = desc.cbvs_[i].resource_;
			DBG_ASSERT(cbvHandle.GetType() == ResourceType::BUFFER);
			DBG_ASSERT(cbvHandle);

			const auto& cbv = desc.cbvs_[i];

			cbvs[i].BufferLocation = bufferResources_[cbvHandle.GetIndex()].resource_->GetGPUVirtualAddress() + cbv.offset_;
			cbvs[i].SizeInBytes = cbv.size_;
		}

		for(i32 i = 0; i < desc.numSamplers_; ++i)
		{
			auto samplerHandle = desc.samplers_[i].resource_;
			DBG_ASSERT(samplerHandle);
			samplers[i] = samplerStates_[samplerHandle.GetIndex()].desc_;
		}

		ErrorCode retVal = device_->CreatePipelineBindingSet(pbs, desc, debugName);
		if(retVal != ErrorCode::OK)
			return retVal;

		pipelineBindingSets_[handle.GetIndex()] = pbs;
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::CreateDrawBindingSet(Handle handle, const DrawBindingSetDesc& desc, const char* debugName)
	{
		D3D12DrawBindingSet dbs;


		//
		Core::ScopedMutex lock(resourceMutex_);
		drawBindingSets_[handle.GetIndex()] = dbs;
		return ErrorCode::UNIMPLEMENTED;
	}

	ErrorCode D3D12Backend::CreateFrameBindingSet(Handle handle, const FrameBindingSetDesc& desc, const char* debugName)
	{
		//
		return ErrorCode::UNIMPLEMENTED;
	}

	ErrorCode D3D12Backend::CreateCommandList(Handle handle, const char* debugName)
	{
		D3D12CommandList* commandList = new D3D12CommandList(*device_, 0x0, D3D12_COMMAND_LIST_TYPE_DIRECT);
		Core::ScopedMutex lock(resourceMutex_);
		commandLists_[handle.GetIndex()] = commandList;
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::CreateFence(Handle handle, const char* debugName)
	{
		//
		return ErrorCode::UNIMPLEMENTED;
	}

	ErrorCode D3D12Backend::DestroyResource(Handle handle)
	{
		switch(handle.GetType())
		{
		case ResourceType::SWAP_CHAIN:
			swapchainResources_[handle.GetIndex()] = D3D12SwapChain();
			break;
		case ResourceType::BUFFER:
			bufferResources_[handle.GetIndex()] = D3D12Resource();
			break;
		case ResourceType::TEXTURE:
			textureResources_[handle.GetIndex()] = D3D12Resource();
			break;
		case ResourceType::SHADER:
			delete[] shaders_[handle.GetIndex()].byteCode_;
			shaders_[handle.GetIndex()] = D3D12Shader();
			break;
		case ResourceType::SAMPLER_STATE:
			samplerStates_[handle.GetIndex()] = D3D12SamplerState();
			break;
		case ResourceType::GRAPHICS_PIPELINE_STATE:
			graphicsPipelineStates_[handle.GetIndex()] = D3D12GraphicsPipelineState();
			break;
		case ResourceType::COMPUTE_PIPELINE_STATE:
			computePipelineStates_[handle.GetIndex()] = D3D12ComputePipelineState();
			break;
		case ResourceType::PIPELINE_BINDING_SET:
			device_->DestroyPipelineBindingSet(pipelineBindingSets_[handle.GetIndex()]);
			pipelineBindingSets_[handle.GetIndex()] = D3D12PipelineBindingSet();
			break;
		case ResourceType::DRAW_BINDING_SET:
			drawBindingSets_[handle.GetIndex()] = D3D12DrawBindingSet();
			break;
		case ResourceType::COMMAND_LIST:
			delete commandLists_[handle.GetIndex()];
			commandLists_[handle.GetIndex()] = nullptr;
			break;
		}
		//
		return ErrorCode::UNIMPLEMENTED;
	}

	ErrorCode D3D12Backend::CompileCommandList(Handle handle, const CommandList& commandList)
	{
		DBG_ASSERT(handle.GetIndex() < commandLists_.size());

		// Lock resources during command list compilation.
		Core::ScopedMutex resourceLock(resourceMutex_);

		D3D12CommandList* outCommandList = commandLists_[handle.GetIndex()];
		(void)outCommandList;

		for(const auto* command : commandList)
		{
			switch(command->type_)
			{
			case CommandType::DRAW:
			case CommandType::DRAW_INDIRECT:
			case CommandType::DISPATCH:
			case CommandType::DISPATCH_INDIRECT:
			case CommandType::CLEAR_RTV:
			case CommandType::CLEAR_DSV:
			case CommandType::CLEAR_UAV:
			case CommandType::UPDATE_BUFFER:
			case CommandType::UPDATE_TEXTURE_SUBRESOURCE:
			case CommandType::COPY_BUFFER:
			case CommandType::COPY_TEXTURE_SUBRESOURCE:
			case CommandType::UPDATE_RTV:
			case CommandType::UPDATE_DSV:
			case CommandType::UPDATE_SRV:
			case CommandType::UPDATE_UAV:
			case CommandType::UPDATE_CBV:
				break;
			default:
				DBG_BREAK;
			}
		}

		return ErrorCode::UNIMPLEMENTED;
	}

	ErrorCode D3D12Backend::SubmitCommandList(Handle handle)
	{
		D3D12CommandList* commandList = commandLists_[handle.GetIndex()];
		DBG_ASSERT(commandList);
		return device_->SubmitCommandList(*commandList);
	}


} // namespace GPU
