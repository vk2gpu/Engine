#pragma once

#include "core/types.h"
#include "core/library.h"
#include "gpu/types.h"

#include <dxgi1_4.h>
#include <dxgidebug.h>

#include <dxgiformat.h>
#include <d3d12.h>
#include <wrl.h>

typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY)(UINT flags, REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_GET_DXGI_DEBUG_INTERFACE)(UINT flags, REFIID _riid, void** _debug);

namespace GPU
{
	using Microsoft::WRL::ComPtr;

	/**
	 * Library handling.
	 */
	extern Core::LibHandle DXGIDebugHandle;
	extern Core::LibHandle DXGIHandle;
	extern Core::LibHandle D3D12Handle;
	extern PFN_GET_DXGI_DEBUG_INTERFACE DXGIGetDebugInterface1Fn;
	extern PFN_CREATE_DXGI_FACTORY DXGICreateDXGIFactory2Fn;
	extern PFN_D3D12_CREATE_DEVICE D3D12CreateDeviceFn;
	extern PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterfaceFn;
	extern PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignatureFn;

	ErrorCode LoadLibraries();

	/**
	 * Enums.
	 */
	enum class RootSignatureType
	{
		INVALID = -1,
		GRAPHICS,
		COMPUTE,
		MAX
	};

	enum class DescriptorHeapSubType : i32
	{
		INVALID = -1,
		CBV = 0,
		SRV,
		UAV,
		SAMPLER,
		RTV,
		DSV,
	};


	/**
	 * Conversion.
	 */
	D3D12_RESOURCE_FLAGS GetResourceFlags(BindFlags bindFlags);
	D3D12_RESOURCE_STATES GetResourceStates(BindFlags bindFlags);
	D3D12_RESOURCE_STATES GetDefaultResourceState(BindFlags bindFlags);
	D3D12_RESOURCE_DIMENSION GetResourceDimension(TextureType type);
	D3D12_SRV_DIMENSION GetSRVDimension(ViewDimension dim);
	D3D12_UAV_DIMENSION GetUAVDimension(ViewDimension dim);
	D3D12_RTV_DIMENSION GetRTVDimension(ViewDimension dim);
	D3D12_DSV_DIMENSION GetDSVDimension(ViewDimension dim);
	DXGI_FORMAT GetFormat(Format format);
	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology(PrimitiveTopology topology);

	/**
	 * Utility.
	 */
	void SetObjectName(ID3D12Object* object, const char* name);
	void WaitOnFence(ID3D12Fence* fence, HANDLE event, u64 value);


} // namespace GPU

#ifdef DEBUG
#define CHECK_ERRORCODE(a) DBG_ASSERT((a) == ErrorCode::OK)
#define CHECK_D3D(a) DBG_ASSERT((a) == S_OK)
#else
#define CHECK_ERRORCODE(a)
#define CHECK_D3D(a) a
#endif