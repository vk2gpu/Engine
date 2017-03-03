#include "gpu_d3d12/d3d12device.h"
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

			// Setup supported states.
			texResource.supportedStates_ = GetResourceStates(texDesc.bindFlags_);
		}

		return ErrorCode::OK;
	}

	ErrorCode D3D12Device::CreateBuffer(D3D12Resource& outResource, const BufferDesc& desc, const char* debugName)
	{
		return ErrorCode::UNIMPLEMENTED;
	}

	ErrorCode D3D12Device::CreateTexture(D3D12Resource& outResource, const TextureDesc& desc, const char* debugName)
	{
		return ErrorCode::UNIMPLEMENTED;
	}


} // namespace GPU
