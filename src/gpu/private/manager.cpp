#include "gpu/manager.h"
#include "gpu/types.h"
#include "gpu/resources.h"

#include "gpu/private/interface.h"

#include "core/array.h"
#include "core/concurrency.h"
#include "core/debug.h"
#include "core/handle.h"
#include "core/vector.h"

#include <utility>

namespace GPU
{
	struct ManagerImpl
	{
		void* deviceWindow_ = nullptr;

		Interface* interface_ = nullptr;

		Core::Mutex mutex_;
		Core::HandleAllocator handles_ = Core::HandleAllocator(ResourceType::MAX);
		Core::Vector<Handle> deferredDeletions_;

		Handle AllocHandle(ResourceType type)
		{
			Core::ScopedMutex lock(mutex_);
			return handles_.Alloc<Handle>(type);
		}

		void ProcessDeletions()
		{
			Core::ScopedMutex lock(mutex_);
			for(auto handle : deferredDeletions_)
			{
				handles_.Free(handle);
			}
		}

		void HandleErrorCode(Handle& handle, ErrorCode errorCode)
		{
			switch(errorCode)
			{
			case ErrorCode::OK:
				break;
			default:
				// TODO: log error.
				handles_.Free(handle);
				handle = Handle();
			}
		}
	};

	Manager::Manager(void* deviceWindow)
	{ //
		impl_ = new ManagerImpl();
		impl_->deviceWindow_ = deviceWindow;
	}

	Manager::~Manager()
	{
#if !defined(FINAL)
		for(i32 i = 0; i < (i32)ResourceType::MAX; ++i)
			DBG_ASSERT_MSG(impl_->handles_.GetTotalHandles(i) == 0, "Handles still remain allocated.");
#endif
		delete impl_;
	}

	i32 Manager::EnumerateAdapters(AdapterInfo* outAdapters, i32 maxAdapters)
	{
		DBG_ASSERT(impl_->interface_);
		return impl_->interface_->EnumerateAdapters(outAdapters, maxAdapters);
	}

	Handle Manager::CreateSwapChain(const SwapChainDesc& desc, const char* debugName)
	{
		DBG_ASSERT(impl_->interface_);
		Handle handle = impl_->AllocHandle(ResourceType::SWAP_CHAIN);
		impl_->HandleErrorCode(handle, impl_->interface_->CreateSwapChain(handle, desc, debugName));
		return handle;
	}

	Handle Manager::CreateBuffer(const BufferDesc& desc, const void* initialData, const char* debugName)
	{
		DBG_ASSERT(impl_->interface_);
		Handle handle = impl_->AllocHandle(ResourceType::BUFFER);
		impl_->HandleErrorCode(handle, impl_->interface_->CreateBuffer(handle, desc, initialData, debugName));
		return handle;
	}

	Handle Manager::CreateTexture(
	    const TextureDesc& desc, const TextureSubResourceData* initialData, const char* debugName)
	{
		DBG_ASSERT(impl_->interface_);
		Handle handle = impl_->AllocHandle(ResourceType::TEXTURE);
		impl_->HandleErrorCode(handle, impl_->interface_->CreateTexture(handle, desc, initialData, debugName));
		return handle;
	}

	Handle Manager::CreateSamplerState(const SamplerState& state, const char* debugName)
	{
		DBG_ASSERT(impl_->interface_);
		Handle handle = impl_->AllocHandle(ResourceType::SAMPLER_STATE);
		impl_->HandleErrorCode(handle, impl_->interface_->CreateSamplerState(handle, state, debugName));
		return handle;
	}

	Handle Manager::CreateShader(const ShaderDesc& desc, const char* debugName)
	{
		DBG_ASSERT(impl_->interface_);
		Handle handle = impl_->AllocHandle(ResourceType::SHADER);
		impl_->HandleErrorCode(handle, impl_->interface_->CreateShader(handle, desc, debugName));
		return handle;
	}

	Handle Manager::CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc, const char* debugName)
	{
		DBG_ASSERT(impl_->interface_);
		Handle handle = impl_->AllocHandle(ResourceType::GRAPHICS_PIPELINE_STATE);
		impl_->HandleErrorCode(handle, impl_->interface_->CreateGraphicsPipelineState(handle, desc, debugName));
		return handle;
	}

	Handle Manager::CreateComputePipelineState(const ComputePipelineStateDesc& desc, const char* debugName)
	{
		DBG_ASSERT(impl_->interface_);
		Handle handle = impl_->AllocHandle(ResourceType::COMPUTE_PIPELINE_STATE);
		impl_->HandleErrorCode(handle, impl_->interface_->CreateComputePipelineState(handle, desc, debugName));
		return handle;
	}

	Handle Manager::CreatePipelineBindingSet(const PipelineBindingSetDesc& desc, const char* debugName)
	{
		DBG_ASSERT(impl_->interface_);
		Handle handle = impl_->AllocHandle(ResourceType::PIPELINE_BINDING_SET);
		impl_->HandleErrorCode(handle, impl_->interface_->CreatePipelineBindingSet(handle, desc, debugName));
		return handle;
	}

	Handle Manager::CreateDrawBindingSet(const DrawBindingSetDesc& desc, const char* debugName)
	{
		DBG_ASSERT(impl_->interface_);
		Handle handle = impl_->AllocHandle(ResourceType::DRAW_BINDING_SET);
		impl_->HandleErrorCode(handle, impl_->interface_->CreateDrawBindingSet(handle, desc, debugName));
		return handle;
	}

	Handle Manager::CreateCommandList(const char* debugName)
	{
		DBG_ASSERT(impl_->interface_);
		Handle handle = impl_->AllocHandle(ResourceType::COMMAND_LIST);
		impl_->HandleErrorCode(handle, impl_->interface_->CreateCommandList(handle, debugName));
		return handle;
	}

	Handle Manager::CreateFence(const char* debugName)
	{
		DBG_ASSERT(impl_->interface_);
		Handle handle = impl_->AllocHandle(ResourceType::FENCE);
		impl_->HandleErrorCode(handle, impl_->interface_->CreateFence(handle, debugName));
		return handle;
	}

	void Manager::DestroyResource(Handle handle) { impl_->deferredDeletions_.push_back(handle); }

	bool Manager::IsValidHandle(Handle handle) const
	{ //
		return impl_->handles_.IsValid(handle);
	}

} // namespace GPU
