#include "gpu_d3d12/d3d12backend.h"
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
	namespace
	{
		static const GUID IID_ID3D12CommandAllocator = {
		    0x6102dee4, 0xaf59, 0x4b09, {0xb9, 0x99, 0xb4, 0x4d, 0x73, 0xf0, 0x9b, 0x24}};
		static const GUID IID_ID3D12CommandQueue = {
		    0x0ec870a6, 0x5d7e, 0x4c22, {0x8c, 0xfc, 0x5b, 0xaa, 0xe0, 0x76, 0x16, 0xed}};
		static const GUID IID_ID3D12CommandSignature = {
		    0xc36a797c, 0xec80, 0x4f0a, {0x89, 0x85, 0xa7, 0xb2, 0x47, 0x50, 0x82, 0xd1}};
		static const GUID IID_ID3D12Debug = {
		    0x344488b7, 0x6846, 0x474b, {0xb9, 0x89, 0xf0, 0x27, 0x44, 0x82, 0x45, 0xe0}};
		static const GUID IID_ID3D12DescriptorHeap = {
		    0x8efb471d, 0x616c, 0x4f49, {0x90, 0xf7, 0x12, 0x7b, 0xb7, 0x63, 0xfa, 0x51}};
		static const GUID IID_ID3D12Device = {
		    0x189819f1, 0x1db6, 0x4b57, {0xbe, 0x54, 0x18, 0x21, 0x33, 0x9b, 0x85, 0xf7}};
		static const GUID IID_ID3D12Fence = {
		    0x0a753dcf, 0xc4d8, 0x4b91, {0xad, 0xf6, 0xbe, 0x5a, 0x60, 0xd9, 0x5a, 0x76}};
		static const GUID IID_ID3D12GraphicsCommandList = {
		    0x5b160d0f, 0xac1b, 0x4185, {0x8b, 0xa8, 0xb3, 0xae, 0x42, 0xa5, 0xa4, 0x55}};
		static const GUID IID_ID3D12InfoQueue = {
		    0x0742a90b, 0xc387, 0x483f, {0xb9, 0x46, 0x30, 0xa7, 0xe4, 0xe6, 0x14, 0x58}};
		static const GUID IID_ID3D12PipelineState = {
		    0x765a30f3, 0xf624, 0x4c6f, {0xa8, 0x28, 0xac, 0xe9, 0x48, 0x62, 0x24, 0x45}};
		static const GUID IID_ID3D12Resource = {
		    0x696442be, 0xa72e, 0x4059, {0xbc, 0x79, 0x5b, 0x5c, 0x98, 0x04, 0x0f, 0xad}};
		static const GUID IID_ID3D12RootSignature = {
		    0xc54a6b66, 0x72df, 0x4ee8, {0x8b, 0xe5, 0xa9, 0x46, 0xa1, 0x42, 0x92, 0x14}};
		static const GUID IID_ID3D12QueryHeap = {
		    0x0d9658ae, 0xed45, 0x469e, {0xa6, 0x1d, 0x97, 0x0e, 0xc5, 0x83, 0xca, 0xb4}};
		static const GUID IID_IDXGIFactory4 = {
		    0x1bc6ea02, 0xef36, 0x464f, {0xbf, 0x0c, 0x21, 0xca, 0x39, 0xe5, 0x16, 0x8a}};
	}


	D3D12Backend::D3D12Backend(void* deviceWindow)
	{
		auto retVal = LoadLibraries();
		DBG_ASSERT(retVal == ErrorCode::OK);
	}

	D3D12Backend::~D3D12Backend() {}

	ErrorCode D3D12Backend::LoadLibraries()
	{
		dxgiHandle_ = Core::LibraryOpen("dxgi.dll");
		d3d12Handle_ = Core::LibraryOpen("d3d12.dll");
		if(!dxgiHandle_)
			return ErrorCode::UNSUPPORTED;
		if(!d3d12Handle_)
			return ErrorCode::UNSUPPORTED;

		// Load symbols.
		dxgiCreateDXGIFactory1Fn_ = (PFN_CREATE_DXGI_FACTORY)Core::LibrarySymbol(dxgiHandle_, "CreateDXGIFactory1");
		d3d12CreateDeviceFn_ = (PFN_D3D12_CREATE_DEVICE)Core::LibrarySymbol(d3d12Handle_, "D3D12CreateDevice");
		d3d12GetDebugInterfaceFn_ =
		    (PFN_D3D12_GET_DEBUG_INTERFACE)Core::LibrarySymbol(d3d12Handle_, "D3D12GetDebugInterface");
		d3d12SerializeRootSignatureFn_ =
		    (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)Core::LibrarySymbol(d3d12Handle_, "D3D12SerializeRootSignature");

		if(!dxgiCreateDXGIFactory1Fn_)
			return ErrorCode::UNSUPPORTED;
		if(!d3d12CreateDeviceFn_)
			return ErrorCode::UNSUPPORTED;
		if(!d3d12GetDebugInterfaceFn_)
			return ErrorCode::UNSUPPORTED;
		if(!d3d12SerializeRootSignatureFn_)
			return ErrorCode::UNSUPPORTED;

		// Create DXGI factory.
		dxgiCreateDXGIFactory1Fn_(IID_IDXGIFactory4, (void**)dxgiFactory_.ReleaseAndGetAddressOf());

		// Done.
		return ErrorCode::OK;
	}


	i32 D3D12Backend::EnumerateAdapters(AdapterInfo* outAdapters, i32 maxAdapters)
	{
		if(adapterInfos_.size() == 0)
		{
			ComPtr<IDXGIAdapter1> adapter;
			while(SUCCEEDED(dxgiFactory_->EnumAdapters1(adapters_.size(), adapter.ReleaseAndGetAddressOf())))
			{
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				AdapterInfo outAdapter;
				Core::StringConvertUTF16toUTF8(desc.Description, 128, outAdapter.description_, sizeof(outAdapter));
				outAdapter.deviceIdx_ = adapters_.size();
				outAdapter.vendorId_ = desc.VendorId;
				outAdapter.deviceId_ = desc.DeviceId;
				outAdapter.subSysId_ = desc.SubSysId;
				outAdapter.revision_ = desc.Revision;
				outAdapter.dedicatedVideoMemory_ = desc.DedicatedVideoMemory;
				outAdapter.dedicatedSystemMemory_ = desc.DedicatedSystemMemory;
				outAdapter.sharedSystemMemory_ = desc.SharedSystemMemory;
				adapterInfos_.push_back(outAdapter);
				adapters_.push_back(adapter);
			}
		}

		for(i32 idx = 0; idx < maxAdapters; ++idx)
		{
			outAdapters[idx] = adapterInfos_[idx];
		}

		return adapterInfos_.size();
	}

	bool D3D12Backend::IsInitialized() const
	{
		return device_;
	}

	ErrorCode D3D12Backend::Initialize(i32 adapterIdx)
	{
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
		ComPtr<ID3D12Device> device;

		HRESULT result = d3d12CreateDeviceFn_(adapters_[adapterIdx].Get(), featureLevel, IID_ID3D12Device, (void**)device.ReleaseAndGetAddressOf());
		if(FAILED(result))
		{
			return ErrorCode::FAIL;
		}


		return ErrorCode::OK;
	}

	ErrorCode D3D12Backend::CreateSwapChain(Handle handle, const SwapChainDesc& desc, const char* debugName)
	{
		//
		return ErrorCode::UNIMPLEMENTED;
	}

	ErrorCode D3D12Backend::CreateBuffer(
	    Handle handle, const BufferDesc& desc, const void* initialData, const char* debugName)
	{
		//
		return ErrorCode::UNIMPLEMENTED;
	}

	ErrorCode D3D12Backend::CreateTexture(
	    Handle handle, const TextureDesc& desc, const TextureSubResourceData* initialData, const char* debugName)
	{
		//
		return ErrorCode::UNIMPLEMENTED;
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
		//
		return ErrorCode::UNIMPLEMENTED;
	}

	ErrorCode D3D12Backend::CreateFence(Handle handle, const char* debugName)
	{
		//
		return ErrorCode::UNIMPLEMENTED;
	}

	ErrorCode D3D12Backend::DestroyResource(Handle handle)
	{
		//
		return ErrorCode::UNIMPLEMENTED;
	}

} // namespace GPU
