#include "gpu_d3d12/d3d12_backend.h"
#include "gpu_d3d12/d3d12_device.h"
#include "gpu_d3d12/d3d12_linear_heap_allocator.h"
#include "gpu_d3d12/d3d12_linear_descriptor_allocator.h"
#include "core/debug.h"
#include "core/string.h"

#include <utility>

extern "C" {
EXPORT bool GetPlugin(struct Plugin::Plugin* outPlugin, Core::UUID uuid)
{
	bool retVal = false;

	// Fill in base info.
	if(uuid == Plugin::Plugin::GetUUID() || uuid == GPU::BackendPlugin::GetUUID())
	{
		if(outPlugin)
		{
			outPlugin->systemVersion_ = Plugin::PLUGIN_SYSTEM_VERSION;
			outPlugin->pluginVersion_ = GPU::BackendPlugin::PLUGIN_VERSION;
			outPlugin->uuid_ = GPU::BackendPlugin::GetUUID();
			outPlugin->name_ = "D3D12 Backend";
			outPlugin->desc_ = "DirectX 12 backend.";
		}
		retVal = true;
	}

	// Fill in plugin specific.
	if(uuid == GPU::BackendPlugin::GetUUID())
	{
		if(outPlugin)
		{
			auto* plugin = static_cast<GPU::BackendPlugin*>(outPlugin);
			plugin->api_ = "D3D12";
			plugin->CreateBackend = [](const GPU::SetupParams& setupParams) -> GPU::IBackend* {
				return new GPU::D3D12Backend(setupParams);
			};
			plugin->DestroyBackend = [](GPU::IBackend*& backend) {
				delete backend;
				backend = nullptr;
			};
		}
		retVal = true;
	}

	return retVal;
}
}

namespace GPU
{
	D3D12Backend::D3D12Backend(const SetupParams& setupParams)
	    : setupParams_(setupParams)
	{
		auto retVal = LoadLibraries();
		DBG_ASSERT(retVal == ErrorCode::OK);
		UINT flags = 0;

#if !defined(_RELEASE)
		HRESULT hr = S_OK;
		// Setup debug interfaces.
		if(Core::ContainsAnyFlags(
		       setupParams.debugFlags_, DebugFlags::DEBUG_RUNTIME | GPU::DebugFlags::GPU_BASED_VALIDATION))
		{
			if(DXGIGetDebugInterface1Fn)
			{
				hr = DXGIGetDebugInterface1Fn(0, IID_IDXGIDebug, (void**)dxgiDebug_.GetAddressOf());
				ComPtr<IDXGIInfoQueue> infoQueue;
				hr = dxgiDebug_.As(&infoQueue);
				if(SUCCEEDED(hr))
				{
					infoQueue->SetMuteDebugOutput(DXGI_DEBUG_ALL, FALSE);
					CHECK_D3D(infoQueue->SetBreakOnSeverity(
					    DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE));
					CHECK_D3D(
					    infoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE));
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

					hr = d3dDebug_.Get()->QueryInterface(IID_ID3D12Debug1, (void**)d3dDebug1_.GetAddressOf());
					if(SUCCEEDED(hr))
					{
						if(Core::ContainsAnyFlags(setupParams.debugFlags_, DebugFlags::GPU_BASED_VALIDATION))
						{
							d3dDebug1_->SetEnableGPUBasedValidation(TRUE);
						}
					}
				}
			}
		}

		flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

		// Vendor specific extensions.
		AGSConfiguration agsConfig;
		agsConfig.allocCallback = nullptr;
		agsConfig.freeCallback = nullptr;
		agsConfig.crossfireMode = AGS_CROSSFIRE_MODE_DISABLE;
		if(AGS_SUCCESS == agsInit(&agsContext_, &agsConfig, &agsGPUInfo_))
		{
			Core::Log("AMD AGS Successfully initialized.\n");
			for(i32 i = 0; i < agsGPUInfo_.numDevices; ++i)
			{
				const auto& device = agsGPUInfo_.devices[i];
				Core::Log(" - %s (%u CUs, %u MHz, %uMB)\n", device.adapterString, device.numCUs, device.coreClock,
				    (u32)(device.localMemoryInBytes / (1024 * 1024)));
			}
		}

		DXGICreateDXGIFactory2Fn(flags, IID_IDXGIFactory4, (void**)dxgiFactory_.ReleaseAndGetAddressOf());
	}

	D3D12Backend::~D3D12Backend()
	{
		delete device_;
		dxgiAdapters_.clear();
		adapterInfos_.clear();
		dxgiFactory_ = nullptr;

#if !defined(_RELEASE)
		if(dxgiDebug_)
		{
			dxgiDebug_->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		}
#endif

		if(agsContext_)
			agsDeInit(agsContext_);
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
		device_ = new D3D12Device(*this, setupParams_, dxgiFactory_.Get(), dxgiAdapters_[adapterIdx].Get());
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

		Core::ScopedWriteLock lock(resLock_);
		swapchainResources_[handle.GetIndex()] = swapChain;
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::CreateBuffer(
	    Handle handle, const BufferDesc& desc, const void* initialData, const char* debugName)
	{
		D3D12Buffer buffer;
		buffer.desc_ = desc;
		ErrorCode retVal = device_->CreateBuffer(buffer, desc, initialData, debugName);
		if(retVal != ErrorCode::OK)
			return retVal;

		Core::ScopedWriteLock lock(resLock_);
		bufferResources_[handle.GetIndex()] = buffer;
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::CreateTexture(
	    Handle handle, const TextureDesc& desc, const TextureSubResourceData* initialData, const char* debugName)
	{
		D3D12Texture texture;
		texture.desc_ = desc;
		ErrorCode retVal = device_->CreateTexture(texture, desc, initialData, debugName);
		if(retVal != ErrorCode::OK)
			return retVal;

		Core::ScopedWriteLock lock(resLock_);
		textureResources_[handle.GetIndex()] = texture;
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::CreateShader(Handle handle, const ShaderDesc& desc, const char* debugName)
	{
		D3D12Shader shader;
		shader.byteCode_ = new u8[desc.dataSize_];
		shader.byteCodeSize_ = desc.dataSize_;
		memcpy(shader.byteCode_, desc.data_, desc.dataSize_);

		Core::ScopedWriteLock lock(resLock_);
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
					DBG_ASSERT(false);
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
					DBG_ASSERT(false);
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
					DBG_ASSERT(false);
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
					DBG_ASSERT(false);
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
					DBG_ASSERT(false);
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
						DBG_ASSERT(false);
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
				DBG_ASSERT(false);
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
				DBG_ASSERT(false);
				return "";
			}
		};

		{
			Core::ScopedReadLock lock(resLock_);
			gpsDesc.VS = GetShaderBytecode(ShaderType::VS);
			gpsDesc.HS = GetShaderBytecode(ShaderType::HS);
			gpsDesc.DS = GetShaderBytecode(ShaderType::DS);
			gpsDesc.GS = GetShaderBytecode(ShaderType::GS);
			gpsDesc.PS = GetShaderBytecode(ShaderType::PS);
		}

		gpsDesc.NodeMask = 0x0;

		gpsDesc.NumRenderTargets = desc.numRTs_;
		gpsDesc.BlendState.AlphaToCoverageEnable = FALSE;
		gpsDesc.BlendState.IndependentBlendEnable = TRUE;
		for(i32 i = 0; i < MAX_BOUND_RTVS; ++i)
		{
			gpsDesc.BlendState.RenderTarget[i] = GetBlendState(desc.renderState_.blendStates_[i]);
			gpsDesc.RTVFormats[i] = i < desc.numRTs_ ? GetFormat(desc.rtvFormats_[i]) : DXGI_FORMAT_UNKNOWN;
		}
		gpsDesc.DSVFormat = GetFormat(desc.dsvFormat_);

		gpsDesc.SampleDesc.Count = 1;
		gpsDesc.SampleDesc.Quality = 0;

		gpsDesc.RasterizerState = GetRasterizerState(desc.renderState_);
		gpsDesc.DepthStencilState = GetDepthStencilState(desc.renderState_);

		gpsDesc.PrimitiveTopologyType = GetTopologyType(desc.topology_);

		gpsDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

		Core::Array<D3D12_INPUT_ELEMENT_DESC, 16> elementDesc = {};
		gpsDesc.InputLayout.NumElements = desc.numVertexElements_;
		gpsDesc.InputLayout.pInputElementDescs = elementDesc.data();
		for(i32 i = 0; i < desc.numVertexElements_; ++i)
		{
			elementDesc[i].SemanticName = GetSemanticName(desc.vertexElements_[i].usage_);
			elementDesc[i].SemanticIndex = desc.vertexElements_[i].usageIdx_;
			elementDesc[i].Format = GetFormat(desc.vertexElements_[i].format_);
			elementDesc[i].AlignedByteOffset = desc.vertexElements_[i].offset_;
			elementDesc[i].InputSlot = desc.vertexElements_[i].streamIdx_;
			elementDesc[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA; // TODO: expose outside of here.
			elementDesc[i].InstanceDataStepRate = 0;                                    // TODO: expose outside of here.
		}
		gps.stencilRef_ = desc.renderState_.stencilRef_;

		ErrorCode retVal = device_->CreateGraphicsPipelineState(gps, gpsDesc, debugName);
		if(retVal != ErrorCode::OK)
			return retVal;

		Core::ScopedWriteLock lock(resLock_);
		graphicsPipelineStates_[handle.GetIndex()] = gps;
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::CreateComputePipelineState(
	    Handle handle, const ComputePipelineStateDesc& desc, const char* debugName)
	{
		D3D12ComputePipelineState cps;
		D3D12_COMPUTE_PIPELINE_STATE_DESC cpsDesc = {};

		{
			Core::ScopedReadLock lock(resLock_);
			const auto& shader = shaders_[desc.shader_.GetIndex()];
			cpsDesc.CS.BytecodeLength = shader.byteCodeSize_;
			cpsDesc.CS.pShaderBytecode = shader.byteCode_;
			cpsDesc.NodeMask = 0x0;
		}

		ErrorCode retVal = device_->CreateComputePipelineState(cps, cpsDesc, debugName);
		if(retVal != ErrorCode::OK)
			return retVal;

		Core::ScopedWriteLock lock(resLock_);
		computePipelineStates_[handle.GetIndex()] = cps;
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::CreatePipelineBindingSet(
	    Handle handle, const PipelineBindingSetDesc& desc, const char* debugName)
	{
		D3D12PipelineBindingSet pbs;

		ErrorCode retVal = ErrorCode::FAIL;
		RETURN_ON_ERROR(retVal = device_->CreatePipelineBindingSet(pbs, desc, debugName));

		Core::ScopedWriteLock lock(resLock_);
		pipelineBindingSets_[handle.GetIndex()] = pbs;
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::CreateDrawBindingSet(Handle handle, const DrawBindingSetDesc& desc, const char* debugName)
	{
		D3D12DrawBindingSet dbs;
		memset(&dbs.ib_, 0, sizeof(dbs.ib_));
		memset(dbs.vbs_.data(), 0, sizeof(dbs.vbs_));

		dbs.desc_ = desc;

		{
			Core::ScopedReadLock lock(resLock_);
			if(desc.ib_.resource_)
			{
				D3D12Buffer* buffer = GetD3D12Buffer(desc.ib_.resource_);
				DBG_ASSERT(buffer);

				DBG_ASSERT(Core::ContainsAllFlags(buffer->supportedStates_, D3D12_RESOURCE_STATE_INDEX_BUFFER));
				dbs.ibResource_ = buffer;

				dbs.ib_.BufferLocation = buffer->resource_->GetGPUVirtualAddress() + desc.ib_.offset_;
				dbs.ib_.SizeInBytes = Core::PotRoundUp(desc.ib_.size_, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
				switch(desc.ib_.stride_)
				{
				case 2:
					dbs.ib_.Format = DXGI_FORMAT_R16_UINT;
					break;
				case 4:
					dbs.ib_.Format = DXGI_FORMAT_R32_UINT;
					break;
				default:
					return ErrorCode::FAIL;
					break;
				}
			}

			i32 idx = 0;
			for(const auto& vb : desc.vbs_)
			{
				if(vb.resource_)
				{
					D3D12Buffer* buffer = GetD3D12Buffer(vb.resource_);
					DBG_ASSERT(buffer);
					DBG_ASSERT(Core::ContainsAllFlags(
					    buffer->supportedStates_, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
					dbs.vbResources_[idx] = buffer;

					dbs.vbs_[idx].BufferLocation = buffer->resource_->GetGPUVirtualAddress() + vb.offset_;
					dbs.vbs_[idx].SizeInBytes = vb.size_;
					dbs.vbs_[idx].StrideInBytes = vb.stride_;
				}
				++idx;
			}
		}

		//
		Core::ScopedWriteLock lock(resLock_);
		drawBindingSets_[handle.GetIndex()] = dbs;
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::CreateFrameBindingSet(Handle handle, const FrameBindingSetDesc& desc, const char* debugName)
	{
		D3D12FrameBindingSet fbs;

		fbs.desc_ = desc;
		{
			Core::ScopedReadLock lock(resLock_);
			Core::Vector<D3D12_RENDER_TARGET_VIEW_DESC> rtvDescs;
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;

			// Check if we're using a swapchain.
			{
				const auto& rtv = desc.rtvs_[0];
				Handle resource = rtv.resource_;
				if(resource.GetType() == ResourceType::SWAP_CHAIN)
				{
					auto& swapChain = swapchainResources_[resource.GetIndex()];
					fbs.numBuffers_ = swapChain.textures_.size();
					fbs.swapChain_ = &swapChain;
				}
			}

			// Resize to support number of buffers.
			rtvDescs.resize(MAX_BOUND_RTVS * fbs.numBuffers_);
			memset(rtvDescs.data(), 0, sizeof(D3D12_RENDER_TARGET_VIEW_DESC) * rtvDescs.size());
			memset(&dsvDesc, 0, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));
			fbs.rtvResources_.resize(MAX_BOUND_RTVS * fbs.numBuffers_);

			for(i32 bufferIdx = 0; bufferIdx < fbs.numBuffers_; ++bufferIdx)
			{
				for(i32 rtvIdx = 0; rtvIdx < MAX_BOUND_RTVS; ++rtvIdx)
				{
					auto& rtvDesc = rtvDescs[rtvIdx + bufferIdx * MAX_BOUND_RTVS];
					D3D12SubresourceRange& rtvResource = fbs.rtvResources_[rtvIdx + bufferIdx * MAX_BOUND_RTVS];
					const auto& rtv = desc.rtvs_[rtvIdx];
					Handle resource = rtv.resource_;
					if(resource)
					{
						// Only first element can be a swap chain, and only one RTV can be set if using a swap chain.
						DBG_ASSERT(rtvIdx == 0 || resource.GetType() == ResourceType::TEXTURE);
						DBG_ASSERT(rtvIdx == 0 || !fbs.swapChain_);

						// No holes allowed.
						if(bufferIdx == 0)
							if(rtvIdx != fbs.numRTs_++)
								return ErrorCode::FAIL;

						D3D12Texture* texture = GetD3D12Texture(resource, bufferIdx);
						DBG_ASSERT(
						    Core::ContainsAllFlags(texture->supportedStates_, D3D12_RESOURCE_STATE_RENDER_TARGET));
						rtvResource.resource_ = texture;
						rtvResource.firstSubRsc_ = 0;
						rtvResource.numSubRsc_ = texture->numSubResources_;

						rtvDesc.Format = GetFormat(rtv.format_);
						rtvDesc.ViewDimension = GetRTVDimension(rtv.dimension_);
						switch(rtv.dimension_)
						{
						case ViewDimension::BUFFER:
							return ErrorCode::UNSUPPORTED;
							break;
						case ViewDimension::TEX1D:
							rtvDesc.Texture1D.MipSlice = rtv.mipSlice_;
							break;
						case ViewDimension::TEX1D_ARRAY:
							rtvDesc.Texture1DArray.ArraySize = rtv.arraySize_;
							rtvDesc.Texture1DArray.FirstArraySlice = rtv.firstArraySlice_;
							rtvDesc.Texture1DArray.MipSlice = rtv.mipSlice_;
							break;
						case ViewDimension::TEX2D:
							rtvDesc.Texture2D.MipSlice = rtv.mipSlice_;
							rtvDesc.Texture2D.PlaneSlice = rtv.planeSlice_FirstWSlice_;
							break;
						case ViewDimension::TEX2D_ARRAY:
							rtvDesc.Texture2DArray.MipSlice = rtv.mipSlice_;
							rtvDesc.Texture2DArray.FirstArraySlice = rtv.firstArraySlice_;
							rtvDesc.Texture2DArray.ArraySize = rtv.arraySize_;
							rtvDesc.Texture2DArray.PlaneSlice = rtv.planeSlice_FirstWSlice_;
							break;
						case ViewDimension::TEX3D:
							rtvDesc.Texture3D.FirstWSlice = rtv.planeSlice_FirstWSlice_;
							rtvDesc.Texture3D.MipSlice = rtv.mipSlice_;
							rtvDesc.Texture3D.WSize = rtv.wSize_;
							break;
						default:
							DBG_ASSERT(false);
							return ErrorCode::FAIL;
							break;
						}
					}
				}
			}

			{
				const auto& dsv = desc.dsv_;
				Handle resource = dsv.resource_;
				if(resource)
				{
					D3D12SubresourceRange& dsvResource = fbs.dsvResource_;
					DBG_ASSERT(resource.GetType() == ResourceType::TEXTURE);
					D3D12Texture* texture = GetD3D12Texture(resource);
					DBG_ASSERT(Core::ContainsAnyFlags(
					    texture->supportedStates_, D3D12_RESOURCE_STATE_DEPTH_WRITE | D3D12_RESOURCE_STATE_DEPTH_READ));
					dsvResource.resource_ = texture;
					dsvResource.firstSubRsc_ = 0;
					dsvResource.numSubRsc_ = texture->numSubResources_;

					dsvDesc.Format = GetFormat(dsv.format_);
					dsvDesc.ViewDimension = GetDSVDimension(dsv.dimension_);
					switch(dsv.dimension_)
					{
					case ViewDimension::BUFFER:
						return ErrorCode::UNSUPPORTED;
						break;
					case ViewDimension::TEX1D:
						dsvDesc.Texture1D.MipSlice = dsv.mipSlice_;
						break;
					case ViewDimension::TEX1D_ARRAY:
						dsvDesc.Texture1DArray.ArraySize = dsv.arraySize_;
						dsvDesc.Texture1DArray.FirstArraySlice = dsv.firstArraySlice_;
						dsvDesc.Texture1DArray.MipSlice = dsv.mipSlice_;
						break;
					case ViewDimension::TEX2D:
						dsvDesc.Texture2D.MipSlice = dsv.mipSlice_;
						break;
					case ViewDimension::TEX2D_ARRAY:
						dsvDesc.Texture2DArray.MipSlice = dsv.mipSlice_;
						dsvDesc.Texture2DArray.FirstArraySlice = dsv.firstArraySlice_;
						dsvDesc.Texture2DArray.ArraySize = dsv.arraySize_;
						break;
					default:
						DBG_ASSERT(false);
						return ErrorCode::FAIL;
						break;
					}
				}
			}

			ErrorCode retVal;
			RETURN_ON_ERROR(retVal = device_->CreateFrameBindingSet(fbs, fbs.desc_, debugName));
			RETURN_ON_ERROR(retVal = device_->UpdateFrameBindingSet(fbs, rtvDescs.data(), &dsvDesc));
		}

		//
		Core::ScopedWriteLock lock(resLock_);
		frameBindingSets_[handle.GetIndex()] = fbs;
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::CreateCommandList(Handle handle, const char* debugName)
	{
		D3D12CommandList* commandList = new D3D12CommandList(*device_, 0x0, D3D12_COMMAND_LIST_TYPE_DIRECT, debugName);
		Core::ScopedWriteLock lock(resLock_);
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
		Core::ScopedWriteLock lock(resLock_);
		switch(handle.GetType())
		{
		case ResourceType::SWAP_CHAIN:
			swapchainResources_[handle.GetIndex()] = D3D12SwapChain();
			break;
		case ResourceType::BUFFER:
			bufferResources_[handle.GetIndex()] = D3D12Buffer();
			break;
		case ResourceType::TEXTURE:
			textureResources_[handle.GetIndex()] = D3D12Texture();
			break;
		case ResourceType::SHADER:
			delete[] shaders_[handle.GetIndex()].byteCode_;
			shaders_[handle.GetIndex()] = D3D12Shader();
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
		case ResourceType::FRAME_BINDING_SET:
			device_->DestroyFrameBindingSet(frameBindingSets_[handle.GetIndex()]);
			frameBindingSets_[handle.GetIndex()] = D3D12FrameBindingSet();
			break;
		case ResourceType::COMMAND_LIST:
			delete commandLists_[handle.GetIndex()];
			commandLists_[handle.GetIndex()] = nullptr;
			break;
		default:
			return ErrorCode::UNIMPLEMENTED;
		}
		//
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::AllocTemporaryPipelineBindingSet(Handle handle, const PipelineBindingSetDesc& desc)
	{
		D3D12PipelineBindingSet pbs;

		if(handle.GetIndex() == 61 || handle.GetIndex() == 62)
		{
			int a = 0;
			++a;
		}

		// Setup descriptors.
		auto& samplerAllocator = device_->GetSamplerDescriptorAllocator();
		auto& cbvSubAllocator = device_->GetCBVSubAllocator();
		auto& srvSubAllocator = device_->GetSRVSubAllocator();
		auto& uavSubAllocator = device_->GetUAVSubAllocator();

		pbs.samplers_ = samplerAllocator.Alloc(desc.numSamplers_, GPU::DescriptorHeapSubType::SAMPLER);
		pbs.cbvs_ = cbvSubAllocator.Alloc(desc.numCBVs_, MAX_CBV_BINDINGS);
		pbs.srvs_ = srvSubAllocator.Alloc(desc.numSRVs_, MAX_SRV_BINDINGS);
		pbs.uavs_ = uavSubAllocator.Alloc(desc.numUAVs_, MAX_UAV_BINDINGS);

		pbs.temporary_ = true;
		pbs.shaderVisible_ = true;

		DBG_ASSERT(pbs.samplers_.size_ >= desc.numSamplers_);
		DBG_ASSERT(pbs.cbvs_.size_ >= desc.numCBVs_);
		DBG_ASSERT(pbs.srvs_.size_ >= desc.numSRVs_);
		DBG_ASSERT(pbs.uavs_.size_ >= desc.numUAVs_);

		pbs.cbvTransitions_.resize(desc.numCBVs_);
		pbs.srvTransitions_.resize(desc.numSRVs_);
		pbs.uavTransitions_.resize(desc.numUAVs_);

		Core::ScopedWriteLock lock(resLock_);
		pipelineBindingSets_[handle.GetIndex()] = pbs;
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::UpdatePipelineBindings(Handle handle, i32 base, Core::ArrayView<const BindingCBV> descs)
	{
		Core::ScopedWriteLock lock(resLock_);
		auto& pbs = pipelineBindingSets_[handle.GetIndex()];

		Core::Array<D3D12_CONSTANT_BUFFER_VIEW_DESC, MAX_CBV_BINDINGS> cbvDescs = {};

		i32 bindingIdx = base;
		for(i32 i = 0; i < descs.size(); ++i, ++bindingIdx)
		{
			auto cbvHandle = descs[i].resource_;
			DBG_ASSERT(cbvHandle.IsValid());
			DBG_ASSERT(cbvHandle.GetType() == ResourceType::BUFFER);

			D3D12Resource* resource = GetD3D12Resource(cbvHandle);
			DBG_ASSERT(resource);

			// Setup transition info.
			pbs.cbvTransitions_[bindingIdx].resource_ = resource;
			pbs.cbvTransitions_[bindingIdx].firstSubRsc_ = 0;
			pbs.cbvTransitions_[bindingIdx].numSubRsc_ = 1;

			// Setup the D3D12 descriptor.
			const auto& cbv = descs[i];
			auto& cbvDesc = cbvDescs[bindingIdx];
			cbvDesc.BufferLocation =
			    bufferResources_[cbvHandle.GetIndex()].resource_->GetGPUVirtualAddress() + cbv.offset_;
			cbvDesc.SizeInBytes = Core::PotRoundUp(cbv.size_, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		}

		return device_->UpdateCBVs(pbs, base, descs.size(), pbs.cbvTransitions_.data() + base, cbvDescs.data() + base);
	}

	ErrorCode D3D12Backend::UpdatePipelineBindings(Handle handle, i32 base, Core::ArrayView<const BindingSRV> descs)
	{
		Core::ScopedWriteLock lock(resLock_);
		auto& pbs = pipelineBindingSets_[handle.GetIndex()];

		Core::Array<D3D12_SHADER_RESOURCE_VIEW_DESC, MAX_SRV_BINDINGS> srvDescs = {};

		i32 bindingIdx = base;
		for(i32 i = 0; i < descs.size(); ++i, ++bindingIdx)
		{
			auto srvHandle = descs[i].resource_;
			DBG_ASSERT(srvHandle.IsValid());
			DBG_ASSERT(srvHandle.GetType() == ResourceType::BUFFER || srvHandle.GetType() == ResourceType::TEXTURE);

			const D3D12Buffer* bufferRes = nullptr;
			const D3D12Texture* textureRes = nullptr;
			if(srvHandle.GetType() == ResourceType::BUFFER)
				bufferRes = &bufferResources_[srvHandle.GetIndex()];
			else
				textureRes = &textureResources_[srvHandle.GetIndex()];

			i32 firstSubRsc = 0;
			i32 numSubRsc = 0;

			const auto& srv = descs[i];

			i32 mipLevels = srv.mipLevels_NumElements_;
			if(mipLevels == -1)
				mipLevels = textureRes->desc_.levels_;
			DBG_ASSERT(mipLevels > 0);

			auto& srvDesc = srvDescs[bindingIdx];

			srvDesc.Format = GetFormat(srv.format_);
			srvDesc.ViewDimension = GetSRVDimension(srv.dimension_);
			srvDesc.Shader4ComponentMapping =
			    D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
			        D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
			        D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
			        D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3);
			switch(srv.dimension_)
			{
			case ViewDimension::BUFFER:
				srvDesc.Buffer.FirstElement = srv.mostDetailedMip_FirstElement_;
				srvDesc.Buffer.NumElements = mipLevels;
				srvDesc.Buffer.StructureByteStride = srv.structureByteStride_;
				if(srv.structureByteStride_ == 0)
					srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
				else
					srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
				firstSubRsc = 0;
				numSubRsc = 1;
				break;
			case ViewDimension::TEX1D:
				srvDesc.Texture1D.MostDetailedMip = srv.mostDetailedMip_FirstElement_;
				srvDesc.Texture1D.MipLevels = mipLevels;
				srvDesc.Texture1D.ResourceMinLODClamp = srv.resourceMinLODClamp_;
				firstSubRsc = srv.mostDetailedMip_FirstElement_;
				numSubRsc = mipLevels;
				break;
			case ViewDimension::TEX1D_ARRAY:
				srvDesc.Texture1DArray.MostDetailedMip = srv.mostDetailedMip_FirstElement_;
				srvDesc.Texture1DArray.MipLevels = mipLevels;
				srvDesc.Texture1DArray.ArraySize = srv.arraySize_;
				srvDesc.Texture1DArray.FirstArraySlice = srv.firstArraySlice_;
				srvDesc.Texture1DArray.ResourceMinLODClamp = srv.resourceMinLODClamp_;
				firstSubRsc = srv.mostDetailedMip_FirstElement_ + (srv.firstArraySlice_ * textureRes->desc_.levels_);
				numSubRsc = mipLevels;
				break;
			case ViewDimension::TEX2D:
				srvDesc.Texture2D.MostDetailedMip = srv.mostDetailedMip_FirstElement_;
				srvDesc.Texture2D.MipLevels = mipLevels;
				srvDesc.Texture2D.PlaneSlice = srv.planeSlice_;
				srvDesc.Texture2D.ResourceMinLODClamp = srv.resourceMinLODClamp_;
				firstSubRsc = srv.mostDetailedMip_FirstElement_;
				numSubRsc = mipLevels;
				break;
			case ViewDimension::TEX2D_ARRAY:
				srvDesc.Texture2DArray.MostDetailedMip = srv.mostDetailedMip_FirstElement_;
				srvDesc.Texture2DArray.MipLevels = mipLevels;
				srvDesc.Texture2DArray.ArraySize = srv.arraySize_;
				srvDesc.Texture2DArray.FirstArraySlice = srv.firstArraySlice_;
				srvDesc.Texture2DArray.PlaneSlice = srv.planeSlice_;
				srvDesc.Texture2DArray.ResourceMinLODClamp = srv.resourceMinLODClamp_;
				firstSubRsc = srv.mostDetailedMip_FirstElement_ + (srv.firstArraySlice_ * textureRes->desc_.levels_);
				numSubRsc = mipLevels;
				break;
			case ViewDimension::TEX3D:
				srvDesc.Texture3D.MostDetailedMip = srv.mostDetailedMip_FirstElement_;
				srvDesc.Texture3D.MipLevels = mipLevels;
				srvDesc.Texture3D.ResourceMinLODClamp = srv.resourceMinLODClamp_;
				firstSubRsc = srv.mostDetailedMip_FirstElement_;
				numSubRsc = mipLevels;
				break;
			case ViewDimension::TEXCUBE:
				srvDesc.TextureCube.MostDetailedMip = srv.mostDetailedMip_FirstElement_;
				srvDesc.TextureCube.MipLevels = mipLevels;
				srvDesc.TextureCube.ResourceMinLODClamp = srv.resourceMinLODClamp_;
				firstSubRsc = 0;
				numSubRsc = textureRes->desc_.levels_ * 6;
				break;
			case ViewDimension::TEXCUBE_ARRAY:
				srvDesc.TextureCubeArray.MostDetailedMip = srv.mostDetailedMip_FirstElement_;
				srvDesc.TextureCubeArray.MipLevels = mipLevels;
				srvDesc.TextureCubeArray.NumCubes = srv.arraySize_;
				srvDesc.TextureCubeArray.First2DArrayFace = srv.firstArraySlice_;
				srvDesc.TextureCubeArray.ResourceMinLODClamp = srv.resourceMinLODClamp_;
				firstSubRsc = srv.firstArraySlice_ * 6;
				numSubRsc = (textureRes->desc_.levels_ * 6) * srv.arraySize_;
				break;
			default:
				DBG_ASSERT(false);
				return ErrorCode::FAIL;
				break;
			}

			D3D12Resource* resource = GetD3D12Resource(srvHandle);
			DBG_ASSERT(resource);
			pbs.srvTransitions_[bindingIdx].resource_ = resource;
			pbs.srvTransitions_[bindingIdx].firstSubRsc_ = firstSubRsc;
			pbs.srvTransitions_[bindingIdx].numSubRsc_ = numSubRsc;
		}

		return device_->UpdateSRVs(pbs, base, descs.size(), pbs.srvTransitions_.data() + base, srvDescs.data() + base);
	}

	ErrorCode D3D12Backend::UpdatePipelineBindings(Handle handle, i32 base, Core::ArrayView<const BindingUAV> descs)
	{
		Core::ScopedWriteLock lock(resLock_);
		auto& pbs = pipelineBindingSets_[handle.GetIndex()];

		Core::Array<D3D12_UNORDERED_ACCESS_VIEW_DESC, MAX_UAV_BINDINGS> uavDescs = {};

		i32 bindingIdx = base;
		for(i32 i = 0; i < descs.size(); ++i, ++bindingIdx)
		{
			auto uavHandle = descs[i].resource_;
			DBG_ASSERT(uavHandle.IsValid());
			DBG_ASSERT(uavHandle.GetType() == ResourceType::BUFFER || uavHandle.GetType() == ResourceType::TEXTURE);

			const D3D12Buffer* bufferRes = nullptr;
			const D3D12Texture* textureRes = nullptr;
			if(uavHandle.GetType() == ResourceType::BUFFER)
				bufferRes = &bufferResources_[uavHandle.GetIndex()];
			else
				textureRes = &textureResources_[uavHandle.GetIndex()];

			const auto& uav = descs[i];

			i32 firstSubRsc = 0;
			i32 numSubRsc = 0;

			auto& uavDesc = uavDescs[bindingIdx];
			uavDesc.Format = GetFormat(uav.format_);
			uavDesc.ViewDimension = GetUAVDimension(uav.dimension_);
			switch(uav.dimension_)
			{
			case ViewDimension::BUFFER:
				uavDesc.Buffer.FirstElement = uav.mipSlice_FirstElement_;
				uavDesc.Buffer.NumElements = uav.firstArraySlice_FirstWSlice_NumElements_;
				uavDesc.Buffer.StructureByteStride = uav.structureByteStride_;
				if(uavDesc.Format == DXGI_FORMAT_R32_TYPELESS && uav.structureByteStride_ == 0)
					uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
				else
					uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
				firstSubRsc = 0;
				numSubRsc = 1;
				break;
			case ViewDimension::TEX1D:
				uavDesc.Texture1D.MipSlice = uav.mipSlice_FirstElement_;
				firstSubRsc = uav.mipSlice_FirstElement_;
				numSubRsc = 1;
				break;
			case ViewDimension::TEX1D_ARRAY:
				uavDesc.Texture1DArray.MipSlice = uav.mipSlice_FirstElement_;
				uavDesc.Texture1DArray.ArraySize = uav.arraySize_WSize_;
				uavDesc.Texture1DArray.FirstArraySlice = uav.firstArraySlice_FirstWSlice_NumElements_;
				firstSubRsc = uav.mipSlice_FirstElement_ +
				              (uav.firstArraySlice_FirstWSlice_NumElements_ * textureRes->desc_.levels_);
				numSubRsc = uav.arraySize_WSize_;
				break;
			case ViewDimension::TEX2D:
				uavDesc.Texture2D.MipSlice = uav.mipSlice_FirstElement_;
				uavDesc.Texture2D.PlaneSlice = uav.planeSlice_;
				firstSubRsc = uav.mipSlice_FirstElement_;
				numSubRsc = 1;
				break;
			case ViewDimension::TEX2D_ARRAY:
				uavDesc.Texture2DArray.MipSlice = uav.mipSlice_FirstElement_;
				uavDesc.Texture2DArray.ArraySize = uav.arraySize_WSize_;
				uavDesc.Texture2DArray.FirstArraySlice = uav.firstArraySlice_FirstWSlice_NumElements_;
				uavDesc.Texture2DArray.PlaneSlice = uav.planeSlice_;
				firstSubRsc = uav.mipSlice_FirstElement_ +
				              (uav.firstArraySlice_FirstWSlice_NumElements_ * textureRes->desc_.levels_);
				numSubRsc = textureRes->desc_.levels_ * uav.arraySize_WSize_;
				break;
			case ViewDimension::TEX3D:
				uavDesc.Texture3D.MipSlice = uav.mipSlice_FirstElement_;
				uavDesc.Texture3D.FirstWSlice = uav.firstArraySlice_FirstWSlice_NumElements_;
				uavDesc.Texture3D.WSize = uav.arraySize_WSize_;
				firstSubRsc = uav.mipSlice_FirstElement_;
				numSubRsc = 1;
				break;
			default:
				DBG_ASSERT(false);
				return ErrorCode::FAIL;
				break;
			}

			D3D12Resource* resource = GetD3D12Resource(uavHandle);
			DBG_ASSERT(resource);
			pbs.uavTransitions_[bindingIdx].resource_ = resource;
			pbs.uavTransitions_[bindingIdx].firstSubRsc_ = firstSubRsc;
			pbs.uavTransitions_[bindingIdx].numSubRsc_ = numSubRsc;
		}

		return device_->UpdateUAVs(pbs, base, descs.size(), pbs.uavTransitions_.data() + base, uavDescs.data() + base);
	}

	ErrorCode D3D12Backend::UpdatePipelineBindings(Handle handle, i32 base, Core::ArrayView<const SamplerState> descs)
	{
		Core::ScopedReadLock lock(resLock_);
		auto& pbs = pipelineBindingSets_[handle.GetIndex()];

		Core::Array<D3D12_SAMPLER_DESC, MAX_SAMPLER_BINDINGS> samplerDescs;

		i32 bindingIdx = base;
		for(i32 i = 0; i < descs.size(); ++i, ++bindingIdx)
		{
			samplerDescs[bindingIdx] = GetSampler(descs[i]);
		}

		return device_->UpdateSamplers(pbs, base, descs.size(), samplerDescs.data() + base);
	}

	ErrorCode D3D12Backend::CopyPipelineBindings(
	    Core::ArrayView<const PipelineBinding> dst, Core::ArrayView<const PipelineBinding> src)
	{
		Core::ScopedReadLock lock(resLock_);

		auto* d3dDevice = device_->d3dDevice_.Get();
		i32 viewIncr = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		i32 samplerIncr = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

		for(i32 i = 0; i < dst.size(); ++i)
		{
			//DBG_LOG("CopyPipelineBindings: %d\n", i);
			auto& dstPBS = pipelineBindingSets_[dst[i].pbs_.GetIndex()];
			auto& srcPBS = pipelineBindingSets_[src[i].pbs_.GetIndex()];

			auto CopyRange = [d3dDevice](D3D12DescriptorAllocation& dstAlloc,
			                     Core::Vector<D3D12SubresourceRange>& dstTransitions, i32 dstOffset,
			                     D3D12DescriptorAllocation& srcAlloc,
			                     Core::Vector<D3D12SubresourceRange>& srcTransitions, i32 srcOffset, i32 num,
			                     D3D12_DESCRIPTOR_HEAP_TYPE type, i32 incr, DescriptorHeapSubType subType) {
				D3D12_CPU_DESCRIPTOR_HANDLE dstHandle = dstAlloc.GetCPUHandle(dstOffset);
				D3D12_CPU_DESCRIPTOR_HANDLE srcHandle = srcAlloc.GetCPUHandle(srcOffset);
				DBG_ASSERT(dstHandle.ptr);
				DBG_ASSERT(srcHandle.ptr);
				DBG_ASSERT(dstOffset < dstAlloc.size_);
				DBG_ASSERT(srcOffset < srcAlloc.size_);
				DBG_ASSERT((dstOffset + num) <= dstAlloc.size_);
				DBG_ASSERT((srcOffset + num) <= srcAlloc.size_);

#if ENABLE_DESCRIPTOR_DEBUG_DATA
				for(i32 i = 0; i < num; ++i)
				{
					auto& dstDebug = dstAlloc.GetDebugData(dstOffset + i);
					const auto& srcDebug = srcAlloc.GetDebugData(srcOffset + i);
					DBG_ASSERT(srcDebug.subType_ == subType);
					if(srcDebug.subType_ != DescriptorHeapSubType::SAMPLER)
					{
						DBG_ASSERT(srcDebug.resource_);
					}
					else
					{
						DBG_ASSERT(!srcDebug.resource_);
					}
					dstDebug = srcDebug;
				}
#endif

				d3dDevice->CopyDescriptorsSimple(num, dstHandle, srcHandle, type);

				if(srcTransitions.size() > 0)
					for(i32 i = 0; i < num; ++i)
						dstTransitions[i + dstOffset] = srcTransitions[i + srcOffset];
			};

			if(dst[i].cbvs_.num_ > 0)
				CopyRange(dstPBS.cbvs_, dstPBS.cbvTransitions_, dst[i].cbvs_.dstOffset_, srcPBS.cbvs_,
				    srcPBS.cbvTransitions_, src[i].cbvs_.srcOffset_, dst[i].cbvs_.num_,
				    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, viewIncr, DescriptorHeapSubType::CBV);
			if(dst[i].srvs_.num_ > 0)
				CopyRange(dstPBS.srvs_, dstPBS.srvTransitions_, dst[i].srvs_.dstOffset_, srcPBS.srvs_,
				    srcPBS.srvTransitions_, src[i].srvs_.srcOffset_, dst[i].srvs_.num_,
				    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, viewIncr, DescriptorHeapSubType::SRV);
			if(dst[i].uavs_.num_ > 0)
				CopyRange(dstPBS.uavs_, dstPBS.uavTransitions_, dst[i].uavs_.dstOffset_, srcPBS.uavs_,
				    srcPBS.uavTransitions_, src[i].uavs_.srcOffset_, dst[i].uavs_.num_,
				    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, viewIncr, DescriptorHeapSubType::UAV);

			static Core::Vector<D3D12SubresourceRange> emptyTransitions;
			if(dst[i].samplers_.num_ > 0)
				CopyRange(dstPBS.samplers_, emptyTransitions, dst[i].samplers_.dstOffset_, srcPBS.samplers_,
				    emptyTransitions, src[i].samplers_.dstOffset_, dst[i].samplers_.num_,
				    D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, samplerIncr, DescriptorHeapSubType::SAMPLER);
		}

		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::ValidatePipelineBindings(Core::ArrayView<const PipelineBinding> pb)
	{
#if ENABLE_DESCRIPTOR_DEBUG_DATA
		auto LogDescriptors = [this](const char* name, const PipelineBinding& pb) {
			const auto& pbs = pipelineBindingSets_[pb.pbs_.GetIndex()];

			Core::Log("Descriptor (%d):\n", pb.pbs_.GetIndex());
			Core::Log("- CBV Base: %d, %d\n", pbs.cbvs_.offset_, pb.cbvs_.dstOffset_);
			Core::Log("- SRV Base: %d, %d\n", pbs.srvs_.offset_, pb.srvs_.dstOffset_);
			Core::Log("- UAV Base: %d, %d\n", pbs.uavs_.offset_, pb.uavs_.dstOffset_);
			Core::Log("- Sampler Base: %d, %d\n", pbs.samplers_.offset_, pb.samplers_.dstOffset_);

			Core::Log("- CBVs: %d\n", pbs.cbvs_.size_);
			for(i32 i = 0; i < pbs.cbvs_.size_; ++i)
			{
				const auto& debugData = pbs.cbvs_.GetDebugData(i);
				Core::Log("- - %d: %d, %s\n", i, debugData.subType_, debugData.name_);
			}

			Core::Log("- SRVs: %d\n", pbs.srvs_.size_);
			for(i32 i = 0; i < pbs.srvs_.size_; ++i)
			{
				const auto& debugData = pbs.srvs_.GetDebugData(i);
				Core::Log("- - %d: %d, %s\n", i, debugData.subType_, debugData.name_);
			}

			Core::Log("- UAVs: %d\n", pbs.uavs_.size_);
			for(i32 i = 0; i < pbs.uavs_.size_; ++i)
			{
				const auto& debugData = pbs.cbvs_.GetDebugData(i);
				Core::Log("- - %d: %d, %s\n", i, debugData.subType_, debugData.name_);
			}

			Core::Log("- Samplers: %d\n", pbs.samplers_.size_);
			for(i32 i = 0; i < pbs.samplers_.size_; ++i)
			{
				const auto& debugData = pbs.samplers_.GetDebugData(i);
				Core::Log("- - %d: %d, %s\n", i, debugData.subType_, debugData.name_);
			}
		};

		for(auto singlePb : pb)
		{
			LogDescriptors("desc", singlePb);
			const auto& pbs = pipelineBindingSets_[singlePb.pbs_.GetIndex()];
			{
				for(i32 i = 0; i < pb[0].samplers_.num_; ++i)
				{
					DBG_ASSERT(pbs.samplers_.GetDebugData(i).subType_ == DescriptorHeapSubType::SAMPLER);
				}

				for(i32 i = 0; i < pb[0].cbvs_.num_; ++i)
				{
					DBG_ASSERT(pbs.cbvs_.GetDebugData(i).subType_ == DescriptorHeapSubType::CBV);
					DBG_ASSERT(pbs.cbvs_.GetDebugData(i).resource_);
				}

				for(i32 i = 0; i < pb[0].srvs_.num_; ++i)
				{
					DBG_ASSERT(pbs.srvs_.GetDebugData(i).subType_ == DescriptorHeapSubType::SRV);
				}

				for(i32 i = 0; i < pb[0].uavs_.num_; ++i)
				{
					DBG_ASSERT(pbs.uavs_.GetDebugData(i).subType_ == DescriptorHeapSubType::UAV);
					DBG_ASSERT(pbs.uavs_.GetDebugData(i).resource_);
				}
			}
		}
#endif // ENABLE_DESCRIPTOR_DEBUG_DATA
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::CompileCommandList(Handle handle, const CommandList& commandList)
	{
		DBG_ASSERT(handle.GetIndex() < commandLists_.size());

		// Lock resources during command list compilation.
		Core::ScopedReadLock lock(resLock_);

		D3D12CommandList* outCommandList = commandLists_[handle.GetIndex()];
		D3D12CompileContext context(*this);
		return context.CompileCommandList(*outCommandList, commandList);
	}

	ErrorCode D3D12Backend::SubmitCommandLists(Core::ArrayView<Handle> handles)
	{
		Core::Array<D3D12CommandList*, COMMAND_LIST_BATCH_SIZE> commandLists;
		i32 numBatches = (handles.size() + (COMMAND_LIST_BATCH_SIZE - 1)) / COMMAND_LIST_BATCH_SIZE;
		for(i32 batch = 0; batch < numBatches; ++batch)
		{
			const i32 baseHandle = batch * COMMAND_LIST_BATCH_SIZE;
			const i32 numHandles = Core::Min(COMMAND_LIST_BATCH_SIZE, handles.size() - baseHandle);
			for(i32 i = 0; i < numHandles; ++i)
			{
				commandLists[i] = commandLists_[handles[i].GetIndex()];
				DBG_ASSERT(commandLists[i]);
			}

			auto retVal =
			    device_->SubmitCommandLists(Core::ArrayView<D3D12CommandList*>(commandLists.data(), numHandles));
			if(retVal != ErrorCode::OK)
				return retVal;
		}
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::PresentSwapChain(Handle handle)
	{
		Core::ScopedWriteLock lock(resLock_);
		DBG_ASSERT(handle.GetIndex() < swapchainResources_.size());
		D3D12SwapChain& swapChain = swapchainResources_[handle.GetIndex()];

		HRESULT retVal = S_OK;
		CHECK_D3D(retVal = swapChain.swapChain_->Present(0, 0));
		if(FAILED(retVal))
			return ErrorCode::FAIL;
		swapChain.bbIdx_ = swapChain.swapChain_->GetCurrentBackBufferIndex();
		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::ResizeSwapChain(Handle handle, i32 width, i32 height)
	{
		Core::ScopedWriteLock lock(resLock_);
		DBG_ASSERT(handle.GetIndex() < swapchainResources_.size());
		D3D12SwapChain& swapChain = swapchainResources_[handle.GetIndex()];

		ErrorCode retVal = device_->ResizeSwapChain(swapChain, width, height);
		if(retVal != ErrorCode::OK)
			return retVal;
		swapChain.bbIdx_ = swapChain.swapChain_->GetCurrentBackBufferIndex();
		return ErrorCode::OK;
	}

	void D3D12Backend::NextFrame()
	{
		if(device_)
			device_->NextFrame();
	}

	D3D12Resource* D3D12Backend::GetD3D12Resource(Handle handle)
	{
		if(handle.GetType() == ResourceType::BUFFER)
		{
			if(handle.GetIndex() >= bufferResources_.size())
				return nullptr;
			return &bufferResources_[handle.GetIndex()];
		}
		else if(handle.GetType() == ResourceType::TEXTURE)
		{
			if(handle.GetIndex() >= textureResources_.size())
				return nullptr;
			return &textureResources_[handle.GetIndex()];
		}
		else if(handle.GetType() == ResourceType::SWAP_CHAIN)
		{
			if(handle.GetIndex() >= swapchainResources_.size())
				return nullptr;
			D3D12SwapChain& swapChain = swapchainResources_[handle.GetIndex()];
			return &swapChain.textures_[swapChain.bbIdx_];
		}
		return nullptr;
	}

	D3D12Buffer* D3D12Backend::GetD3D12Buffer(Handle handle)
	{
		if(handle.GetType() != ResourceType::BUFFER)
			return nullptr;
		if(handle.GetIndex() >= bufferResources_.size())
			return nullptr;
		return &bufferResources_[handle.GetIndex()];
	}

	D3D12Texture* D3D12Backend::GetD3D12Texture(Handle handle, i32 bufferIdx)
	{
		if(handle.GetType() == ResourceType::TEXTURE)
		{
			if(handle.GetIndex() >= textureResources_.size())
				return nullptr;
			return &textureResources_[handle.GetIndex()];
		}
		else if(handle.GetType() == ResourceType::SWAP_CHAIN)
		{
			if(handle.GetIndex() >= swapchainResources_.size())
				return nullptr;
			D3D12SwapChain& swapChain = swapchainResources_[handle.GetIndex()];
			return &swapChain.textures_[bufferIdx >= 0 ? bufferIdx : swapChain.bbIdx_];
		}
		return nullptr;
	}

} // namespace GPU
