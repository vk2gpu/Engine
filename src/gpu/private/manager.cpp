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
#include "core/string.h"
#include "core/timer.h"
#include "core/vector.h"

#include "renderdoc_app.h"

#include "plugin/manager.h"

#include "Remotery.h"

#include <cstdarg>
#include <utility>

#define ENABLE_TEMP_DEBUG_INFO (0)

/**
 * GPU debug information.
 */
namespace GPU
{
#if !defined(_RELEASE)
	static const i32 MAX_DEBUG_NAME_LENGTH = 256;

	struct ResourceDebugInfo
	{
		ResourceDebugInfo() = default;
		ResourceDebugInfo(const char* name, Handle handle)
		{
			sprintf_s(name_.data(), name_.size(), "%s [%u, %u]", name, (u32)handle.GetIndex(), (u32)handle.GetType());
			Core::GetCallstack(2, creationCallstack_.data(), creationCallstack_.size());
		}

		Core::Array<char, MAX_DEBUG_NAME_LENGTH> name_ = {};
		Core::Array<void*, 16> creationCallstack_ = {};
	};

	using ResourceDebugInfos = Core::Vector<ResourceDebugInfo>;
	GPU_DLL Core::Mutex resourceDebugInfoMutex_;
	GPU_DLL Core::Array<ResourceDebugInfos, (i32)GPU::ResourceType::MAX> resourceDebugInfo_;

	GPU_DLL ResourceDebugInfo GetDebugInfo(GPU::Handle handle)
	{
		return resourceDebugInfo_[(i32)handle.GetType()][handle.GetIndex()];
	}
#endif // !defined(_RELEASE)

	GPU_DLL const char* SetDebugInfo(GPU::Handle handle, const char* debugName)
	{
#if !defined(_RELEASE)
		Core::ScopedMutex lock(resourceDebugInfoMutex_);

		auto& resourceDebugInfo = resourceDebugInfo_[(i32)handle.GetType()];
		if(resourceDebugInfo.size() <= handle.GetIndex())
		{
			resourceDebugInfo.resize(Core::PotRoundUp(handle.GetIndex() + 1, 32));
		}
		resourceDebugInfo[handle.GetIndex()] = ResourceDebugInfo(debugName, handle);
		return resourceDebugInfo[handle.GetIndex()].name_.data();
#else
		return nullptr;
#endif // !defined(_RELEASE)
	}

// Helper macro for formatting and setting debug name.
#if !defined(_RELEASE)
#define SET_DEBUG_INFO()                                                                                               \
	Core::Array<char, MAX_DEBUG_NAME_LENGTH> debugName = {};                                                           \
	va_list argList;                                                                                                   \
	va_start(argList, debugFmt);                                                                                       \
	vsprintf_s(debugName.data(), debugName.size(), debugFmt, argList);                                                 \
	const char* fullDebugName = SetDebugInfo(handle, debugName.data());                                                \
	va_end(argList);

#else
#define SET_DEBUG_INFO() static const char* fullDebugName = "";
#endif
} // namespace GPU

namespace RenderDoc
{
	pRENDERDOC_GetAPI RENDERDOC_GetAPI = nullptr;

	Core::LibHandle renderDocLib_ = 0;
	RENDERDOC_API_1_1_1* renderDocAPI_ = nullptr;
	void Load()
	{
		const char* renderDocPaths[] = {
		    "renderdoc.dll", "C:\\Dev\\RenderDoc\\renderdoc.dll", "C:\\Program Files\\RenderDoc\\renderdoc.dll"};

		// Attempt to load renderdoc from various paths.
		for(auto renderDocPath : renderDocPaths)
		{
			renderDocLib_ = Core::LibraryOpen(renderDocPath);
			if(renderDocLib_)
				break;
		}

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
				else
				{
					Core::LibraryClose(renderDocLib_);
					renderDocLib_ = nullptr;
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

	void Open(bool quitOnOpen)
	{
		if(!renderDocAPI_)
			return;

		if(!renderDocAPI_->IsTargetControlConnected())
		{
			renderDocAPI_->LaunchReplayUI(1, nullptr);
		}

		if(quitOnOpen)
		{
			Core::Timer timer;
			timer.Mark();
			while(timer.GetTime() < 30.0f)
			{
				Core::Sleep(1.0);
				if(renderDocAPI_->IsTargetControlConnected())
				{
					Core::Sleep(5.0);
					exit(0);
				}
			}

			DBG_LOG("RenderDoc has not connected within timeout.");
		}
	}

	void Trigger()
	{
		if(!renderDocAPI_)
			return;

		renderDocAPI_->TriggerCapture();
	}
}

namespace GPU
{
	struct ManagerImpl
	{
		void* deviceWindow_ = nullptr;
		DebugFlags debugFlags_ = DebugFlags::NONE;

		BackendPlugin plugin_;
		IBackend* backend_ = nullptr;

		Core::Mutex mutex_;
		Core::HandleAllocator handles_ = Core::HandleAllocator(ResourceType::MAX);

		Core::Array<Core::Vector<Handle>, MAX_GPU_FRAMES> deferredDeletions_ = {};
		i64 frameIdx_ = 0;

		ManagerImpl(const SetupParams& setupParams)
		    : deviceWindow_(setupParams.deviceWindow_)
		    , debugFlags_(setupParams.debugFlags_)
		{
			// Create matching backend.
			Core::Vector<BackendPlugin> plugins;

			if(Core::ContainsAnyFlags(debugFlags_, DebugFlags::RENDERDOC))
			{
				RenderDoc::Load();
			}

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
				DBG_ASSERT(false);
			}
		}

		~ManagerImpl() { plugin_.DestroyBackend(backend_); }

		Handle AllocHandle(ResourceType type)
		{
			Core::ScopedMutex lock(mutex_);
			return handles_.Alloc<Handle>(type);
		}

		void ProcessDeletions()
		{
			Core::ScopedMutex lock(mutex_);
			auto& deletions = deferredDeletions_[frameIdx_ % deferredDeletions_.size()];
			for(auto handle : deletions)
			{
				backend_->DestroyResource(handle);
				handles_.Free(handle);
			}
			deletions.clear();
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
				if(Core::IsDebuggerAttached())
				{
					DBG_ASSERT(false);
				}
				handles_.Free(handle);
				handle = Handle();
				return false;
			}
		}

		bool HandleErrorCode(ErrorCode errorCode)
		{
			switch(errorCode)
			{
			case ErrorCode::OK:
				return true;
				break;
			default:
				// TODO: log error.
				if(Core::IsDebuggerAttached())
				{
					DBG_ASSERT(false);
				}
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

		for(i32 idx = 0; idx < MAX_GPU_FRAMES; ++idx)
			NextFrame();

#if !defined(_RELEASE)
		bool handlesAlive = false;
		for(i32 i = 0; i < (i32)ResourceType::MAX; ++i)
		{
			handlesAlive |= impl_->handles_.GetTotalHandles(i) > 0;
		}

		if(handlesAlive)
		{
			Core::Log("GPU Handles still allocated:\n");

			for(i32 i = 0; i < (i32)ResourceType::MAX; ++i)
			{
				Core::Log(" - Type %u:\n", i);
				if(impl_->handles_.GetTotalHandles(i) > 0)
				{
					for(i32 resIdx = 0; resIdx < impl_->handles_.GetMaxHandleIndex(i); ++resIdx)
					{
						if(impl_->handles_.IsHandleIndexAllocated(i, resIdx))
						{
							const char* debugName = resourceDebugInfo_[i][resIdx].name_.data();
							Core::Log(" - - %s\n", debugName);
						}
					}
				}
			}
		}

		DBG_ASSERT(!handlesAlive);
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

	Handle Manager::CreateSwapChain(const SwapChainDesc& desc, const char* debugFmt, ...)
	{
		DBG_ASSERT(IsInitialized());
		rmt_ScopedCPUSample(GPU_CreateSwapChain, RMTSF_None);
		Handle handle = impl_->AllocHandle(ResourceType::SWAP_CHAIN);
		SET_DEBUG_INFO();
		impl_->HandleErrorCode(handle, impl_->backend_->CreateSwapChain(handle, desc, fullDebugName));
		return handle;
	}

	Handle Manager::CreateBuffer(const BufferDesc& desc, const void* initialData, const char* debugFmt, ...)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(desc.size_ > 0);

		rmt_ScopedCPUSample(GPU_CreateBuffer, RMTSF_None);
		Handle handle = impl_->AllocHandle(ResourceType::BUFFER);
		SET_DEBUG_INFO();
		impl_->HandleErrorCode(handle, impl_->backend_->CreateBuffer(handle, desc, initialData, fullDebugName));
		return handle;
	}

	Handle Manager::CreateTexture(
	    const TextureDesc& desc, const TextureSubResourceData* initialData, const char* debugFmt, ...)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(desc.width_ > 0);
		DBG_ASSERT(desc.height_ > 0);
		DBG_ASSERT(desc.depth_ > 0);
		DBG_ASSERT(desc.levels_ > 0);
		DBG_ASSERT(desc.elements_ > 0);

		rmt_ScopedCPUSample(GPU_CreateTexture, RMTSF_None);
		Handle handle = impl_->AllocHandle(ResourceType::TEXTURE);
		SET_DEBUG_INFO();
		impl_->HandleErrorCode(handle, impl_->backend_->CreateTexture(handle, desc, initialData, fullDebugName));
		return handle;
	}

	Handle Manager::CreateShader(const ShaderDesc& desc, const char* debugFmt, ...)
	{
		DBG_ASSERT(IsInitialized());
		rmt_ScopedCPUSample(GPU_CreateShader, RMTSF_None);
		Handle handle = impl_->AllocHandle(ResourceType::SHADER);
		SET_DEBUG_INFO();
		impl_->HandleErrorCode(handle, impl_->backend_->CreateShader(handle, desc, fullDebugName));
		return handle;
	}

	Handle Manager::CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc, const char* debugFmt, ...)
	{
		DBG_ASSERT(IsInitialized());
		rmt_ScopedCPUSample(GPU_CreateGraphicsPipelineState, RMTSF_None);
		Handle handle = impl_->AllocHandle(ResourceType::GRAPHICS_PIPELINE_STATE);
		SET_DEBUG_INFO();
		impl_->HandleErrorCode(handle, impl_->backend_->CreateGraphicsPipelineState(handle, desc, fullDebugName));
		return handle;
	}

	Handle Manager::CreateComputePipelineState(const ComputePipelineStateDesc& desc, const char* debugFmt, ...)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(impl_->backend_);
		rmt_ScopedCPUSample(GPU_CreateComputePipelineState, RMTSF_None);
		Handle handle = impl_->AllocHandle(ResourceType::COMPUTE_PIPELINE_STATE);
		SET_DEBUG_INFO();
		impl_->HandleErrorCode(handle, impl_->backend_->CreateComputePipelineState(handle, desc, fullDebugName));
		return handle;
	}

	Handle Manager::CreatePipelineBindingSet(const PipelineBindingSetDesc& desc, const char* debugFmt, ...)
	{
		DBG_ASSERT(IsInitialized());
		rmt_ScopedCPUSample(GPU_CreatePipelineBindingSet, RMTSF_None);
		Handle handle = impl_->AllocHandle(ResourceType::PIPELINE_BINDING_SET);
		SET_DEBUG_INFO();
		impl_->HandleErrorCode(handle, impl_->backend_->CreatePipelineBindingSet(handle, desc, fullDebugName));
		return handle;
	}

	Handle Manager::CreateDrawBindingSet(const DrawBindingSetDesc& desc, const char* debugFmt, ...)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(impl_->backend_);
		rmt_ScopedCPUSample(GPU_CreateDrawBindingSet, RMTSF_None);
		Handle handle = impl_->AllocHandle(ResourceType::DRAW_BINDING_SET);
		SET_DEBUG_INFO();
		impl_->HandleErrorCode(handle, impl_->backend_->CreateDrawBindingSet(handle, desc, fullDebugName));
		return handle;
	}

	Handle Manager::CreateFrameBindingSet(const FrameBindingSetDesc& desc, const char* debugFmt, ...)
	{
		DBG_ASSERT(IsInitialized());
		rmt_ScopedCPUSample(GPU_CreateFrameBindingSet, RMTSF_None);
		Handle handle = impl_->AllocHandle(ResourceType::FRAME_BINDING_SET);
		SET_DEBUG_INFO();
		impl_->HandleErrorCode(handle, impl_->backend_->CreateFrameBindingSet(handle, desc, fullDebugName));
		return handle;
	}

	Handle Manager::CreateCommandList(const char* debugFmt, ...)
	{
		DBG_ASSERT(IsInitialized());
		rmt_ScopedCPUSample(GPU_CreateCommandList, RMTSF_None);
		Handle handle = impl_->AllocHandle(ResourceType::COMMAND_LIST);
		SET_DEBUG_INFO();
		impl_->HandleErrorCode(handle, impl_->backend_->CreateCommandList(handle, fullDebugName));
		return handle;
	}

	Handle Manager::CreateFence(const char* debugFmt, ...)
	{
		DBG_ASSERT(IsInitialized());
		rmt_ScopedCPUSample(GPU_CreateFence, RMTSF_None);
		Handle handle = impl_->AllocHandle(ResourceType::FENCE);
		SET_DEBUG_INFO();
		impl_->HandleErrorCode(handle, impl_->backend_->CreateFence(handle, fullDebugName));
		return handle;
	}

	void Manager::DestroyResource(Handle handle)
	{
		DBG_ASSERT(IsInitialized());
		rmt_ScopedCPUSample(GPU_DestroyResource, RMTSF_None);
		if(handle)
		{
			DBG_ASSERT_MSG(impl_->handles_.IsValid(handle), "Attempting to destroy invalid handle.");
			Core::ScopedMutex lock(impl_->mutex_);
			auto& deletions = impl_->deferredDeletions_[impl_->frameIdx_ % impl_->deferredDeletions_.size()];
			deletions.push_back(handle);
		}
	}

	Handle Manager::AllocTemporaryPipelineBindingSet(const PipelineBindingSetDesc& desc)
	{
		DBG_ASSERT(IsInitialized());
		rmt_ScopedCPUSample(GPU_AllocTemporaryPipelineBindingSet, RMTSF_None);
		Handle handle = impl_->AllocHandle(ResourceType::PIPELINE_BINDING_SET);
#if ENABLE_TEMP_DEBUG_INFO && !defined(_RELEASE)
		Core::Array<char, MAX_DEBUG_NAME_LENGTH> debugName;
		sprintf_s(debugName.data(), debugName.size(), "Temp/PipelineBindingSet(%i, %i, %i, %i)", desc.numCBVs_,
		    desc.numSRVs_, desc.numUAVs_, desc.numSamplers_);
		SetDebugInfo(handle, debugName.data());
#endif
		impl_->HandleErrorCode(handle, impl_->backend_->AllocTemporaryPipelineBindingSet(handle, desc));
		DestroyResource(handle);
		return handle;
	}

	bool Manager::UpdatePipelineBindings(Handle handle, i32 base, Core::ArrayView<const BindingCBV> descs)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(handle.GetType() == ResourceType::PIPELINE_BINDING_SET);
		rmt_ScopedCPUSample(GPU_UpdatePipelineBindings, RMTSF_None);
		return impl_->HandleErrorCode(impl_->backend_->UpdatePipelineBindings(handle, base, descs));
	}


	bool Manager::UpdatePipelineBindings(Handle handle, i32 base, Core::ArrayView<const BindingSRV> descs)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(handle.GetType() == ResourceType::PIPELINE_BINDING_SET);
		rmt_ScopedCPUSample(GPU_UpdatePipelineBindings, RMTSF_None);
		return impl_->HandleErrorCode(impl_->backend_->UpdatePipelineBindings(handle, base, descs));
	}

	bool Manager::UpdatePipelineBindings(Handle handle, i32 base, Core::ArrayView<const BindingUAV> descs)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(handle.GetType() == ResourceType::PIPELINE_BINDING_SET);
		rmt_ScopedCPUSample(GPU_UpdatePipelineBindings, RMTSF_None);
		return impl_->HandleErrorCode(impl_->backend_->UpdatePipelineBindings(handle, base, descs));
	}

	bool Manager::UpdatePipelineBindings(Handle handle, i32 base, Core::ArrayView<const SamplerState> descs)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(handle.GetType() == ResourceType::PIPELINE_BINDING_SET);
		rmt_ScopedCPUSample(GPU_UpdatePipelineBindings, RMTSF_None);
		return impl_->HandleErrorCode(impl_->backend_->UpdatePipelineBindings(handle, base, descs));
	}

	bool Manager::CopyPipelineBindings(
	    Core::ArrayView<const PipelineBinding> dst, Core::ArrayView<const PipelineBinding> src)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(dst.size() == src.size());
#if !defined(_RELEASE)
		for(i32 i = 0; i < dst.size(); ++i)
		{
			DBG_ASSERT(dst[i].pbs_.IsValid());
			DBG_ASSERT(src[i].pbs_.IsValid());
			DBG_ASSERT(dst[i].pbs_.GetType() == ResourceType::PIPELINE_BINDING_SET);
			DBG_ASSERT(src[i].pbs_.GetType() == ResourceType::PIPELINE_BINDING_SET);
			DBG_ASSERT(dst[i].cbvs_.num_ == src[i].cbvs_.num_);
			DBG_ASSERT(dst[i].srvs_.num_ == src[i].srvs_.num_);
			DBG_ASSERT(dst[i].uavs_.num_ == src[i].uavs_.num_);
			DBG_ASSERT(dst[i].samplers_.num_ == src[i].samplers_.num_);
		}
#endif
		rmt_ScopedCPUSample(GPU_CopyPipelineBindings, RMTSF_None);
		return impl_->HandleErrorCode(impl_->backend_->CopyPipelineBindings(dst, src));
	}

	bool Manager::ValidatePipelineBindings(Core::ArrayView<const PipelineBinding> pb)
	{
		DBG_ASSERT(IsInitialized());
		rmt_ScopedCPUSample(GPU_ValidatePipelineBindings, RMTSF_None);
		return impl_->HandleErrorCode(impl_->backend_->ValidatePipelineBindings(pb));
	}


	bool Manager::CompileCommandList(Handle handle, const CommandList& commandList)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(handle.GetType() == ResourceType::COMMAND_LIST);
		rmt_ScopedCPUSample(GPU_CompileCommandList, RMTSF_None);
		return impl_->HandleErrorCode(impl_->backend_->CompileCommandList(handle, commandList));
	}

	bool Manager::SubmitCommandList(Handle handle)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(handle.GetType() == ResourceType::COMMAND_LIST);
		rmt_ScopedCPUSample(GPU_SubmitCommandList, RMTSF_None);
		return impl_->HandleErrorCode(impl_->backend_->SubmitCommandLists(Core::ArrayView<Handle>(&handle, 1)));
	}

	bool Manager::SubmitCommandLists(Core::ArrayView<Handle> handles)
	{
		DBG_ASSERT(IsInitialized());
#if !defined(_RELEASE)
		for(auto handle : handles)
			DBG_ASSERT(handle.GetType() == ResourceType::COMMAND_LIST);
#endif
		rmt_ScopedCPUSample(GPU_SubmitCommandLists, RMTSF_None);
		return impl_->HandleErrorCode(impl_->backend_->SubmitCommandLists(handles));
	}

	bool Manager::PresentSwapChain(Handle handle)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(handle.GetType() == ResourceType::SWAP_CHAIN);
		rmt_ScopedCPUSample(GPU_PresentSwapChain, RMTSF_None);
		return impl_->HandleErrorCode(impl_->backend_->PresentSwapChain(handle));
	}

	bool Manager::ResizeSwapChain(Handle handle, i32 width, i32 height)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(handle.GetType() == ResourceType::SWAP_CHAIN);
		rmt_ScopedCPUSample(GPU_ResizeSwapChain, RMTSF_None);
		return impl_->HandleErrorCode(impl_->backend_->ResizeSwapChain(handle, width, height));
	}

	void Manager::NextFrame()
	{
		DBG_ASSERT(IsInitialized());
		rmt_ScopedCPUSample(GPU_NextFrame, RMTSF_None);
		impl_->frameIdx_++;
		impl_->backend_->NextFrame();
		impl_->ProcessDeletions();
	}

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
		if(Core::ContainsAllFlags(impl_->debugFlags_, DebugFlags::RENDERDOC))
		{
			RenderDoc::BeginCapture(name);
		}
	}

	void Manager::EndDebugCapture()
	{
		DBG_ASSERT(IsInitialized());
		if(Core::ContainsAllFlags(impl_->debugFlags_, DebugFlags::RENDERDOC))
		{
			RenderDoc::EndCapture();
		}
	}

	void Manager::OpenDebugCapture(bool quitOnOpen)
	{
		DBG_ASSERT(IsInitialized());
		if(Core::ContainsAllFlags(impl_->debugFlags_, DebugFlags::RENDERDOC))
		{
			RenderDoc::Open(quitOnOpen);
		}
	}

	void Manager::TriggerDebugCapture()
	{
		DBG_ASSERT(IsInitialized());
		if(Core::ContainsAllFlags(impl_->debugFlags_, DebugFlags::RENDERDOC))
		{
			RenderDoc::Trigger();
		}
	}

} // namespace GPU
