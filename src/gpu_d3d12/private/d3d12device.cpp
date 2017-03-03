#include "gpu_d3d12/d3d12device.h"
#include "gpu_d3d12/private/shaders/default_cs.h"
#include "gpu_d3d12/private/shaders/default_vs.h"

#include "core/debug.h"

#include <utility>

namespace GPU
{
	D3D12Device::D3D12Device(IDXGIFactory4* dxgiFactory, IDXGIAdapter1* adapter)
	    : dxgiFactory_(dxgiFactory)
	{
		HRESULT hr = S_OK;

		const D3D_FEATURE_LEVEL featureLevels[] = {
		    D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
		};

		for(i32 i = 0; i < 4; ++i)
		{
			hr = D3D12CreateDeviceFn(
			    adapter, featureLevels[i], IID_ID3D12Device, (void**)device_.ReleaseAndGetAddressOf());
			if(SUCCEEDED(hr))
				break;
		}
		if(FAILED(hr))
			return;

#if !defined(FINAL)
		// Setup break on error + corruption.
		ComPtr<ID3D12InfoQueue> infoQueue;
		hr = device_.As(&infoQueue);
		if(SUCCEEDED(hr))
		{
			CHECK_D3D(infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE));
			CHECK_D3D(infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE));
			CHECK_D3D(infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE));
		}
#endif

		// device created, setup command queues.
		CreateCommandQueues();

		// setup root signatures.
		CreateRootSignatures();

		// setup default PSOs.
		CreateDefaultPSOs();
	}

	D3D12Device::~D3D12Device()
	{ //
	}

	void D3D12Device::CreateCommandQueues()
	{
		D3D12_COMMAND_QUEUE_DESC directDesc = {D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0x0};
		D3D12_COMMAND_QUEUE_DESC copyDesc = {D3D12_COMMAND_LIST_TYPE_COPY, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0x0};
		D3D12_COMMAND_QUEUE_DESC asyncDesc = {D3D12_COMMAND_LIST_TYPE_COMPUTE, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0x0};

		HRESULT hr = S_OK;
		CHECK_D3D(hr = device_->CreateCommandQueue(
		              &directDesc, IID_ID3D12CommandQueue, (void**)directQueue_.ReleaseAndGetAddressOf()));
		CHECK_D3D(hr = device_->CreateCommandQueue(
		              &copyDesc, IID_ID3D12CommandQueue, (void**)copyQueue_.ReleaseAndGetAddressOf()));
		CHECK_D3D(hr = device_->CreateCommandQueue(
		              &asyncDesc, IID_ID3D12CommandQueue, (void**)asyncComputeQueue_.ReleaseAndGetAddressOf()));
	}

	void D3D12Device::CreateRootSignatures()
	{
		HRESULT hr = S_OK;
		ComPtr<ID3DBlob> outBlob, errorBlob;

		// Setup descriptor ranges.
		D3D12_DESCRIPTOR_RANGE descriptorRanges[4];
		descriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		descriptorRanges[0].NumDescriptors = MAX_SAMPLER_BINDINGS;
		descriptorRanges[0].BaseShaderRegister = 0;
		descriptorRanges[0].RegisterSpace = 0;
		descriptorRanges[0].OffsetInDescriptorsFromTableStart = 0;

		descriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRanges[1].NumDescriptors = MAX_SRV_BINDINGS;
		descriptorRanges[1].BaseShaderRegister = 0;
		descriptorRanges[1].RegisterSpace = 0;
		descriptorRanges[1].OffsetInDescriptorsFromTableStart = 0;

		descriptorRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		descriptorRanges[2].NumDescriptors = MAX_CBV_BINDINGS;
		descriptorRanges[2].BaseShaderRegister = 0;
		descriptorRanges[2].RegisterSpace = 0;
		descriptorRanges[2].OffsetInDescriptorsFromTableStart = 0;

		descriptorRanges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		descriptorRanges[3].NumDescriptors = MAX_UAV_BINDINGS;
		descriptorRanges[3].BaseShaderRegister = 0;
		descriptorRanges[3].RegisterSpace = 0;
		descriptorRanges[3].OffsetInDescriptorsFromTableStart = 0;

		rootSignatures_.resize((i32)RootSignatureType::MAX);

		auto CreateRootSignature = [&](D3D12_ROOT_SIGNATURE_DESC& desc, RootSignatureType type) {
			hr = D3D12SerializeRootSignatureFn(
			    &desc, D3D_ROOT_SIGNATURE_VERSION_1, outBlob.GetAddressOf(), errorBlob.GetAddressOf());
			if(FAILED(hr))
			{
				const void* bufferData = errorBlob->GetBufferPointer();
				Core::Log(reinterpret_cast<const char*>(bufferData));
			}
			CHECK_D3D(hr = device_->CreateRootSignature(0, outBlob->GetBufferPointer(), outBlob->GetBufferSize(),
			              IID_PPV_ARGS(rootSignatures_[(i32)type].GetAddressOf())));
		};

		D3D12_ROOT_PARAMETER parameters[16];
		auto SetupParams = [&](i32 base, D3D12_SHADER_VISIBILITY visibility) {
			parameters[base].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			parameters[base].DescriptorTable.NumDescriptorRanges = 1;
			parameters[base].DescriptorTable.pDescriptorRanges = &descriptorRanges[0];
			parameters[base].ShaderVisibility = visibility;

			parameters[base + 1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			parameters[base + 1].DescriptorTable.NumDescriptorRanges = 1;
			parameters[base + 1].DescriptorTable.pDescriptorRanges = &descriptorRanges[1];
			parameters[base + 1].ShaderVisibility = visibility;

			parameters[base + 2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			parameters[base + 2].DescriptorTable.NumDescriptorRanges = 1;
			parameters[base + 2].DescriptorTable.pDescriptorRanges = &descriptorRanges[2];
			parameters[base + 2].ShaderVisibility = visibility;
		};

		// VS_PS
		{
			// Setup sampler, srv, and cbv for all stages.
			SetupParams(0, D3D12_SHADER_VISIBILITY_VERTEX);
			SetupParams(3, D3D12_SHADER_VISIBILITY_PIXEL);

			// Now shared UAV for all.
			parameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			parameters[6].DescriptorTable.NumDescriptorRanges = 1;
			parameters[6].DescriptorTable.pDescriptorRanges = &descriptorRanges[3];
			parameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.NumParameters = 7;
			rootSignatureDesc.NumStaticSamplers = 0;
			rootSignatureDesc.pParameters = parameters;
			rootSignatureDesc.pStaticSamplers = nullptr;
			rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			                          D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			                          D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			                          D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

			CreateRootSignature(rootSignatureDesc, RootSignatureType::VS_PS);
		}

		// VS_GS_HS_DS_PS
		{
			// Setup sampler, srv, and cbv for all stages.
			SetupParams(0, D3D12_SHADER_VISIBILITY_VERTEX);
			SetupParams(3, D3D12_SHADER_VISIBILITY_GEOMETRY);
			SetupParams(6, D3D12_SHADER_VISIBILITY_HULL);
			SetupParams(9, D3D12_SHADER_VISIBILITY_DOMAIN);
			SetupParams(12, D3D12_SHADER_VISIBILITY_PIXEL);

			// Now shared UAV for all.
			parameters[15].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			parameters[15].DescriptorTable.NumDescriptorRanges = 1;
			parameters[15].DescriptorTable.pDescriptorRanges = &descriptorRanges[3];
			parameters[15].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.NumParameters = 16;
			rootSignatureDesc.NumStaticSamplers = 0;
			rootSignatureDesc.pParameters = parameters;
			rootSignatureDesc.pStaticSamplers = nullptr;
			rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

			CreateRootSignature(rootSignatureDesc, RootSignatureType::VS_GS_HS_DS_PS);
		}

		// CS
		{
			// Setup sampler, srv, and cbv for all stages.
			SetupParams(0, D3D12_SHADER_VISIBILITY_ALL);

			// Now shared UAV for all.
			parameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			parameters[3].DescriptorTable.NumDescriptorRanges = 1;
			parameters[3].DescriptorTable.pDescriptorRanges = &descriptorRanges[3];
			parameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.NumParameters = 4;
			rootSignatureDesc.NumStaticSamplers = 0;
			rootSignatureDesc.pParameters = parameters;
			rootSignatureDesc.pStaticSamplers = nullptr;
			rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
			                          D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			                          D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			                          D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			                          D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

			CreateRootSignature(rootSignatureDesc, RootSignatureType::CS);
		}
	}

	void D3D12Device::CreateDefaultPSOs()
	{
		HRESULT hr = S_OK;
		D3D12_GRAPHICS_PIPELINE_STATE_DESC defaultPSO = {};
		memset(&defaultPSO, 0, sizeof( defaultPSO ) );

		auto CreateGraphicsPSO = [&](RootSignatureType type)
		{
			D3D12_INPUT_ELEMENT_DESC InputElementDescs[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};
			D3D12_GRAPHICS_PIPELINE_STATE_DESC defaultPSO = {};
			defaultPSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			defaultPSO.InputLayout.NumElements = ARRAYSIZE( InputElementDescs );
			defaultPSO.InputLayout.pInputElementDescs = InputElementDescs;
			defaultPSO.VS.pShaderBytecode = g_VShader;
			defaultPSO.VS.BytecodeLength = ARRAYSIZE( g_VShader );
			defaultPSO.pRootSignature = rootSignatures_[(i32)type].Get();
			defaultPSO.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			defaultPSO.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
			defaultPSO.NumRenderTargets = 1;
			defaultPSO.SampleDesc.Count = 1;
			defaultPSO.SampleDesc.Quality = 0;
			defaultPSO.RTVFormats[ 0 ] = DXGI_FORMAT_R8G8B8A8_UNORM;

			ComPtr<ID3D12PipelineState> pipelineState;
			CHECK_D3D(hr = device_->CreateGraphicsPipelineState( &defaultPSO, 
				IID_ID3D12PipelineState, (void**)pipelineState.GetAddressOf()));
			defaultPSOs_.push_back(pipelineState);
		};

		auto CreateComputePSO = [&](RootSignatureType type)
		{
			D3D12_COMPUTE_PIPELINE_STATE_DESC defaultPSO = {};
			defaultPSO.CS.pShaderBytecode = g_CShader;
			defaultPSO.CS.BytecodeLength = ARRAYSIZE( g_CShader );
			defaultPSO.pRootSignature = rootSignatures_[(i32)type].Get();

			ComPtr<ID3D12PipelineState> pipelineState;
			CHECK_D3D(hr = device_->CreateComputePipelineState( &defaultPSO, 
				IID_ID3D12PipelineState, (void**)pipelineState.GetAddressOf()));
			defaultPSOs_.push_back(pipelineState);
		};

		CreateGraphicsPSO(RootSignatureType::VS_PS);
		CreateGraphicsPSO(RootSignatureType::VS_GS_HS_DS_PS);
		CreateComputePSO(RootSignatureType::CS);
	}

	ErrorCode D3D12Device::CreateSwapChain(
	    D3D12SwapChainResource& outResource, const SwapChainDesc& desc, const char* debugName)
	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
		memset(&swapChainDesc, 0, sizeof(swapChainDesc));
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Width = desc.width_;
		swapChainDesc.Height = desc.height_;
		swapChainDesc.Format = GetFormat(desc.format_);
		swapChainDesc.Scaling = DXGI_SCALING_NONE;
		swapChainDesc.BufferCount = desc.bufferCount_;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.Flags = 0;

		HRESULT hr = S_OK;
		D3D12SwapChainResource swapChainRes;
		ComPtr<IDXGISwapChain1> swapChain;
		CHECK_D3D(hr = dxgiFactory_->CreateSwapChainForHwnd(directQueue_.Get(), (HWND)desc.outputWindow_,
		              &swapChainDesc, nullptr, nullptr, swapChain.GetAddressOf()));
		if(hr != S_OK)
			return ErrorCode::FAIL;

		swapChain.As(&swapChainRes.swapChain_);
		swapChainRes.textures_.resize(swapChainDesc.BufferCount);

		// Setup swapchain resources.
		TextureDesc texDesc;
		texDesc.type_ = TextureType::TEX2D;
		texDesc.bindFlags_ = BindFlags::RENDER_TARGET | BindFlags::PRESENT;
		texDesc.format_ = desc.format_;
		texDesc.width_ = desc.width_;
		texDesc.height_ = desc.height_;
		texDesc.depth_ = 1;
		texDesc.elements_ = 1;
		texDesc.levels_ = 1;

		for(i32 i = 0; i < swapChainRes.textures_.size(); ++i)
		{
			auto& texResource = swapChainRes.textures_[i];

			// Get buffer from swapchain.
			CHECK_D3D(hr = swapChainRes.swapChain_->GetBuffer(
			              i, IID_ID3D12Resource, (void**)texResource.resource_.ReleaseAndGetAddressOf()));

			// Setup states.
			texResource.supportedStates_ = GetResourceStates(texDesc.bindFlags_);
			texResource.defaultState_ = GetDefaultResourceState(texDesc.bindFlags_);
		}

		return ErrorCode::OK;
	}

	ErrorCode D3D12Device::CreateBuffer(
	    D3D12Resource& outResource, const BufferDesc& desc, const void* initialData, const char* debugName)
	{
		D3D12Resource res;
		res.supportedStates_ = GetResourceStates(desc.bindFlags_);
		res.defaultState_ = GetDefaultResourceState(desc.bindFlags_);

		// Add on copy src/dest flags to supported flags, copy dest to default.
		res.supportedStates_ |= D3D12_RESOURCE_STATE_COPY_SOURCE;
		res.supportedStates_ |= D3D12_RESOURCE_STATE_COPY_DEST;

		D3D12_HEAP_PROPERTIES heapProperties;
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		heapProperties.CreationNodeMask = 0x0;
		heapProperties.VisibleNodeMask = 0x0;

		D3D12_RESOURCE_DESC resourceDesc;
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		resourceDesc.Width = desc.size_;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = GetResourceFlags(desc.bindFlags_);

		ComPtr<ID3D12Resource> d3dResource;
		HRESULT hr = S_OK;
		CHECK_D3D(hr = device_->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		              res.defaultState_, nullptr, IID_PPV_ARGS(d3dResource.GetAddressOf())));
		if(FAILED(hr))
			return ErrorCode::FAIL;

		d3dResource.As(&res.resource_);

		// TODO: InitialData via copy queue.
		if(initialData)
		{
			DBG_BREAK;
			return ErrorCode::UNIMPLEMENTED;
		}

		return ErrorCode::OK;
	}

	ErrorCode D3D12Device::CreateTexture(D3D12Resource& outResource, const TextureDesc& desc,
	    const TextureSubResourceData* initialData, const char* debugName)
	{
		D3D12Resource res;
		res.supportedStates_ = GetResourceStates(desc.bindFlags_);
		res.defaultState_ = GetDefaultResourceState(desc.bindFlags_);

		// Add on copy src/dest flags to supported flags, copy dest to default.
		res.supportedStates_ |= D3D12_RESOURCE_STATE_COPY_SOURCE;
		res.supportedStates_ |= D3D12_RESOURCE_STATE_COPY_DEST;

		D3D12_HEAP_PROPERTIES heapProperties;
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		heapProperties.CreationNodeMask = 0x0;
		heapProperties.VisibleNodeMask = 0x0;

		D3D12_RESOURCE_DESC resourceDesc;
		resourceDesc.Dimension = GetResourceDimension(desc.type_);
		resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		resourceDesc.Width = desc.width_;
		resourceDesc.Height = desc.height_;
		resourceDesc.DepthOrArraySize = desc.type_ == TextureType::TEX3D ? desc.depth_ : desc.elements_;
		resourceDesc.MipLevels = desc.levels_;
		resourceDesc.Format = GetFormat(desc.format_);
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = GetResourceFlags(desc.bindFlags_);

		if(desc.type_ == TextureType::TEXCUBE)
		{
			resourceDesc.DepthOrArraySize = 6;
		}

		// Set default clear.
		D3D12_CLEAR_VALUE clearValue;
		D3D12_CLEAR_VALUE* setClearValue = nullptr;
		memset(&clearValue, 0, sizeof(clearValue));

		// Setup initial bind type to be whatever is likely what it will be used as first.
		if(Core::ContainsAllFlags(desc.bindFlags_, BindFlags::RENDER_TARGET))
		{
			clearValue.Format = GetFormat(desc.format_);
			setClearValue = &clearValue;
		}
		else if(Core::ContainsAllFlags(desc.bindFlags_, BindFlags::DEPTH_STENCIL))
		{
			clearValue.Format = GetFormat(desc.format_);
			clearValue.DepthStencil.Depth = 0.0f;
			clearValue.DepthStencil.Stencil = 0;
			setClearValue = &clearValue;
		}

		ComPtr<ID3D12Resource> d3dResource;
		HRESULT hr = S_OK;
		CHECK_D3D(hr = device_->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		              res.defaultState_, setClearValue, IID_PPV_ARGS(d3dResource.GetAddressOf())));
		if(FAILED(hr))
			return ErrorCode::FAIL;

		d3dResource.As(&res.resource_);

		// TODO: InitialData via copy queue.
		if(initialData)
		{
			DBG_BREAK;
			return ErrorCode::UNIMPLEMENTED;
		}

		return ErrorCode::OK;
	}


} // namespace GPU
