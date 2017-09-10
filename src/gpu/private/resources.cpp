#include "gpu/resources.h"

#if !CODE_INLINE
#include "gpu/private/resources.inl"
#endif

#pragma once

#include "core/debug.h"
#include "gpu/manager.h"

namespace GPU
{
	namespace Binding
	{
		BindingCBV CBuffer(GPU::Handle res, i32 offset, i32 size)
		{
			DBG_ASSERT(!res || GPU::Manager::GetHandleAllocator().IsValid(res));
			BindingBuffer binding;
			binding.resource_ = res;
			binding.offset_ = offset;
			binding.size_ = size;
			return binding;
		}

		BindingSRV Buffer(
		    GPU::Handle res, GPU::Format format, i32 firstElement, i32 numElements, i32 structureByteStride)
		{
			DBG_ASSERT(!res || GPU::Manager::GetHandleAllocator().IsValid(res));
			BindingSRV binding;
			binding.resource_ = res;
			binding.format_ = format;
			binding.dimension_ = GPU::ViewDimension::BUFFER;
			binding.mostDetailedMip_FirstElement_ = firstElement;
			binding.mipLevels_NumElements_ = numElements;
			binding.structureByteStride_ = structureByteStride;
			return binding;
		}

		BindingSRV Texture1D(
		    GPU::Handle res, GPU::Format format, i32 mostDetailedMip, i32 mipLevels, f32 resourceMinLODClamp)
		{
			DBG_ASSERT(!res || GPU::Manager::GetHandleAllocator().IsValid(res));
			BindingSRV binding;
			binding.resource_ = res;
			binding.format_ = format;
			binding.dimension_ = GPU::ViewDimension::TEX1D;
			binding.mostDetailedMip_FirstElement_ = mostDetailedMip;
			binding.mipLevels_NumElements_ = mipLevels;
			binding.resourceMinLODClamp_ = resourceMinLODClamp;
			return binding;
		}

		BindingSRV Texture1DArray(GPU::Handle res, GPU::Format format, i32 mostDetailedMip, i32 mipLevels,
		    i32 firstArraySlice, i32 arraySize, f32 resourceMinLODClamp)
		{
			DBG_ASSERT(!res || GPU::Manager::GetHandleAllocator().IsValid(res));
			BindingSRV binding;
			binding.resource_ = res;
			binding.format_ = format;
			binding.dimension_ = GPU::ViewDimension::TEX1D_ARRAY;
			binding.mostDetailedMip_FirstElement_ = mostDetailedMip;
			binding.mipLevels_NumElements_ = mipLevels;
			binding.firstArraySlice_ = firstArraySlice;
			binding.arraySize_ = arraySize;
			binding.resourceMinLODClamp_ = resourceMinLODClamp;
			return binding;
		}

		BindingSRV Texture2D(GPU::Handle res, GPU::Format format, i32 mostDetailedMip, i32 mipLevels, i32 planeSlice,
		    f32 resourceMinLODClamp)
		{
			DBG_ASSERT(!res || GPU::Manager::GetHandleAllocator().IsValid(res));
			BindingSRV binding;
			binding.resource_ = res;
			binding.format_ = format;
			binding.dimension_ = GPU::ViewDimension::TEX2D;
			binding.mostDetailedMip_FirstElement_ = mostDetailedMip;
			binding.mipLevels_NumElements_ = mipLevels;
			binding.planeSlice_ = planeSlice;
			binding.resourceMinLODClamp_ = resourceMinLODClamp;
			return binding;
		}

		BindingSRV Texture2DArray(GPU::Handle res, GPU::Format format, i32 mostDetailedMip, i32 mipLevels,
		    i32 firstArraySlice, i32 arraySize, i32 planeSlice, f32 resourceMinLODClamp)
		{
			DBG_ASSERT(!res || GPU::Manager::GetHandleAllocator().IsValid(res));
			BindingSRV binding;
			binding.resource_ = res;
			binding.format_ = format;
			binding.dimension_ = GPU::ViewDimension::TEX2D_ARRAY;
			binding.mostDetailedMip_FirstElement_ = mostDetailedMip;
			binding.mipLevels_NumElements_ = mipLevels;
			binding.firstArraySlice_ = firstArraySlice;
			binding.arraySize_ = arraySize;
			binding.planeSlice_ = planeSlice;
			binding.resourceMinLODClamp_ = resourceMinLODClamp;
			return binding;
		}

		BindingSRV Texture3D(
		    GPU::Handle res, GPU::Format format, i32 mostDetailedMip, i32 mipLevels, f32 resourceMinLODClamp)
		{
			DBG_ASSERT(!res || GPU::Manager::GetHandleAllocator().IsValid(res));
			BindingSRV binding;
			binding.resource_ = res;
			binding.format_ = format;
			binding.dimension_ = GPU::ViewDimension::TEX3D;
			binding.mostDetailedMip_FirstElement_ = mostDetailedMip;
			binding.mipLevels_NumElements_ = mipLevels;
			binding.resourceMinLODClamp_ = resourceMinLODClamp;
			return binding;
		}

		BindingSRV TextureCube(
		    GPU::Handle res, GPU::Format format, i32 mostDetailedMip, i32 mipLevels, f32 resourceMinLODClamp)
		{
			DBG_ASSERT(!res || GPU::Manager::GetHandleAllocator().IsValid(res));
			BindingSRV binding;
			binding.resource_ = res;
			binding.format_ = format;
			binding.dimension_ = GPU::ViewDimension::TEXCUBE;
			binding.mostDetailedMip_FirstElement_ = mostDetailedMip;
			binding.mipLevels_NumElements_ = mipLevels;
			binding.resourceMinLODClamp_ = resourceMinLODClamp;
			return binding;
		}

		BindingSRV TextureCubeArray(GPU::Handle res, GPU::Format format, i32 mostDetailedMip, i32 mipLevels,
		    i32 first2DArrayFace, i32 numCubes, f32 resourceMinLODClamp)
		{
			DBG_ASSERT(!res || GPU::Manager::GetHandleAllocator().IsValid(res));
			BindingSRV binding;
			binding.resource_ = res;
			binding.format_ = format;
			binding.dimension_ = GPU::ViewDimension::TEXCUBE_ARRAY;
			binding.mostDetailedMip_FirstElement_ = mostDetailedMip;
			binding.mipLevels_NumElements_ = mipLevels;
			binding.firstArraySlice_ = first2DArrayFace;
			binding.arraySize_ = numCubes;
			binding.resourceMinLODClamp_ = resourceMinLODClamp;
			return binding;
		}

		BindingUAV RWBuffer(
		    GPU::Handle res, GPU::Format format, i32 firstElement, i32 numElements, i32 structureByteStride)
		{
			DBG_ASSERT(!res || GPU::Manager::GetHandleAllocator().IsValid(res));
			BindingUAV binding;
			binding.resource_ = res;
			binding.format_ = format;
			binding.dimension_ = GPU::ViewDimension::BUFFER;
			binding.mipSlice_FirstElement_ = firstElement;
			binding.firstArraySlice_FirstWSlice_NumElements_ = numElements;
			binding.structureByteStride_ = structureByteStride;
			return binding;
		}

		BindingUAV RWTexture1D(GPU::Handle res, GPU::Format format, i32 mipSlice)
		{
			DBG_ASSERT(!res || GPU::Manager::GetHandleAllocator().IsValid(res));
			BindingUAV binding;
			binding.resource_ = res;
			binding.format_ = format;
			binding.dimension_ = GPU::ViewDimension::TEX1D;
			binding.mipSlice_FirstElement_ = mipSlice;
			return binding;
		}

		BindingUAV RWTexture1DArray(
		    GPU::Handle res, GPU::Format format, i32 mipSlice, i32 firstArraySlice, i32 arraySize)
		{
			DBG_ASSERT(!res || GPU::Manager::GetHandleAllocator().IsValid(res));
			BindingUAV binding;
			binding.resource_ = res;
			binding.format_ = format;
			binding.dimension_ = GPU::ViewDimension::TEX1D_ARRAY;
			binding.mipSlice_FirstElement_ = mipSlice;
			binding.firstArraySlice_FirstWSlice_NumElements_ = firstArraySlice;
			binding.arraySize_PlaneSlice_WSize_ = arraySize;
			return binding;
		}

		BindingUAV RWTexture2D(GPU::Handle res, GPU::Format format, i32 mipSlice, i32 planeSlice)
		{
			DBG_ASSERT(!res || GPU::Manager::GetHandleAllocator().IsValid(res));
			BindingUAV binding;
			binding.resource_ = res;
			binding.format_ = format;
			binding.dimension_ = GPU::ViewDimension::TEX2D;
			binding.mipSlice_FirstElement_ = mipSlice;
			binding.arraySize_PlaneSlice_WSize_ = planeSlice;
			return binding;
		}

		BindingUAV RWTexture2DArray(
		    GPU::Handle res, GPU::Format format, i32 mipSlice, i32 planeSlice, i32 firstArraySlice, i32 arraySize)
		{
			DBG_ASSERT(!res || GPU::Manager::GetHandleAllocator().IsValid(res));
			BindingUAV binding;
			binding.resource_ = res;
			binding.format_ = format;
			binding.dimension_ = GPU::ViewDimension::TEX2D_ARRAY;
			binding.mipSlice_FirstElement_ = mipSlice;
			binding.arraySize_PlaneSlice_WSize_ = planeSlice;
			binding.firstArraySlice_FirstWSlice_NumElements_ = firstArraySlice;
			binding.arraySize_PlaneSlice_WSize_ = arraySize;
			return binding;
		}

		BindingUAV RWTexture3D(GPU::Handle res, GPU::Format format, i32 mipSlice, i32 firstWSlice, i32 wSize)
		{
			DBG_ASSERT(!res || GPU::Manager::GetHandleAllocator().IsValid(res));
			BindingUAV binding;
			binding.resource_ = res;
			binding.format_ = format;
			binding.dimension_ = GPU::ViewDimension::TEX3D;
			binding.mipSlice_FirstElement_ = mipSlice;
			binding.firstArraySlice_FirstWSlice_NumElements_ = firstWSlice;
			binding.arraySize_PlaneSlice_WSize_ = wSize;
			return binding;
		}
	} // namespace Binding

} // namespace GPU
