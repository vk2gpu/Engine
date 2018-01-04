#include "gpu_d3d12/d3d12_types.h"
#include "core/concurrency.h"
#include "core/debug.h"
#include "core/misc.h"
#include "core/string.h"

// clang-format off
const GUID IID_ID3D12CommandAllocator = {0x6102dee4, 0xaf59, 0x4b09, {0xb9, 0x99, 0xb4, 0x4d, 0x73, 0xf0, 0x9b, 0x24}};
const GUID IID_ID3D12CommandQueue = {0x0ec870a6, 0x5d7e, 0x4c22, {0x8c, 0xfc, 0x5b, 0xaa, 0xe0, 0x76, 0x16, 0xed}};
const GUID IID_ID3D12CommandSignature = {0xc36a797c, 0xec80, 0x4f0a, {0x89, 0x85, 0xa7, 0xb2, 0x47, 0x50, 0x82, 0xd1}};
const GUID IID_ID3D12Debug = {0x344488b7, 0x6846, 0x474b, {0xb9, 0x89, 0xf0, 0x27, 0x44, 0x82, 0x45, 0xe0}};
const GUID IID_ID3D12Debug1 = {0xaffaa4ca, 0x63fe, 0x4d8e, { 0xb8, 0xad, 0x15, 0x90, 0x00, 0xaf, 0x43, 0x04}};
const GUID IID_ID3D12DescriptorHeap = {0x8efb471d, 0x616c, 0x4f49, {0x90, 0xf7, 0x12, 0x7b, 0xb7, 0x63, 0xfa, 0x51}};
const GUID IID_ID3D12Device = {0x189819f1, 0x1db6, 0x4b57, {0xbe, 0x54, 0x18, 0x21, 0x33, 0x9b, 0x85, 0xf7}};
const GUID IID_ID3D12Fence = {0x0a753dcf, 0xc4d8, 0x4b91, {0xad, 0xf6, 0xbe, 0x5a, 0x60, 0xd9, 0x5a, 0x76}};
const GUID IID_ID3D12Fence1 = {0x433685fe,0xe22b,0x4ca0,{0xa8,0xdb,0xb5,0xb4,0xf4,0xdd,0x0e,0x4a}};
const GUID IID_ID3D12CommandList = {0x7116d91c, 0xe7e4, 0x47ce, {0xb8, 0xc6, 0xec, 0x81, 0x68, 0xf4, 0x37, 0xe5}};
const GUID IID_ID3D12GraphicsCommandList = {0x5b160d0f, 0xac1b, 0x4185, {0x8b, 0xa8, 0xb3, 0xae, 0x42, 0xa5, 0xa4, 0x55}};
const GUID IID_ID3D12GraphicsCommandList1 = {0x553103fb,0x1fe7,0x4557,{0xbb,0x38,0x94,0x6d,0x7d,0x0e,0x7c,0xa7}};
const GUID IID_ID3D12GraphicsCommandList2 = {0x38C3E585,0xFF17,0x412C,{0x91,0x50,0x4F,0xC6,0xF9,0xD7,0x2A,0x28}};
const GUID IID_ID3D12InfoQueue = {0x0742a90b, 0xc387, 0x483f, {0xb9, 0x46, 0x30, 0xa7, 0xe4, 0xe6, 0x14, 0x58}};
const GUID IID_ID3D12PipelineState = {0x765a30f3, 0xf624, 0x4c6f, {0xa8, 0x28, 0xac, 0xe9, 0x48, 0x62, 0x24, 0x45}};
const GUID IID_ID3D12Resource = {0x696442be, 0xa72e, 0x4059, {0xbc, 0x79, 0x5b, 0x5c, 0x98, 0x04, 0x0f, 0xad}};
const GUID IID_ID3D12RootSignature = {0xc54a6b66, 0x72df, 0x4ee8, {0x8b, 0xe5, 0xa9, 0x46, 0xa1, 0x42, 0x92, 0x14}};
const GUID IID_ID3D12QueryHeap = {0x0d9658ae, 0xed45, 0x469e, {0xa6, 0x1d, 0x97, 0x0e, 0xc5, 0x83, 0xca, 0xb4}};
const GUID IID_ID3D12PipelineLibrary = {0xc64226a8,0x9201,0x46af,{0xb4,0xcc,0x53,0xfb,0x9f,0xf7,0x41,0x4f}};
const GUID IID_ID3D12PipelineLibrary1 = {0x80eabf42,0x2568,0x4e5e,{0xbd,0x82,0xc3,0x7f,0x86,0x96,0x1d,0xc3}};
const GUID IID_ID3D12Device1 = {0x77acce80,0x638e,0x4e65,{0x88,0x95,0xc1,0xf2,0x33,0x86,0x86,0x3e}};
const GUID IID_ID3D12Device2 = {0x30baa41e,0xb15b,0x475c,{0xa0,0xbb,0x1a,0xf5,0xc5,0xb6,0x43,0x28}};
const GUID IID_ID3D12Device3 = {0x81dadc15,0x2bad,0x4392,{0x93,0xc5,0x10,0x13,0x45,0xc4,0xaa,0x98}};
const GUID IID_ID3D12Tools = {0x7071e1f0,0xe84b,0x4b33,{0x97,0x4f,0x12,0xfa,0x49,0xde,0x65,0xc5}};
const GUID IID_IDXGIInfoQueue = {0xD67441C7, 0x672A, 0x476f, {0x9E, 0x82, 0xCD, 0x55, 0xB4, 0x49, 0x49, 0xCE}};
const GUID IID_IDXGIDebug = {0x119E7452, 0xDE9E, 0x40fe, {0x88, 0x06, 0x88, 0xF9, 0x0C, 0x12, 0xB4, 0x41}};
const GUID IID_IDXGIFactory1 = {0x790a45f7, 0x0d42, 0x4876, {0x98, 0x3a, 0x0a, 0x55, 0xcf, 0xe6, 0xf4, 0xaa}};
const GUID IID_IDXGIFactory4 = {0x1bc6ea02, 0xef36, 0x464f, {0xbf, 0x0c, 0x21, 0xca, 0x39, 0xe5, 0x16, 0x8a}};
const GUID DXGI_DEBUG_ALL = {0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8}};
const GUID DXGI_DEBUG_DX = {0x35cdd7fc, 0x13b2, 0x421d, {0xa5, 0xd7, 0x7e, 0x44, 0x51, 0x28, 0x7d, 0x64}};
const GUID DXGI_DEBUG_DXGI = {0x25cddaa4, 0xb1c6, 0x47e1, {0xac, 0x3e, 0x98, 0x87, 0x5b, 0x5a, 0x2e, 0x2a}};
const GUID DXGI_DEBUG_APP = {0x6cd6e01, 0x4219, 0x4ebd, {0x87, 0x9, 0x27, 0xed, 0x23, 0x36, 0xc, 0x62}};
// clang-format on

namespace GPU
{
	bool Initialized = false;
	Core::LibHandle DXGIDebugHandle = 0;
	Core::LibHandle DXGIHandle = 0;
	Core::LibHandle D3D12Handle = 0;
	extern PFN_GET_DXGI_DEBUG_INTERFACE DXGIGetDebugInterface1Fn = nullptr;
	PFN_CREATE_DXGI_FACTORY DXGICreateDXGIFactory2Fn = nullptr;
	PFN_D3D12_CREATE_DEVICE D3D12CreateDeviceFn = nullptr;
	PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterfaceFn = nullptr;
	PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignatureFn = nullptr;

	ErrorCode LoadLibraries()
	{
		if(Initialized)
			return ErrorCode::OK;

		DXGIDebugHandle = Core::LibraryOpen("dxgidebug.dll");
		DXGIHandle = Core::LibraryOpen("dxgi.dll");
		D3D12Handle = Core::LibraryOpen("d3d12.dll");

		if(!DXGIHandle)
			return ErrorCode::FAIL;
		if(!D3D12Handle)
			return ErrorCode::FAIL;
		// Load symbols.
		DXGIGetDebugInterface1Fn =
		    (PFN_GET_DXGI_DEBUG_INTERFACE)Core::LibrarySymbol(DXGIHandle, "DXGIGetDebugInterface1");
		DXGICreateDXGIFactory2Fn = (PFN_CREATE_DXGI_FACTORY)Core::LibrarySymbol(DXGIHandle, "CreateDXGIFactory2");
		D3D12CreateDeviceFn = (PFN_D3D12_CREATE_DEVICE)Core::LibrarySymbol(D3D12Handle, "D3D12CreateDevice");
		D3D12GetDebugInterfaceFn =
		    (PFN_D3D12_GET_DEBUG_INTERFACE)Core::LibrarySymbol(D3D12Handle, "D3D12GetDebugInterface");
		D3D12SerializeRootSignatureFn =
		    (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)Core::LibrarySymbol(D3D12Handle, "D3D12SerializeRootSignature");

		if(!DXGICreateDXGIFactory2Fn)
			return ErrorCode::FAIL;
		if(!D3D12CreateDeviceFn)
			return ErrorCode::FAIL;
		if(!D3D12GetDebugInterfaceFn)
			return ErrorCode::FAIL;
		if(!D3D12SerializeRootSignatureFn)
			return ErrorCode::FAIL;

		// Done.
		return ErrorCode::OK;
	}

	D3D12_RESOURCE_FLAGS GetResourceFlags(BindFlags bindFlags)
	{
		D3D12_RESOURCE_FLAGS retVal = D3D12_RESOURCE_FLAG_NONE;
		if(Core::ContainsAllFlags(bindFlags, BindFlags::RENDER_TARGET))
			retVal |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		if(Core::ContainsAllFlags(bindFlags, BindFlags::DEPTH_STENCIL))
			retVal |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		if(Core::ContainsAllFlags(bindFlags, BindFlags::UNORDERED_ACCESS))
			retVal |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		if(!Core::ContainsAllFlags(bindFlags, BindFlags::SHADER_RESOURCE) &&
		    Core::ContainsAllFlags(bindFlags, BindFlags::DEPTH_STENCIL))
			retVal |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		return retVal;
	}

	D3D12_RESOURCE_STATES GetResourceStates(BindFlags bindFlags)
	{
		D3D12_RESOURCE_STATES retVal = D3D12_RESOURCE_STATE_COMMON;
		if(Core::ContainsAnyFlags(bindFlags, BindFlags::VERTEX_BUFFER | BindFlags::CONSTANT_BUFFER))
			retVal |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		if(Core::ContainsAllFlags(bindFlags, BindFlags::INDEX_BUFFER))
			retVal |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
		if(Core::ContainsAllFlags(bindFlags, BindFlags::INDIRECT_BUFFER))
			retVal |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		if(Core::ContainsAllFlags(bindFlags, BindFlags::SHADER_RESOURCE))
			retVal |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		if(Core::ContainsAllFlags(bindFlags, BindFlags::STREAM_OUTPUT))
			retVal |= D3D12_RESOURCE_STATE_STREAM_OUT;
		if(Core::ContainsAllFlags(bindFlags, BindFlags::RENDER_TARGET))
			retVal |= D3D12_RESOURCE_STATE_RENDER_TARGET;
		if(Core::ContainsAllFlags(bindFlags, BindFlags::DEPTH_STENCIL))
			retVal |= D3D12_RESOURCE_STATE_DEPTH_WRITE | D3D12_RESOURCE_STATE_DEPTH_READ;
		if(Core::ContainsAllFlags(bindFlags, BindFlags::UNORDERED_ACCESS))
			retVal |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		if(Core::ContainsAllFlags(bindFlags, BindFlags::PRESENT))
			retVal |= D3D12_RESOURCE_STATE_PRESENT;
		return retVal;
	}

	D3D12_RESOURCE_STATES GetDefaultResourceState(BindFlags bindFlags)
	{
		D3D12_RESOURCE_STATES retVal = D3D12_RESOURCE_STATE_COMMON;
		if(Core::ContainsAnyFlags(bindFlags, BindFlags::VERTEX_BUFFER | BindFlags::CONSTANT_BUFFER))
			retVal = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		if(Core::ContainsAllFlags(bindFlags, BindFlags::INDEX_BUFFER))
			retVal = D3D12_RESOURCE_STATE_INDEX_BUFFER;
		if(Core::ContainsAllFlags(bindFlags, BindFlags::INDIRECT_BUFFER))
			retVal = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		if(Core::ContainsAllFlags(bindFlags, BindFlags::SHADER_RESOURCE))
			retVal = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		if(Core::ContainsAllFlags(bindFlags, BindFlags::STREAM_OUTPUT))
			retVal = D3D12_RESOURCE_STATE_STREAM_OUT;
		if(Core::ContainsAllFlags(bindFlags, BindFlags::UNORDERED_ACCESS))
			retVal = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		if(Core::ContainsAllFlags(bindFlags, BindFlags::RENDER_TARGET))
			retVal = D3D12_RESOURCE_STATE_RENDER_TARGET;
		if(Core::ContainsAllFlags(bindFlags, BindFlags::DEPTH_STENCIL))
			retVal = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		if(Core::ContainsAllFlags(bindFlags, BindFlags::PRESENT))
			retVal = D3D12_RESOURCE_STATE_PRESENT;
		return retVal;
	}

	D3D12_RESOURCE_DIMENSION GetResourceDimension(TextureType type)
	{
		switch(type)
		{
		case TextureType::TEX1D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		case TextureType::TEX2D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		case TextureType::TEX3D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		case TextureType::TEXCUBE:
			return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		default:
			return D3D12_RESOURCE_DIMENSION_UNKNOWN;
		}
	}

	D3D12_SRV_DIMENSION GetSRVDimension(ViewDimension dim)
	{
		switch(dim)
		{
		case ViewDimension::BUFFER:
			return D3D12_SRV_DIMENSION_BUFFER;
		case ViewDimension::TEX1D:
			return D3D12_SRV_DIMENSION_TEXTURE1D;
		case ViewDimension::TEX1D_ARRAY:
			return D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
		case ViewDimension::TEX2D:
			return D3D12_SRV_DIMENSION_TEXTURE2D;
		case ViewDimension::TEX2D_ARRAY:
			return D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		case ViewDimension::TEX3D:
			return D3D12_SRV_DIMENSION_TEXTURE3D;
		case ViewDimension::TEXCUBE:
			return D3D12_SRV_DIMENSION_TEXTURECUBE;
		case ViewDimension::TEXCUBE_ARRAY:
			return D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
		default:
			return D3D12_SRV_DIMENSION_UNKNOWN;
		}
	}

	D3D12_UAV_DIMENSION GetUAVDimension(ViewDimension dim)
	{
		switch(dim)
		{
		case ViewDimension::BUFFER:
			return D3D12_UAV_DIMENSION_BUFFER;
		case ViewDimension::TEX1D:
			return D3D12_UAV_DIMENSION_TEXTURE1D;
		case ViewDimension::TEX1D_ARRAY:
			return D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
		case ViewDimension::TEX2D:
			return D3D12_UAV_DIMENSION_TEXTURE2D;
		case ViewDimension::TEX2D_ARRAY:
			return D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		case ViewDimension::TEX3D:
			return D3D12_UAV_DIMENSION_TEXTURE3D;
		default:
			return D3D12_UAV_DIMENSION_UNKNOWN;
		}
	}

	D3D12_RTV_DIMENSION GetRTVDimension(ViewDimension dim)
	{
		switch(dim)
		{
		case ViewDimension::BUFFER:
			return D3D12_RTV_DIMENSION_BUFFER;
		case ViewDimension::TEX1D:
			return D3D12_RTV_DIMENSION_TEXTURE1D;
		case ViewDimension::TEX1D_ARRAY:
			return D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
		case ViewDimension::TEX2D:
			return D3D12_RTV_DIMENSION_TEXTURE2D;
		case ViewDimension::TEX2D_ARRAY:
			return D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		case ViewDimension::TEX3D:
			return D3D12_RTV_DIMENSION_TEXTURE3D;
		default:
			return D3D12_RTV_DIMENSION_UNKNOWN;
		}
	}

	D3D12_DSV_DIMENSION GetDSVDimension(ViewDimension dim)
	{
		switch(dim)
		{
		case ViewDimension::TEX1D:
			return D3D12_DSV_DIMENSION_TEXTURE1D;
		case ViewDimension::TEX1D_ARRAY:
			return D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
		case ViewDimension::TEX2D:
			return D3D12_DSV_DIMENSION_TEXTURE2D;
		case ViewDimension::TEX2D_ARRAY:
			return D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		default:
			return D3D12_DSV_DIMENSION_UNKNOWN;
		}
	}

	DXGI_FORMAT GetFormat(Format format)
	{
#define CASE(FORMAT)                                                                                                   \
	case Format::FORMAT:                                                                                               \
		return DXGI_FORMAT_##FORMAT

		switch(format)
		{
			CASE(R32G32B32A32_TYPELESS);
			CASE(R32G32B32A32_FLOAT);
			CASE(R32G32B32A32_UINT);
			CASE(R32G32B32A32_SINT);
			CASE(R32G32B32_TYPELESS);
			CASE(R32G32B32_FLOAT);
			CASE(R32G32B32_UINT);
			CASE(R32G32B32_SINT);
			CASE(R16G16B16A16_TYPELESS);
			CASE(R16G16B16A16_FLOAT);
			CASE(R16G16B16A16_UNORM);
			CASE(R16G16B16A16_UINT);
			CASE(R16G16B16A16_SNORM);
			CASE(R16G16B16A16_SINT);
			CASE(R32G32_TYPELESS);
			CASE(R32G32_FLOAT);
			CASE(R32G32_UINT);
			CASE(R32G32_SINT);
			CASE(R32G8X24_TYPELESS);
			CASE(D32_FLOAT_S8X24_UINT);
			CASE(R32_FLOAT_X8X24_TYPELESS);
			CASE(X32_TYPELESS_G8X24_UINT);
			CASE(R10G10B10A2_TYPELESS);
			CASE(R10G10B10A2_UNORM);
			CASE(R10G10B10A2_UINT);
			CASE(R11G11B10_FLOAT);
			CASE(R8G8B8A8_TYPELESS);
			CASE(R8G8B8A8_UNORM);
			CASE(R8G8B8A8_UNORM_SRGB);
			CASE(R8G8B8A8_UINT);
			CASE(R8G8B8A8_SNORM);
			CASE(R8G8B8A8_SINT);
			CASE(R16G16_TYPELESS);
			CASE(R16G16_FLOAT);
			CASE(R16G16_UNORM);
			CASE(R16G16_UINT);
			CASE(R16G16_SNORM);
			CASE(R16G16_SINT);
			CASE(R32_TYPELESS);
			CASE(D32_FLOAT);
			CASE(R32_FLOAT);
			CASE(R32_UINT);
			CASE(R32_SINT);
			CASE(R24G8_TYPELESS);
			CASE(D24_UNORM_S8_UINT);
			CASE(R24_UNORM_X8_TYPELESS);
			CASE(X24_TYPELESS_G8_UINT);
			CASE(R8G8_TYPELESS);
			CASE(R8G8_UNORM);
			CASE(R8G8_UINT);
			CASE(R8G8_SNORM);
			CASE(R8G8_SINT);
			CASE(R16_TYPELESS);
			CASE(R16_FLOAT);
			CASE(D16_UNORM);
			CASE(R16_UNORM);
			CASE(R16_UINT);
			CASE(R16_SNORM);
			CASE(R16_SINT);
			CASE(R8_TYPELESS);
			CASE(R8_UNORM);
			CASE(R8_UINT);
			CASE(R8_SNORM);
			CASE(R8_SINT);
			CASE(A8_UNORM);
			CASE(R1_UNORM);
			CASE(R9G9B9E5_SHAREDEXP);
			CASE(R8G8_B8G8_UNORM);
			CASE(G8R8_G8B8_UNORM);
			CASE(BC1_TYPELESS);
			CASE(BC1_UNORM);
			CASE(BC1_UNORM_SRGB);
			CASE(BC2_TYPELESS);
			CASE(BC2_UNORM);
			CASE(BC2_UNORM_SRGB);
			CASE(BC3_TYPELESS);
			CASE(BC3_UNORM);
			CASE(BC3_UNORM_SRGB);
			CASE(BC4_TYPELESS);
			CASE(BC4_UNORM);
			CASE(BC4_SNORM);
			CASE(BC5_TYPELESS);
			CASE(BC5_UNORM);
			CASE(BC5_SNORM);
			CASE(B5G6R5_UNORM);
			CASE(B5G5R5A1_UNORM);
			CASE(B8G8R8A8_UNORM);
			CASE(B8G8R8X8_UNORM);
			CASE(R10G10B10_XR_BIAS_A2_UNORM);
			CASE(B8G8R8A8_TYPELESS);
			CASE(B8G8R8A8_UNORM_SRGB);
			CASE(B8G8R8X8_TYPELESS);
			CASE(B8G8R8X8_UNORM_SRGB);
			CASE(BC6H_TYPELESS);
			CASE(BC6H_UF16);
			CASE(BC6H_SF16);
			CASE(BC7_TYPELESS);
			CASE(BC7_UNORM);
			CASE(BC7_UNORM_SRGB);
		// CASE(ETC1_UNORM);
		// CASE(ETC2_UNORM);
		// CASE(ETC2A_UNORM);
		// CASE(ETC2A1_UNORM);
		default:
			return DXGI_FORMAT_UNKNOWN;
		}
#undef FORMAT
	}

	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology(PrimitiveTopology topology)
	{
		switch(topology)
		{
		case PrimitiveTopology::POINT_LIST:
			return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		case PrimitiveTopology::LINE_LIST:
			return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		case PrimitiveTopology::LINE_STRIP:
			return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
		case PrimitiveTopology::LINE_LIST_ADJ:
			return D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
		case PrimitiveTopology::LINE_STRIP_ADJ:
			return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
		case PrimitiveTopology::TRIANGLE_LIST:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case PrimitiveTopology::TRIANGLE_STRIP:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		case PrimitiveTopology::TRIANGLE_LIST_ADJ:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
		case PrimitiveTopology::TRIANGLE_STRIP_ADJ:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
		default:
			DBG_ASSERT(false);
			return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}
	}

	D3D12_RESOURCE_DESC GetResourceDesc(const BufferDesc& desc)
	{
		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		resourceDesc.Width = Core::PotRoundUp(desc.size_, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = GetResourceFlags(desc.bindFlags_);
		return resourceDesc;
	}
	D3D12_RESOURCE_DESC GetResourceDesc(const TextureDesc& desc)
	{
		D3D12_RESOURCE_DESC resourceDesc = {};
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
			resourceDesc.DepthOrArraySize *= 6;
		}
		return resourceDesc;
	}

	D3D12_TEXTURE_ADDRESS_MODE GetAddressingMode(AddressingMode addressMode)
	{
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
			DBG_ASSERT(false);
			return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		}
	};

	D3D12_FILTER GetFilteringMode(FilteringMode min, FilteringMode mag, u32 anisotropy)
	{
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

	D3D12_SAMPLER_DESC GetSampler(const GPU::SamplerState& state)
	{
		D3D12_SAMPLER_DESC desc = {};
		memset(&desc, 0, sizeof(desc));
		desc.AddressU = GetAddressingMode(state.addressU_);
		desc.AddressV = GetAddressingMode(state.addressV_);
		desc.AddressW = GetAddressingMode(state.addressW_);
		desc.Filter = GetFilteringMode(state.minFilter_, state.magFilter_, state.maxAnisotropy_);
		desc.MipLODBias = state.mipLODBias_;
		desc.MaxAnisotropy = state.maxAnisotropy_;

		switch(state.borderColor_)
		{
		case BorderColor::TRANSPARENT:
			desc.BorderColor[0] = 0.0f;
			desc.BorderColor[1] = 0.0f;
			desc.BorderColor[2] = 0.0f;
			desc.BorderColor[3] = 0.0f;
			break;
		case BorderColor::BLACK:
			desc.BorderColor[0] = 0.0f;
			desc.BorderColor[1] = 0.0f;
			desc.BorderColor[2] = 0.0f;
			desc.BorderColor[3] = 1.0f;
			break;
		case BorderColor::WHITE:
			desc.BorderColor[0] = 1.0f;
			desc.BorderColor[1] = 1.0f;
			desc.BorderColor[2] = 1.0f;
			desc.BorderColor[3] = 1.0f;
			break;
		}
		desc.MinLOD = state.minLOD_;
		desc.MaxLOD = state.maxLOD_;
		return desc;
	}

	D3D12_STATIC_SAMPLER_DESC GetStaticSampler(const GPU::SamplerState& state)
	{
		D3D12_STATIC_SAMPLER_DESC desc = {};
		memset(&desc, 0, sizeof(desc));
		desc.AddressU = GetAddressingMode(state.addressU_);
		desc.AddressV = GetAddressingMode(state.addressV_);
		desc.AddressW = GetAddressingMode(state.addressW_);
		desc.Filter = GetFilteringMode(state.minFilter_, state.magFilter_, state.maxAnisotropy_);
		desc.MipLODBias = state.mipLODBias_;
		desc.MaxAnisotropy = state.maxAnisotropy_;

		switch(state.borderColor_)
		{
		case BorderColor::TRANSPARENT:
			desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
			break;
		case BorderColor::BLACK:
			desc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
			break;
		case BorderColor::WHITE:
			desc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
			break;
		}
		desc.MinLOD = state.minLOD_;
		desc.MaxLOD = state.maxLOD_;
		return desc;
	}

	D3D12_RESOURCE_BARRIER TransitionBarrier(
	    ID3D12Resource* res, UINT subRsc, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
	{
		D3D12_RESOURCE_BARRIER barrier;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = res;
		barrier.Transition.Subresource = subRsc;
		barrier.Transition.StateBefore = before;
		barrier.Transition.StateAfter = after;
		return barrier;
	}

	// {BF15EAFD-72B9-4965-B4E1-48549D035999}
	static const GUID GUID_DebugName = {0xbf15eafd, 0x72b9, 0x4965, {0xb4, 0xe1, 0x48, 0x54, 0x9d, 0x3, 0x59, 0x99}};

	void SetObjectName(ID3D12Object* object, const char* name)
	{
#if !defined(FINAL)
		if(name)
		{
			wchar wName[512] = {0};
			Core::StringConvertUTF8toUTF16(name, (i32)strlen(name), wName, 512);
			object->SetName(wName);
			object->SetPrivateData(GUID_DebugName, Core::Min(31, (UINT)strlen(name)), name);
		}
#endif
	}

	void GetObjectName(ID3D12Object* object, char* outName, i32& outSize)
	{
#if !defined(FINAL)
		UINT len = (UINT)outSize;
		memset(outName, 0, outSize);
		object->GetPrivateData(GUID_DebugName, &len, outName);
		outSize = (i32)len;
#endif
	}

	void WaitOnFence(ID3D12Fence* fence, HANDLE event, u64 value)
	{
		if(fence->GetCompletedValue() < value)
		{
			fence->SetEventOnCompletion(value, event);
			::WaitForSingleObject(event, INFINITE);
		}
	}

	void ClearDescriptorRange(ID3D12DescriptorHeap* d3dDescriptorHeap,
	    Core::ArrayView<D3D12DescriptorDebugData> debugDataBase, DescriptorHeapSubType subType, i32 offset,
	    i32 numDescriptors)
	{
		auto d3dDesc = d3dDescriptorHeap->GetDesc();
		ComPtr<ID3D12Device> d3dDevice;
		d3dDescriptorHeap->GetDevice(IID_ID3D12Device, (void**)d3dDevice.GetAddressOf());
		auto descriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(d3dDesc.Type);
		D3D12_CPU_DESCRIPTOR_HANDLE handle = d3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += offset * descriptorSize;
		for(i32 i = offset; i < (offset + numDescriptors); ++i)
		{
			if(subType == DescriptorHeapSubType::INVALID)
			{
				if(debugDataBase)
				{
					debugDataBase[i].subType_ = DescriptorHeapSubType::INVALID;
					debugDataBase[i].resource_ = nullptr;
					sprintf_s(debugDataBase[i].name_.data(), debugDataBase[i].name_.size(), "<INVALID>");
				}

				switch(d3dDesc.Type)
				{
				case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
				{
					D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
					d3dDevice->CreateConstantBufferView(&desc, handle);
					break;
				}
				case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
				{
					D3D12_SAMPLER_DESC desc = {};
					// Setup easy to spot when debugging, but valid default.
					desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
					desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
					desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
					desc.BorderColor[0] = 1.0f;
					desc.BorderColor[1] = 2.0f;
					desc.BorderColor[2] = 3.0f;
					desc.BorderColor[3] = 4.0f;
					d3dDevice->CreateSampler(&desc, handle);
					break;
				}
				case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
				case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
					// Don't need to clear these ranges.
					break;
				default:
					DBG_ASSERT(false);
					break;
				};
			}
			else
			{
				switch(subType)
				{
				case DescriptorHeapSubType::CBV:
				{
					D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
					d3dDevice->CreateConstantBufferView(&desc, handle);
					if(debugDataBase)
					{
						debugDataBase[i].subType_ = DescriptorHeapSubType::CBV;
						debugDataBase[i].resource_ = nullptr;
						sprintf_s(debugDataBase[i].name_.data(), debugDataBase[i].name_.size(), "<NULL CBV>");
					}
					break;
				}
				case DescriptorHeapSubType::SRV:
				{
					D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
					desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
					desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
					desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
					    D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0, D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
					    D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0, D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0);
					d3dDevice->CreateShaderResourceView(nullptr, &desc, handle);
					if(debugDataBase)
					{
						debugDataBase[i].subType_ = DescriptorHeapSubType::SRV;
						debugDataBase[i].resource_ = nullptr;
						sprintf_s(debugDataBase[i].name_.data(), debugDataBase[i].name_.size(), "<NULL SRV>");
					}
					break;
				}
				case DescriptorHeapSubType::UAV:
				{
					D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
					desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
					desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
					d3dDevice->CreateUnorderedAccessView(nullptr, nullptr, &desc, handle);
					if(debugDataBase)
					{
						debugDataBase[i].subType_ = DescriptorHeapSubType::UAV;
						debugDataBase[i].resource_ = nullptr;
						sprintf_s(debugDataBase[i].name_.data(), debugDataBase[i].name_.size(), "<NULL UAV>");
					}
					break;
				}
				case DescriptorHeapSubType::SAMPLER:
				{
					D3D12_SAMPLER_DESC desc = {};
					// Setup easy to spot when debugging, but valid default.
					desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
					desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
					desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
					desc.BorderColor[0] = 1.0f;
					desc.BorderColor[1] = 2.0f;
					desc.BorderColor[2] = 3.0f;
					desc.BorderColor[3] = 4.0f;
					d3dDevice->CreateSampler(&desc, handle);
					if(debugDataBase)
					{
						debugDataBase[i].subType_ = DescriptorHeapSubType::SAMPLER;
						debugDataBase[i].resource_ = nullptr;
						sprintf_s(debugDataBase[i].name_.data(), debugDataBase[i].name_.size(), "<NULL SAMPLER>");
					}
					break;
				}
				default:
					DBG_ASSERT(false);
					break;
				};
			}

			// Advance handle.
			handle.ptr += descriptorSize;
		}
	}

} // namespace GPU
