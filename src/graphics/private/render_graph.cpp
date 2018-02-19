#include "graphics/render_graph.h"
#include "graphics/render_pass.h"
#include "graphics/private/render_pass_impl.h"

#include "core/concurrency.h"
#include "core/linear_allocator.h"
#include "core/misc.h"
#include "core/set.h"
#include "core/string.h"
#include "core/vector.h"

#include "gpu/command_list.h"
#include "gpu/manager.h"
#include "gpu/utils.h"

#include "job/function_job.h"
#include "job/manager.h"

#include "Remotery.h"

#include <algorithm>

namespace
{
	bool operator==(const Graphics::RenderGraphBufferDesc& a, const Graphics::RenderGraphBufferDesc& b)
	{
		return memcmp(&a, &b, sizeof(a)) == 0;
	}

	bool operator==(const Graphics::RenderGraphTextureDesc& a, const Graphics::RenderGraphTextureDesc& b)
	{
		return memcmp(&a, &b, sizeof(a)) == 0;
	}
}

namespace Graphics
{
	// Memory for to be allocated from the render graph at runtime.
	static constexpr i32 MAX_FRAME_DATA = 1024 * 1024;

	struct RenderPassEntry
	{
		RenderPass* renderPass_ = nullptr;
		i32 idx_ = 0;
		Core::Array<char, 64> name_;
	};

	struct ResourceDesc
	{
		GPU::Handle handle_;
		i32 id_ = 0;
		Core::Array<char, 64> name_;
		GPU::ResourceType resType_ = GPU::ResourceType::INVALID;
		RenderGraphBufferDesc bufferDesc_;
		RenderGraphTextureDesc textureDesc_;
		i32 inUse_ = 0;
	};

	struct RenderGraphImpl
	{
		// Built during setup.
		Core::Vector<RenderPassEntry> renderPassEntries_;
		Core::Vector<ResourceDesc> resourceDescs_;
		Core::Set<i32> resourcesNeeded_;

		Core::Vector<ResourceDesc> transientResources_;

		// Built during execute.
		Core::Vector<RenderPassEntry*> executeRenderPasses_;

		// Frame data for allocation.
		Core::LinearAllocator frameAllocator_;

		// Command lists.
		Core::Vector<GPU::CommandList> cmdLists_;
		Core::Vector<GPU::Handle> cmdHandles_;

		// Error handling.
		volatile i32 compilationFailures_ = 0;

		RenderGraphImpl(i32 frameAllocatorSize)
		    : frameAllocator_(frameAllocatorSize)
		{
		}

		void AddDependencies(Core::Vector<RenderPassEntry*>& outRenderPasses,
		    const Core::ArrayView<const RenderGraphResource>& resources)
		{
			i32 beginIdx = outRenderPasses.size();
			i32 endIdx = beginIdx;

			for(auto& res : resources)
			{
				resourcesNeeded_.insert(res.idx_);
			}

			for(auto& entry : renderPassEntries_)
			{
				for(const auto& outputRes : entry.renderPass_->GetOutputs())
				{
					for(const auto& res : resources)
					{
						if(res.idx_ == outputRes.idx_ && res.version_ == outputRes.version_)
						{
							outRenderPasses.push_back(&entry);
							endIdx++;
						}
					}
				}
			}

			// Add all dependencies for the added render passes.
			for(i32 idx = beginIdx; idx < endIdx; ++idx)
			{
				const auto* renderPass = outRenderPasses[idx]->renderPass_;
				AddDependencies(outRenderPasses, renderPass->GetInputs());
			}
		};

		void FilterRenderPasses(Core::Vector<RenderPassEntry*>& outRenderPasses)
		{
			Core::Set<i32> exists;
			for(auto it = outRenderPasses.begin(); it != outRenderPasses.end();)
			{
				RenderPassEntry* entry = *it;
				if(exists.find(entry->idx_) != nullptr)
				{
					it = outRenderPasses.erase(it);
				}
				else
				{
					++it;
				}
				exists.insert(entry->idx_);
			}
		}

		void CreateResources()
		{
			for(i32 idx : resourcesNeeded_)
			{
				auto& resDesc = resourceDescs_[idx];

				auto foundIt = std::find_if(transientResources_.begin(), transientResources_.end(),
				    [&resDesc](const ResourceDesc& transientDesc) {
					    if(transientDesc.handle_ && transientDesc.inUse_ == 0 &&
					        resDesc.resType_ == transientDesc.resType_)
					    {
						    if(resDesc.resType_ == GPU::ResourceType::BUFFER)
						    {
							    return resDesc.bufferDesc_ == transientDesc.bufferDesc_;
						    }
						    else if(resDesc.resType_ == GPU::ResourceType::TEXTURE)
						    {
							    return resDesc.textureDesc_ == transientDesc.textureDesc_;
							    ;
						    }
					    }
					    return false;
					});

				if(foundIt == transientResources_.end())
				{
					if(!resDesc.handle_)
					{
						if(resDesc.resType_ == GPU::ResourceType::BUFFER)
						{
							resDesc.handle_ =
							    GPU::Manager::CreateBuffer(resDesc.bufferDesc_, nullptr, resDesc.name_.data());
						}
						else if(resDesc.resType_ == GPU::ResourceType::TEXTURE)
						{
							resDesc.handle_ =
							    GPU::Manager::CreateTexture(resDesc.textureDesc_, nullptr, resDesc.name_.data());
						}

						resDesc.inUse_ = 1;
						transientResources_.push_back(resDesc);
					}
				}
				else
				{
					DBG_ASSERT(foundIt->handle_);
					foundIt->inUse_ = 1;
					resDesc = *foundIt;
				}
			}

			for(auto it = transientResources_.begin(); it != transientResources_.end();)
			{
				if(it->inUse_ == 0)
				{
					GPU::Manager::DestroyResource(it->handle_);
					it = transientResources_.erase(it);
				}
				else
				{
					++it;
				}
			}
		}

		void CreateFrameBindingSets()
		{
			for(auto& entry : renderPassEntries_)
			{
				auto* renderPass = entry.renderPass_->impl_;
				if(renderPass->dsv_ || renderPass->rtvs_[0])
				{
					auto dsvRes = renderPass->dsv_;
					if(dsvRes)
						renderPass->fbsDesc_.dsv_.resource_ = GetTexture(dsvRes, nullptr);
					for(i32 idx = 0; idx < renderPass->rtvs_.size(); ++idx)
					{
						auto rtvRes = renderPass->rtvs_[idx];
						if(rtvRes)
							renderPass->fbsDesc_.rtvs_[idx].resource_ = GetTexture(rtvRes, nullptr);
					}

					renderPass->fbs_ = GPU::Manager::CreateFrameBindingSet(renderPass->fbsDesc_, entry.name_.data());
				}
			}
		}

		GPU::Handle GetHandle(RenderGraphResource res) const
		{
			const auto& resDesc = resourceDescs_[res.idx_];
			return resDesc.handle_;
		}

		GPU::Handle GetBuffer(RenderGraphResource res, RenderGraphBufferDesc* outDesc) const
		{
			const auto& resDesc = resourceDescs_[res.idx_];
			DBG_ASSERT(resDesc.resType_ == GPU::ResourceType::BUFFER);
			if(outDesc)
				*outDesc = resDesc.bufferDesc_;
			return resDesc.handle_;
		}

		GPU::Handle GetTexture(RenderGraphResource res, RenderGraphTextureDesc* outDesc) const
		{
			const auto& resDesc = resourceDescs_[res.idx_];
			DBG_ASSERT(
			    resDesc.resType_ == GPU::ResourceType::TEXTURE || resDesc.resType_ == GPU::ResourceType::SWAP_CHAIN);
			if(outDesc)
				*outDesc = resDesc.textureDesc_;
			return resDesc.handle_;
		}
	};


	RenderGraphBuilder::RenderGraphBuilder(RenderGraphImpl* impl, RenderPass* renderPass)
	    : impl_(impl)
	    , renderPass_(renderPass)
	{
	}

	RenderGraphBuilder::~RenderGraphBuilder() {}

	RenderGraphResource RenderGraphBuilder::Create(const char* name, const RenderGraphBufferDesc& desc)
	{
		ResourceDesc resDesc;
		resDesc.id_ = impl_->resourceDescs_.size();
		strcpy_s(resDesc.name_.data(), resDesc.name_.size(), name);
		resDesc.resType_ = GPU::ResourceType::BUFFER;
		resDesc.bufferDesc_ = desc;
		impl_->resourceDescs_.push_back(resDesc);
		return RenderGraphResource(resDesc.id_, 0);
	}

	RenderGraphResource RenderGraphBuilder::Create(const char* name, const RenderGraphTextureDesc& desc)
	{
		ResourceDesc resDesc;
		const char* renderPassName = "TODO_RENDER_PASS_NAME";
		resDesc.id_ = impl_->resourceDescs_.size();
		sprintf_s(resDesc.name_.data(), resDesc.name_.size(), "%s/%s", renderPassName, name);
		resDesc.resType_ = GPU::ResourceType::TEXTURE;
		resDesc.textureDesc_ = desc;
		impl_->resourceDescs_.push_back(resDesc);
		return RenderGraphResource(resDesc.id_, 0);
	}

	RenderGraphResource RenderGraphBuilder::Read(RenderGraphResource res, GPU::BindFlags bindFlags)
	{
		DBG_ASSERT(res);
		if(!res)
			return res;

		// Patch up required bind flags.
		auto& resource = impl_->resourceDescs_[res.idx_];
		switch(resource.resType_)
		{
		case GPU::ResourceType::BUFFER:
			resource.bufferDesc_.bindFlags_ |= bindFlags;
			break;
		case GPU::ResourceType::TEXTURE:
		case GPU::ResourceType::SWAP_CHAIN:
			resource.textureDesc_.bindFlags_ |= bindFlags;
			break;

		default:
			DBG_ASSERT(false);
			break;
		}

		renderPass_->impl_->AddInput(res);

		return res;
	}

	RenderGraphResource RenderGraphBuilder::Write(RenderGraphResource res, GPU::BindFlags bindFlags)
	{
		DBG_ASSERT(res);
		if(!res)
			return res;

		// Patch up required bind flags.
		auto& resource = impl_->resourceDescs_[res.idx_];
		switch(resource.resType_)
		{
		case GPU::ResourceType::BUFFER:
			resource.bufferDesc_.bindFlags_ |= bindFlags;
			break;
		case GPU::ResourceType::TEXTURE:
		case GPU::ResourceType::SWAP_CHAIN:
			resource.textureDesc_.bindFlags_ |= bindFlags;
			break;

		default:
			DBG_ASSERT(false);
			break;
		}

		renderPass_->impl_->AddInput(res);
		res.version_++;
		renderPass_->impl_->AddOutput(res);
		return res;
	}

	RenderGraphResource RenderGraphBuilder::SetRTV(i32 idx, RenderGraphResource res, GPU::BindingRTV binding)
	{
		DBG_ASSERT(res);
		if(!res)
			return res;

		// Patch up required bind flags.
		auto& resource = impl_->resourceDescs_[res.idx_];
		switch(resource.resType_)
		{
		case GPU::ResourceType::TEXTURE:
		case GPU::ResourceType::SWAP_CHAIN:
			resource.textureDesc_.bindFlags_ |= GPU::BindFlags::RENDER_TARGET;
			break;

		default:
			DBG_ASSERT(false);
			break;
		}

		// Invalid dimension specified, setup default.
		if(binding.dimension_ == GPU::ViewDimension::INVALID)
		{
			binding.dimension_ = GPU::GetViewDimension(resource.textureDesc_.type_);
			binding.format_ = resource.textureDesc_.format_;
		}

		// Set framebuffer binding desc on render pass.
		renderPass_->impl_->rtvs_[idx] = res;
		renderPass_->impl_->fbsDesc_.rtvs_[idx] = binding;

		// Add inputs & outputs for dependency tracking.
		renderPass_->impl_->AddInput(res);
		res.version_++;
		renderPass_->impl_->AddOutput(res);
		return res;
	}

	RenderGraphResource RenderGraphBuilder::SetDSV(RenderGraphResource res, GPU::BindingDSV binding)
	{
		DBG_ASSERT(res);
		if(!res)
			return res;

		// Patch up required bind flags.
		auto& resource = impl_->resourceDescs_[res.idx_];
		switch(resource.resType_)
		{
		case GPU::ResourceType::TEXTURE:
			resource.textureDesc_.bindFlags_ |= GPU::BindFlags::DEPTH_STENCIL;
			break;

		default:
			DBG_ASSERT(false);
			break;
		}

		// Invalid dimension specified, setup default.
		if(binding.dimension_ == GPU::ViewDimension::INVALID)
		{
			binding.dimension_ = GPU::GetViewDimension(resource.textureDesc_.type_);
			binding.format_ = GPU::GetDSVFormat(resource.textureDesc_.format_);
		}

		// Set framebuffer binding desc on render pass.
		renderPass_->impl_->dsv_ = res;
		renderPass_->impl_->fbsDesc_.dsv_ = binding;

		// Add inputs & outputs for dependency tracking.
		renderPass_->impl_->AddInput(res);

		// If not read only, then it's also an output.
		if(!Core::ContainsAllFlags(binding.flags_, GPU::DSVFlags::READ_ONLY_DEPTH | GPU::DSVFlags::READ_ONLY_STENCIL))
		{
			res.version_++;
			renderPass_->impl_->AddOutput(res);
		}

		return res;
	}

	bool RenderGraphBuilder::GetBuffer(RenderGraphResource res, RenderGraphBufferDesc* outDesc) const
	{
		if(res.idx_ < impl_->resourceDescs_.size())
		{
			const auto& resDesc = impl_->resourceDescs_[res.idx_];
			if(resDesc.resType_ == GPU::ResourceType::BUFFER)
			{
				if(outDesc)
				{
					*outDesc = resDesc.bufferDesc_;
				}
				return true;
			}
		}
		return false;
	}

	bool RenderGraphBuilder::GetTexture(RenderGraphResource res, RenderGraphTextureDesc* outDesc) const
	{
		if(res.idx_ < impl_->resourceDescs_.size())
		{
			const auto& resDesc = impl_->resourceDescs_[res.idx_];
			if(resDesc.resType_ == GPU::ResourceType::TEXTURE || resDesc.resType_ == GPU::ResourceType::SWAP_CHAIN)
			{
				if(outDesc)
				{
					*outDesc = resDesc.textureDesc_;
				}
				return true;
			}
		}
		return false;
	}

	void* RenderGraphBuilder::Alloc(i32 size) { return impl_->frameAllocator_.Allocate(size); }

	RenderGraphResources::RenderGraphResources(RenderGraphImpl* impl, RenderPassImpl* renderPass)
	    : impl_(impl)
	    , renderPass_(renderPass)
	{
	}

	RenderGraphResources::~RenderGraphResources() {}

	GPU::Handle RenderGraphResources::GetBuffer(RenderGraphResource res, RenderGraphBufferDesc* outDesc) const
	{
		return impl_->GetBuffer(res, outDesc);
	}

	GPU::Handle RenderGraphResources::GetTexture(RenderGraphResource res, RenderGraphTextureDesc* outDesc) const
	{
		return impl_->GetTexture(res, outDesc);
	}

	GPU::Handle RenderGraphResources::GetFrameBindingSet(GPU::FrameBindingSetDesc* outDesc) const
	{
		if(outDesc)
			*outDesc = renderPass_->fbsDesc_;
		return renderPass_->fbs_;
	}

	GPU::BindingCBV RenderGraphResources::CBuffer(RenderGraphResource res, i32 offset, i32 size) const
	{
		return GPU::Binding::CBuffer(impl_->GetHandle(res), offset, size);
	}
	GPU::BindingSRV RenderGraphResources::Buffer(
	    RenderGraphResource res, GPU::Format format, i32 firstElement, i32 numElements, i32 structureByteStride) const
	{
		return GPU::Binding::Buffer(impl_->GetHandle(res), format, firstElement, numElements, structureByteStride);
	}
	GPU::BindingSRV RenderGraphResources::Texture1D(
	    RenderGraphResource res, GPU::Format format, i32 mostDetailedMip, i32 mipLevels, f32 resourceMinLODClamp) const
	{
		return GPU::Binding::Texture1D(impl_->GetHandle(res), format, mostDetailedMip, mipLevels, resourceMinLODClamp);
	}
	GPU::BindingSRV RenderGraphResources::Texture1DArray(RenderGraphResource res, GPU::Format format,
	    i32 mostDetailedMip, i32 mipLevels, i32 firstArraySlice, i32 arraySize, f32 resourceMinLODClamp) const
	{
		return GPU::Binding::Texture1DArray(
		    impl_->GetHandle(res), format, mostDetailedMip, mipLevels, firstArraySlice, arraySize, resourceMinLODClamp);
	}
	GPU::BindingSRV RenderGraphResources::Texture2D(RenderGraphResource res, GPU::Format format, i32 mostDetailedMip,
	    i32 mipLevels, i32 planeSlice, f32 resourceMinLODClamp) const
	{
		return GPU::Binding::Texture2D(
		    impl_->GetHandle(res), format, mostDetailedMip, mipLevels, planeSlice, resourceMinLODClamp);
	}
	GPU::BindingSRV RenderGraphResources::Texture2DArray(RenderGraphResource res, GPU::Format format,
	    i32 mostDetailedMip, i32 mipLevels, i32 firstArraySlice, i32 arraySize, i32 planeSlice,
	    f32 resourceMinLODClamp) const
	{
		return GPU::Binding::Texture2DArray(impl_->GetHandle(res), format, mostDetailedMip, mipLevels, firstArraySlice,
		    arraySize, planeSlice, resourceMinLODClamp);
	}
	GPU::BindingSRV RenderGraphResources::Texture3D(
	    RenderGraphResource res, GPU::Format format, i32 mostDetailedMip, i32 mipLevels, f32 resourceMinLODClamp) const
	{
		return GPU::Binding::Texture3D(impl_->GetHandle(res), format, mostDetailedMip, mipLevels, resourceMinLODClamp);
	}
	GPU::BindingSRV RenderGraphResources::TextureCube(
	    RenderGraphResource res, GPU::Format format, i32 mostDetailedMip, i32 mipLevels, f32 resourceMinLODClamp) const
	{
		return GPU::Binding::TextureCube(
		    impl_->GetHandle(res), format, mostDetailedMip, mipLevels, resourceMinLODClamp);
	}
	GPU::BindingSRV RenderGraphResources::TextureCubeArray(RenderGraphResource res, GPU::Format format,
	    i32 mostDetailedMip, i32 mipLevels, i32 first2DArrayFace, i32 numCubes, f32 resourceMinLODClamp) const
	{
		return GPU::Binding::TextureCubeArray(
		    impl_->GetHandle(res), format, mostDetailedMip, mipLevels, first2DArrayFace, numCubes, resourceMinLODClamp);
	}
	GPU::BindingUAV RenderGraphResources::RWBuffer(
	    RenderGraphResource res, GPU::Format format, i32 firstElement, i32 numElements, i32 structureByteStride) const
	{
		return GPU::Binding::RWBuffer(impl_->GetHandle(res), format, firstElement, numElements, structureByteStride);
	}
	GPU::BindingUAV RenderGraphResources::RWTexture1D(RenderGraphResource res, GPU::Format format, i32 mipSlice) const
	{
		return GPU::Binding::RWTexture1D(impl_->GetHandle(res), format, mipSlice);
	}
	GPU::BindingUAV RenderGraphResources::RWTexture1DArray(
	    RenderGraphResource res, GPU::Format format, i32 mipSlice, i32 firstArraySlice, i32 arraySize) const
	{
		return GPU::Binding::RWTexture1DArray(impl_->GetHandle(res), format, mipSlice, firstArraySlice, arraySize);
	}
	GPU::BindingUAV RenderGraphResources::RWTexture2D(
	    RenderGraphResource res, GPU::Format format, i32 mipSlice, i32 planeSlice) const
	{
		return GPU::Binding::RWTexture2D(impl_->GetHandle(res), format, mipSlice, planeSlice);
	}
	GPU::BindingUAV RenderGraphResources::RWTexture2DArray(RenderGraphResource res, GPU::Format format, i32 mipSlice,
	    i32 planeSlice, i32 firstArraySlice, i32 arraySize) const
	{
		return GPU::Binding::RWTexture2DArray(
		    impl_->GetHandle(res), format, mipSlice, planeSlice, firstArraySlice, arraySize);
	}
	GPU::BindingUAV RenderGraphResources::RWTexture3D(
	    RenderGraphResource res, GPU::Format format, i32 mipSlice, i32 firstWSlice, i32 wSize) const
	{
		return GPU::Binding::RWTexture3D(impl_->GetHandle(res), format, mipSlice, firstWSlice, wSize);
	}


	RenderGraph::RenderGraph() { impl_ = new RenderGraphImpl(MAX_FRAME_DATA); }

	RenderGraph::~RenderGraph()
	{
		Clear();
		for(auto resDesc : impl_->transientResources_)
			GPU::Manager::DestroyResource(resDesc.handle_);

		for(auto cmdHandle : impl_->cmdHandles_)
			GPU::Manager::DestroyResource(cmdHandle);
		delete impl_;
	}

	RenderGraphResource RenderGraph::ImportResource(
	    const char* name, GPU::Handle handle, const RenderGraphBufferDesc& desc)
	{
		ResourceDesc resDesc;
		resDesc.id_ = impl_->resourceDescs_.size();
		strcpy_s(resDesc.name_.data(), resDesc.name_.size(), name);
		resDesc.resType_ = handle.GetType();
		resDesc.handle_ = handle;
		resDesc.bufferDesc_ = desc;
		impl_->resourceDescs_.push_back(resDesc);
		return RenderGraphResource(resDesc.id_, 0);
	}

	RenderGraphResource RenderGraph::ImportResource(
	    const char* name, GPU::Handle handle, const RenderGraphTextureDesc& desc)
	{
		ResourceDesc resDesc;
		resDesc.id_ = impl_->resourceDescs_.size();
		strcpy_s(resDesc.name_.data(), resDesc.name_.size(), name);
		resDesc.resType_ = handle.GetType();
		resDesc.handle_ = handle;
		resDesc.textureDesc_ = desc;
		impl_->resourceDescs_.push_back(resDesc);
		return RenderGraphResource(resDesc.id_, 0);
	}

	void RenderGraph::Clear()
	{
		rmt_ScopedCPUSample(RenderGraph_Clear, RMTSF_None);

		for(auto& resDesc : impl_->transientResources_)
			resDesc.inUse_ = 0;

		for(auto& renderPassEntry : impl_->renderPassEntries_)
			renderPassEntry.renderPass_->~RenderPass();

		impl_->resourcesNeeded_.clear();
		impl_->renderPassEntries_.clear();
		impl_->resourceDescs_.clear();
		impl_->frameAllocator_.Reset();
	}

	bool RenderGraph::Execute(RenderGraphResource finalRes)
	{
		// Find newest version of finalRes.
		finalRes.version_ = -1;
		for(const auto& entry : impl_->renderPassEntries_)
		{
			for(const auto& outputRes : entry.renderPass_->GetOutputs())
			{
				if(finalRes.idx_ == outputRes.idx_ && finalRes.version_ < outputRes.version_)
				{
					finalRes = outputRes;
				}
			}
		}

		if(finalRes.version_ == -1)
		{
			DBG_LOG("ERROR: Unable to find finalRes in graph.");
		}

		// Add finalRes to outputs to start traversal.
		const i32 MAX_OUTPUTS = Core::Max(GPU::MAX_UAV_BINDINGS, GPU::MAX_BOUND_RTVS);
		Core::Vector<RenderGraphResource> outputs;
		outputs.reserve(MAX_OUTPUTS);

		outputs.push_back(finalRes);

		// From finalRes, work backwards and push all render passes that are required onto the stack.
		auto& renderPasses = impl_->renderPassEntries_;
		{
			rmt_ScopedCPUSample(RenderGraph_AddDependencies, RMTSF_None);
			impl_->executeRenderPasses_.clear();
			impl_->executeRenderPasses_.reserve(renderPasses.size());

			impl_->AddDependencies(impl_->executeRenderPasses_, outputs);

			std::reverse(impl_->executeRenderPasses_.begin(), impl_->executeRenderPasses_.end());
		}

		{
			rmt_ScopedCPUSample(RenderGraph_FilterPasses, RMTSF_None);
			impl_->FilterRenderPasses(impl_->executeRenderPasses_);
		}

		{
			rmt_ScopedCPUSample(RenderGraph_CreateResources, RMTSF_None);
			impl_->CreateResources();
		}

		{
			rmt_ScopedCPUSample(RenderGraph_CreateFrameBindingSets, RMTSF_None);
			impl_->CreateFrameBindingSets();
		}


		// Create more command lists as required.
		const i32 numPasses = impl_->executeRenderPasses_.size();
		if(impl_->cmdLists_.size() < numPasses)
		{
			impl_->cmdLists_.reserve(numPasses);
			impl_->cmdHandles_.reserve(numPasses);
			for(i32 idx = impl_->cmdLists_.size(); idx < numPasses; ++idx)
			{
				impl_->cmdLists_.emplace_back(8 * 1024 * 1024);
				impl_->cmdHandles_.emplace_back(GPU::Manager::CreateCommandList("RenderGraph"));
			}
		}

#if 0
		bool useSingleCommandList = true;

		auto& singleCmdList = impl_->cmdLists_[0];
		auto& singleCmdHandle = impl_->cmdHandles_[0];
		if(useSingleCommandList)
			singleCmdList.Reset();

		// Execute render passes sequentially.
		for(i32 idx = 0; idx < numPasses; ++idx)
		{
			auto& entry = impl_->executeRenderPasses_[idx];
			auto& cmdList = impl_->cmdLists_[idx];
			auto& cmdHandle = impl_->cmdHandles_[idx];

			RenderGraphResources resources(impl_, entry->renderPass_->impl_);
			if(useSingleCommandList == false)
			{
				cmdList.Reset();
				if(auto event = cmdList.Event(0, entry->name_.data()))
				{
					entry->renderPass_->Execute(resources, cmdList);
					if(cmdList.NumCommands() > 0)
						GPU::Manager::CompileCommandList(cmdHandle, cmdList);
				}
			}
			else
			{
				if(auto event = singleCmdList.Event(0, entry->name_.data()))
				{
					entry->renderPass_->Execute(resources, singleCmdList);
				}
			}
		}

		if(useSingleCommandList && singleCmdList.NumCommands() > 0)
			GPU::Manager::CompileCommandList(singleCmdHandle, singleCmdList);

#else
		// Setup job to execute & compile all command lists.
		Core::Vector<Job::JobDesc> jobDescs;
		jobDescs.resize(numPasses);

		for(i32 idx = 0; idx < numPasses; ++idx)
		{
			auto& jobDesc = jobDescs[idx];

			jobDesc.func_ = [](i32 idx, void* userData) {
				auto* impl = reinterpret_cast<RenderGraphImpl*>(userData);
				auto& entry = impl->executeRenderPasses_[idx];
				auto& cmdList = impl->cmdLists_[idx];
				auto& cmdHandle = impl->cmdHandles_[idx];

				RenderGraphResources resources(impl, entry->renderPass_->impl_);

				cmdList.Reset();
				if(auto event = cmdList.Event(0x00000000, entry->name_.data()))
					entry->renderPass_->Execute(resources, cmdList);

				if(cmdList.GetType() != GPU::CommandQueueType::NONE)
				{
					if(!GPU::Manager::CompileCommandList(cmdHandle, cmdList))
					{
						Core::AtomicInc(&impl->compilationFailures_);
						DBG_ASSERT_MSG(
						    false, "Failed to compile command list for render pass \"%s\"", entry->name_.data());
					}
				}
			};

			jobDesc.param_ = idx;
			jobDesc.data_ = impl_;
			jobDesc.name_ = impl_->executeRenderPasses_[idx]->name_.data();
		}

		// Wait for all render pass execution to complete.
		Job::Counter* counter = nullptr;
		Job::Manager::RunJobs(jobDescs.data(), jobDescs.size(), &counter);
		Job::Manager::WaitForCounter(counter, 0);
#endif

		if(impl_->compilationFailures_ > 0)
			return false;

		static bool individualSubmission = false;

		//Core::Log("Execute done\n");
		// Submit all command lists with commands in sequential order.
		rmt_ScopedCPUSample(RenderGraph_SubmitCommandLists, RMTSF_None);
		if(individualSubmission)
		{
			for(i32 idx = 0; idx < numPasses; ++idx)
			{
				auto& entry = impl_->executeRenderPasses_[idx];
				auto cmdHandle = impl_->cmdHandles_[idx];
				if(!GPU::Manager::SubmitCommandLists(cmdHandle))
				{
					DBG_ASSERT_MSG(false, "Failed to submit command list for render pass \"%s\".", entry->name_.data());
					return false;
				}
			}
		}
		else
		{
			Core::Vector<GPU::Handle> cmdLists;
			jobDescs.resize(numPasses);
			for(i32 idx = 0; idx < numPasses; ++idx)
			{
				const auto& cmdList = impl_->cmdLists_[idx];
				auto cmdHandle = impl_->cmdHandles_[idx];
				if(cmdList.NumCommands() > 0)
					cmdLists.push_back(cmdHandle);
			}
			if(!GPU::Manager::SubmitCommandLists(cmdLists))
			{
				DBG_ASSERT_MSG(false, "Failed to submit command lists.");
				return false;
			}
		}
		return true;
	}

	i32 RenderGraph::GetNumExecutedRenderPasses() const { return impl_->executeRenderPasses_.size(); }

	void RenderGraph::GetExecutedRenderPasses(const RenderPass** renderPasses, const char** renderPassNames) const
	{
		i32 idx = 0;
		for(const auto* entry : impl_->executeRenderPasses_)
		{
			if(renderPasses)
				renderPasses[idx] = entry->renderPass_;
			if(renderPassNames)
				renderPassNames[idx] = entry->name_.data();
			++idx;
		}
	}

	void RenderGraph::GetResourceName(RenderGraphResource res, const char** name) const
	{
		if(name)
			*name = impl_->resourceDescs_[res.idx_].name_.data();
	}

	void RenderGraph::InternalAddRenderPass(const char* name, RenderPass* renderPass)
	{
		RenderPassEntry entry;
		entry.idx_ = impl_->renderPassEntries_.size();
		strcpy_s(entry.name_.data(), entry.name_.size(), name);
		entry.renderPass_ = renderPass;
		impl_->renderPassEntries_.push_back(entry);
	}

	void* RenderGraph::InternalAlloc(i32 size) { return impl_->frameAllocator_.Allocate(size); }

	bool RenderGraph::GetBuffer(RenderGraphResource res, RenderGraphBufferDesc* outDesc) const
	{
		if(res.idx_ < impl_->resourceDescs_.size())
		{
			const auto& resDesc = impl_->resourceDescs_[res.idx_];
			if(resDesc.resType_ == GPU::ResourceType::BUFFER)
			{
				if(outDesc)
					*outDesc = resDesc.bufferDesc_;
				return true;
			}
		}
		return false;
	}

	bool RenderGraph::GetTexture(RenderGraphResource res, RenderGraphTextureDesc* outDesc) const
	{
		if(res.idx_ < impl_->resourceDescs_.size())
		{
			const auto& resDesc = impl_->resourceDescs_[res.idx_];
			if(resDesc.resType_ == GPU::ResourceType::TEXTURE || resDesc.resType_ == GPU::ResourceType::SWAP_CHAIN)
			{
				if(outDesc)
					*outDesc = resDesc.textureDesc_;
				return true;
			}
		}
		return false;
	}

} // namespace Graphics
