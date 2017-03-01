#pragma once

#include <dxgi1_4.h>
#include <dxgiformat.h>
#include <d3d12.h>
#include <wrl.h>

typedef HRESULT (WINAPI* PFN_CREATE_DXGI_FACTORY)(REFIID _riid, void** _factory);

namespace GPU
{
	using Microsoft::WRL::ComPtr;
} // namespace GPU
