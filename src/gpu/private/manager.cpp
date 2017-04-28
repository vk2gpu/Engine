#include "gpu/manager.h"
#include "gpu/types.h"
#include "gpu/resources.h"

#include "gpu/backend.h"

#include "core/array.h"
#include "core/concurrency.h"
#include "core/debug.h"
#include "core/handle.h"
#include "core/library.h"
#include "core/misc.h"
#include "core/vector.h"

#include "gpu/private/renderdoc_app.h"

#include "plugin/manager.h"

#include <utility>

namespace RenderDoc
{
	pRENDERDOC_GetAPI RENDERDOC_GetAPI = nullptr;

	Core::LibHandle renderDocLib_ = 0;
	RENDERDOC_API_1_1_1* renderDocAPI_ = nullptr;
	void Load()
	{
		// Attempt to load renderdoc from various paths.
		if(!renderDocLib_)
			renderDocLib_ = Core::LibraryOpen("renderdoc.dll");
		if(!renderDocLib_)
			renderDocLib_ = Core::LibraryOpen("C:\\Program Files\\RenderDoc\\renderdoc.dll");

		if(renderDocLib_)
		{
			RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)Core::LibrarySymbol(renderDocLib_, "RENDERDOC_GetAPI");
			if(RENDERDOC_GetAPI)
			{
				RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_1, (void**)&renderDocAPI_);

				if(renderDocAPI_)
				{
					renderDocAPI_->SetCaptureOptionU32(eRENDERDOC_Option_CaptureAllCmdLists, 1);
					renderDocAPI_->SetCaptureOptionU32(eRENDERDOC_Option_SaveAllInitials, 1);
				}
			}
		}
	}

	void BeginCapture(const char* name)
	{
		if(!renderDocAPI_)
			return;

		if(name == nullptr)
			renderDocAPI_->SetLogFilePathTemplate("Capture");
		else
			renderDocAPI_->SetLogFilePathTemplate(name);

		renderDocAPI_->StartFrameCapture(nullptr, nullptr);
	}

	void EndCapture()
	{
		if(!renderDocAPI_)
			return;

		auto retVal = renderDocAPI_->EndFrameCapture(nullptr, nullptr);
		if(retVal)
		{
			char logFile[4096] = {0};
			u32 pathLength = 4096;

			u32 numCaptures = renderDocAPI_->GetNumCaptures();
			if(renderDocAPI_->GetCapture(numCaptures - 1, logFile, &pathLength, nullptr))
			{
				DBG_LOG("RenderDoc capture successful: %s\n", logFile);
			}
		}
	}
}

namespace GPU
{
	struct ManagerImpl
	{
		void* deviceWindow_ = nullptr;
		DebuggerIntegrationFlags debuggerIntegration_ = DebuggerIntegrationFlags::NONE;

		BackendPlugin plugin_;
		IBackend* backend_ = nullptr;

		Core::Mutex mutex_;
		Core::HandleAllocator handles_ = Core::HandleAllocator(ResourceType::MAX);
		Core::Vector<Handle> deferredDeletions_;

		ManagerImpl(const SetupParams& setupParams)
			: deviceWindow_(setupParams.deviceWindow_)
			, debuggerIntegration_(setupParams.debuggerIntegration_)
		{
			// Create matching backend.
			Core::Vector<BackendPlugin> plugins;

			auto found = Plugin::Manager::GetPlugins<BackendPlugin>(nullptr, 0);
			DBG_ASSERT(found > 0);

			plugins.resize(found);
			found = Plugin::Manager::GetPlugins(plugins.data(), found);
			DBG_ASSERT(found > 0);

			for(const auto& plugin : plugins)
			{
				if(setupParams.api_ == nullptr || strcmp(setupParams.api_, plugin.api_) == 0)
				{
					plugin_ = plugin;
					backend_ = plugin_.CreateBackend(setupParams);
					break;
				}
			}

			// Check for failure.
			if(backend_ == nullptr)
			{
				DBG_LOG("No backend created. Valid APIs:\n");
				for(const auto& plugin : plugins)
				{
					DBG_LOG(" - %s (%s)\n", plugin.api_, plugin.name_);
				}
				DBG_BREAK;
			}
		}

		~ManagerImpl()
		{
			plugin_.DestroyBackend(backend_);
		}

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
				backend_->DestroyResource(handle);
				handles_.Free(handle);
			}
		}

		bool HandleErrorCode(Handle& handle, ErrorCode errorCode)
		{
			switch(errorCode)
			{
			case ErrorCode::OK:
				return true;
				break;
			default:
				// TODO: log error.
				DBG_BREAK;
				handles_.Free(handle);
				handle = Handle();
				return false;
			}
		}
	};

	ManagerImpl* impl_ = nullptr;

	void Manager::Initialize(const SetupParams& setupParams)
	{
		DBG_ASSERT(impl_ == nullptr);
		DBG_ASSERT(Plugin::Manager::IsInitialized());
		impl_ = new ManagerImpl(setupParams);
	}

	void Manager::Finalize()
	{
		DBG_ASSERT(impl_);
		impl_->ProcessDeletions();

#if !defined(FINAL)
		for(i32 i = 0; i < (i32)ResourceType::MAX; ++i)
			DBG_ASSERT_MSG(
			    impl_->handles_.GetTotalHandles(i) == 0, "Handles still remain allocated for resource type %u", i);
#endif
		delete impl_;
		impl_ = nullptr;
	}

	bool Manager::IsInitialized()
	{ //
		return !!impl_;
	}

	i32 Manager::EnumerateAdapters(AdapterInfo* outAdapters, i32 maxAdapters)
	{
		DBG_ASSERT(IsInitialized());
		return impl_->backend_->EnumerateAdapters(outAdapters, maxAdapters);
	}

	ErrorCode Manager::CreateAdapter(i32 adapterIdx)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(!IsAdapterCreated());
		DBG_ASSERT(impl_->backend_);
		return impl_->backend_->Initialize(adapterIdx);
	}

	bool Manager::IsAdapterCreated()
	{ //
		DBG_ASSERT(IsInitialized());
		return impl_->backend_ && impl_->backend_->IsInitialized();
	}

	Handle Manager::CreateSwapChain(const SwapChainDesc& desc, const char* debugName)
	{
		DBG_ASSERT(IsInitialized());
		Handle handle = impl_->AllocHandle(ResourceType::SWAP_CHAIN);
		impl_->HandleErrorCode(handle, impl_->backend_->CreateSwapChain(handle, desc, debugName));
		return handle;
	}

	Handle Manager::CreateBuffer(const BufferDesc& desc, const void* initialData, const char* debugName)
	{
		DBG_ASSERT(IsInitialized());
		Handle handle = impl_->AllocHandle(ResourceType::BUFFER);
		impl_->HandleErrorCode(handle, impl_->backend_->CreateBuffer(handle, desc, initialData, debugName));
		return handle;
	}

	Handle Manager::CreateTexture(
	    const TextureDesc& desc, const TextureSubResourceData* initialData, const char* debugName)
	{
		DBG_ASSERT(IsInitialized());
		Handle handle = impl_->AllocHandle(ResourceType::TEXTURE);
		impl_->HandleErrorCode(handle, impl_->backend_->CreateTexture(handle, desc, initialData, debugName));
		return handle;
	}

	Handle Manager::CreateSamplerState(const SamplerState& state, const char* debugName)
	{
		DBG_ASSERT(IsInitialized());
		Handle handle = impl_->AllocHandle(ResourceType::SAMPLER_STATE);
		impl_->HandleErrorCode(handle, impl_->backend_->CreateSamplerState(handle, state, debugName));
		return handle;
	}

	Handle Manager::CreateShader(const ShaderDesc& desc, const char* debugName)
	{
		DBG_ASSERT(IsInitialized());
		Handle handle = impl_->AllocHandle(ResourceType::SHADER);
		impl_->HandleErrorCode(handle, impl_->backend_->CreateShader(handle, desc, debugName));
		return handle;
	}

	Handle Manager::CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc, const char* debugName)
	{
		DBG_ASSERT(IsInitialized());
		Handle handle = impl_->AllocHandle(ResourceType::GRAPHICS_PIPELINE_STATE);
		impl_->HandleErrorCode(handle, impl_->backend_->CreateGraphicsPipelineState(handle, desc, debugName));
		return handle;
	}

	Handle Manager::CreateComputePipelineState(const ComputePipelineStateDesc& desc, const char* debugName)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(impl_->backend_);
		Handle handle = impl_->AllocHandle(ResourceType::COMPUTE_PIPELINE_STATE);
		impl_->HandleErrorCode(handle, impl_->backend_->CreateComputePipelineState(handle, desc, debugName));
		return handle;
	}

	Handle Manager::CreatePipelineBindingSet(const PipelineBindingSetDesc& desc, const char* debugName)
	{
		DBG_ASSERT(IsInitialized());
		Handle handle = impl_->AllocHandle(ResourceType::PIPELINE_BINDING_SET);
		impl_->HandleErrorCode(handle, impl_->backend_->CreatePipelineBindingSet(handle, desc, debugName));
		return handle;
	}

	Handle Manager::CreateDrawBindingSet(const DrawBindingSetDesc& desc, const char* debugName)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(impl_->backend_);
		Handle handle = impl_->AllocHandle(ResourceType::DRAW_BINDING_SET);
		impl_->HandleErrorCode(handle, impl_->backend_->CreateDrawBindingSet(handle, desc, debugName));
		return handle;
	}

	Handle Manager::CreateFrameBindingSet(const FrameBindingSetDesc& desc, const char* debugName)
	{
		DBG_ASSERT(IsInitialized());
		Handle handle = impl_->AllocHandle(ResourceType::FRAME_BINDING_SET);
		impl_->HandleErrorCode(handle, impl_->backend_->CreateFrameBindingSet(handle, desc, debugName));
		return handle;
	}

	Handle Manager::CreateCommandList(const char* debugName)
	{
		DBG_ASSERT(IsInitialized());
		Handle handle = impl_->AllocHandle(ResourceType::COMMAND_LIST);
		impl_->HandleErrorCode(handle, impl_->backend_->CreateCommandList(handle, debugName));
		return handle;
	}

	Handle Manager::CreateFence(const char* debugName)
	{
		DBG_ASSERT(IsInitialized());
		Handle handle = impl_->AllocHandle(ResourceType::FENCE);
		impl_->HandleErrorCode(handle, impl_->backend_->CreateFence(handle, debugName));
		return handle;
	}

	void Manager::DestroyResource(Handle handle)
	{
		DBG_ASSERT(IsInitialized());
		Core::ScopedMutex lock(impl_->mutex_);
		impl_->deferredDeletions_.push_back(handle);
	}

	bool Manager::CompileCommandList(Handle handle, const CommandList& commandList)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(handle.GetType() == ResourceType::COMMAND_LIST);
		return impl_->HandleErrorCode(handle, impl_->backend_->CompileCommandList(handle, commandList));
	}

	bool Manager::SubmitCommandList(Handle handle)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(handle.GetType() == ResourceType::COMMAND_LIST);
		return impl_->HandleErrorCode(handle, impl_->backend_->SubmitCommandList(handle));
	}

	bool Manager::PresentSwapChain(Handle handle)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(handle.GetType() == ResourceType::SWAP_CHAIN);
		return impl_->HandleErrorCode(handle, impl_->backend_->PresentSwapChain(handle));
	}

	bool Manager::ResizeSwapChain(Handle handle, i32 width, i32 height)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(handle.GetType() == ResourceType::SWAP_CHAIN);
		return impl_->HandleErrorCode(handle, impl_->backend_->ResizeSwapChain(handle, width, height));
	}

	void Manager::NextFrame() { impl_->backend_->NextFrame(); }

	bool Manager::IsValidHandle(Handle handle)
	{
		DBG_ASSERT(IsInitialized());
		return impl_->handles_.IsValid(handle);
	}

	const Core::HandleAllocator& Manager::GetHandleAllocator()
	{
		DBG_ASSERT(IsInitialized());
		return impl_->handles_;
	}

	void Manager::BeginDebugCapture(const char* name)
	{
		DBG_ASSERT(IsInitialized());
		if(Core::ContainsAllFlags(impl_->debuggerIntegration_, DebuggerIntegrationFlags::RENDERDOC))
		{
			RenderDoc::BeginCapture(name);
		}
	}

	void Manager::EndDebugCapture()
	{
		DBG_ASSERT(IsInitialized());
		if(Core::ContainsAllFlags(impl_->debuggerIntegration_, DebuggerIntegrationFlags::RENDERDOC))
		{
			RenderDoc::EndCapture();
		}
	}

} // namespace GPU
