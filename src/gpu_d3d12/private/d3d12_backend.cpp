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

	D3D12Backend::~D3D12Backend() {}

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
		D3D12SwapChainResource swapChain;
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
		//
		return ErrorCode::UNIMPLEMENTED;
	}

	ErrorCode D3D12Backend::CreateShader(Handle handle, const ShaderDesc& desc, const char* debugName)
	{
		//
		return ErrorCode::UNIMPLEMENTED;
	}

	ErrorCode D3D12Backend::CreateGraphicsPipelineState(
	    Handle handle, const GraphicsPipelineStateDesc& desc, const char* debugName)
	{
		//
		return ErrorCode::UNIMPLEMENTED;
	}

	ErrorCode D3D12Backend::CreateComputePipelineState(
	    Handle handle, const ComputePipelineStateDesc& desc, const char* debugName)
	{
		//
		return ErrorCode::UNIMPLEMENTED;
	}

	ErrorCode D3D12Backend::CreatePipelineBindingSet(
	    Handle handle, const PipelineBindingSetDesc& desc, const char* debugName)
	{
		//
		return ErrorCode::UNIMPLEMENTED;
	}

	ErrorCode D3D12Backend::CreateDrawBindingSet(Handle handle, const DrawBindingSetDesc& desc, const char* debugName)
	{
		//
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
			swapchainResources_[handle.GetIndex()] = D3D12SwapChainResource();
			break;
		case ResourceType::BUFFER:
			bufferResources_[handle.GetIndex()] = D3D12Resource();
			break;
		case ResourceType::TEXTURE:
			textureResources_[handle.GetIndex()] = D3D12Resource();
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
