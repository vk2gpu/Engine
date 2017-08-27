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

#include "job/function_job.h"
#include "job/manager.h"

#include "Remotery.h"

#include <algorithm>

namespace Graphics
{
	// Memory for to be allocated from the render graph at runtime.
	static constexpr i32 MAX_FRAME_DATA = 64 * 1024;

	struct RenderPassEntry
	{
		i32 idx_ = 0;
		Core::String name_;
		RenderPass* renderPass_ = nullptr;
	};

	struct ResourceDesc
	{
		i32 id_ = 0;
		Core::String name_;
		GPU::ResourceType resType_ = GPU::ResourceType::INVALID;
		GPU::Handle handle_;
		RenderGraphBufferDesc bufferDesc_;
		RenderGraphTextureDesc textureDesc_;
	};

	struct RenderGraphImpl
	{
		// Built during setup.
		Core::Vector<RenderPassEntry> renderPassEntries_;
		Core::Vector<ResourceDesc> resourceDescs_;
		Core::Set<i32> resourcesNeeded_;
		Core::Vector<GPU::Handle> transientResources_;

		// Built during execute.
		Core::Vector<RenderPassEntry*> executeRenderPasses_;

		// Frame data for allocation.
		Core::LinearAllocator frameAllocator_;

		// Command lists.
		Core::Vector<GPU::CommandList> cmdLists_;
		Core::Vector<GPU::Handle> cmdHandles_;

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
				if(exists.find(entry->idx_) != exists.end())
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
				if(!resDesc.handle_)
				{
					if(resDesc.resType_ == GPU::ResourceType::BUFFER)
					{
						resDesc.handle_ =
						    GPU::Manager::CreateBuffer(resDesc.bufferDesc_, nullptr, resDesc.name_.c_str());
					}
					else if(resDesc.resType_ == GPU::ResourceType::TEXTURE)
					{
						resDesc.handle_ =
						    GPU::Manager::CreateTexture(resDesc.textureDesc_, nullptr, resDesc.name_.c_str());
					}

					transientResources_.push_back(resDesc.handle_);
				}
			}
		}
	};


	RenderGraphBuilder::RenderGraphBuilder(RenderGraphImpl* impl)
	    : impl_(impl)
	{
	}

	RenderGraphBuilder::~RenderGraphBuilder() {}

	RenderGraphResource RenderGraphBuilder::CreateBuffer(const char* name, const RenderGraphBufferDesc& desc)
	{
		ResourceDesc resDesc;
		resDesc.id_ = impl_->resourceDescs_.size();
		resDesc.name_ = name;
		resDesc.resType_ = GPU::ResourceType::BUFFER;
		resDesc.bufferDesc_ = desc;
		impl_->resourceDescs_.push_back(resDesc);
		return RenderGraphResource(resDesc.id_, 0);
	}

	RenderGraphResource RenderGraphBuilder::CreateTexture(const char* name, const RenderGraphTextureDesc& desc)
	{
		ResourceDesc resDesc;
		resDesc.id_ = impl_->resourceDescs_.size();
		resDesc.name_ = name;
		resDesc.resType_ = GPU::ResourceType::TEXTURE;
		resDesc.textureDesc_ = desc;
		impl_->resourceDescs_.push_back(resDesc);
		return RenderGraphResource(resDesc.id_, 0);
	}

	RenderGraphResource RenderGraphBuilder::UseCBV(RenderPass* renderPass, RenderGraphResource res, bool update)
	{
		DBG_ASSERT(res);
		if(!res)
			return res;

		// Patch up required bind flags.
		auto& resource = impl_->resourceDescs_[res.idx_];
		switch(resource.resType_)
		{
		case GPU::ResourceType::BUFFER:
			resource.bufferDesc_.bindFlags_ |= GPU::BindFlags::CONSTANT_BUFFER;
			break;

		default:
			DBG_BREAK;
			break;
		}

		renderPass->impl_->AddInput(res);
		if(update)
		{
			res.version_++;
			renderPass->impl_->AddOutput(res);
		}
		return res;
	}

	RenderGraphResource RenderGraphBuilder::UseSRV(RenderPass* renderPass, RenderGraphResource res)
	{
		DBG_ASSERT(res);
		if(!res)
			return res;

		// Patch up required bind flags.
		auto& resource = impl_->resourceDescs_[res.idx_];
		switch(resource.resType_)
		{
		case GPU::ResourceType::BUFFER:
			resource.bufferDesc_.bindFlags_ |= GPU::BindFlags::SHADER_RESOURCE;
			break;
		case GPU::ResourceType::TEXTURE:
		case GPU::ResourceType::SWAP_CHAIN:
			resource.textureDesc_.bindFlags_ |= GPU::BindFlags::SHADER_RESOURCE;
			break;

		default:
			DBG_BREAK;
			break;
		}

		renderPass->impl_->AddInput(res);

		return res;
	}

	RenderGraphResource RenderGraphBuilder::UseRTV(RenderPass* renderPass, RenderGraphResource res)
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
			DBG_BREAK;
			break;
		}

		renderPass->impl_->AddInput(res);
		res.version_++;
		renderPass->impl_->AddOutput(res);
		return res;
	}

	RenderGraphResource RenderGraphBuilder::UseDSV(RenderPass* renderPass, RenderGraphResource res, GPU::DSVFlags flags)
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
			resource.textureDesc_.bindFlags_ |= GPU::BindFlags::DEPTH_STENCIL;
			break;

		default:
			DBG_BREAK;
			break;
		}

		renderPass->impl_->AddInput(res);

		// If not read only, then it's also an output.
		if(!Core::ContainsAllFlags(flags, GPU::DSVFlags::READ_ONLY_DEPTH | GPU::DSVFlags::READ_ONLY_STENCIL))
		{
			res.version_++;
			renderPass->impl_->AddOutput(res);
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

	RenderGraphResources::RenderGraphResources(RenderGraphImpl* impl)
	    : impl_(impl)
	{
	}

	RenderGraphResources::~RenderGraphResources() {}

	GPU::Handle RenderGraphResources::GetBuffer(RenderGraphResource res, RenderGraphBufferDesc* outDesc) const
	{
		const auto& resDesc = impl_->resourceDescs_[res.idx_];
		DBG_ASSERT(resDesc.resType_ == GPU::ResourceType::BUFFER);
		if(outDesc)
			*outDesc = resDesc.bufferDesc_;
		return resDesc.handle_;
	}

	GPU::Handle RenderGraphResources::GetTexture(RenderGraphResource res, RenderGraphTextureDesc* outDesc) const
	{
		const auto& resDesc = impl_->resourceDescs_[res.idx_];
		DBG_ASSERT(resDesc.resType_ == GPU::ResourceType::TEXTURE || resDesc.resType_ == GPU::ResourceType::SWAP_CHAIN);
		if(outDesc)
			*outDesc = resDesc.textureDesc_;
		return resDesc.handle_;
	}

	RenderGraph::RenderGraph() { impl_ = new RenderGraphImpl(MAX_FRAME_DATA); }

	RenderGraph::~RenderGraph()
	{
		Clear();
		for(auto cmdHandle : impl_->cmdHandles_)
			GPU::Manager::DestroyResource(cmdHandle);
		delete impl_;
	}

	RenderGraphResource RenderGraph::ImportResource(
	    const char* name, GPU::Handle handle, const RenderGraphBufferDesc& desc)
	{
		ResourceDesc resDesc;
		resDesc.id_ = impl_->resourceDescs_.size();
		resDesc.name_ = name;
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
		resDesc.name_ = name;
		resDesc.resType_ = handle.GetType();
		resDesc.handle_ = handle;
		resDesc.textureDesc_ = desc;
		impl_->resourceDescs_.push_back(resDesc);
		return RenderGraphResource(resDesc.id_, 0);
	}

	void RenderGraph::Clear()
	{
		rmt_ScopedCPUSample(RenderGraph_Clear, RMTSF_None);

		for(auto& renderPassEntry : impl_->renderPassEntries_)
		{
			renderPassEntry.renderPass_->~RenderPass();
		}

		for(auto handle : impl_->transientResources_)
		{
			GPU::Manager::DestroyResource(handle);
		}

		impl_->resourcesNeeded_.clear();
		impl_->transientResources_.clear();
		impl_->renderPassEntries_.clear();
		impl_->resourceDescs_.clear();
		impl_->frameAllocator_.Reset();
	}

	void RenderGraph::Execute(RenderGraphResource finalRes)
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
		// Execute render passes sequentially.
		for(i32 idx = 0; idx < numPasses; ++idx)
		{
			RenderGraphResources resources(impl_);
			auto& entry = impl_->executeRenderPasses_[idx];
			auto& cmdList = impl_->cmdLists_[idx];
			auto& cmdHandle = impl_->cmdHandles_[idx];

			cmdList.Reset();
			entry->renderPass_->Execute(resources, cmdList);
			if(cmdList.NumCommands() > 0)
				GPU::Manager::CompileCommandList(cmdHandle, cmdList);
		}

#else
		// Setup job to execute & compile all command lists.
		volatile i32 completed = 0;
		Job::FunctionJob executeJob("RenderGraph::Execute", [impl = this->impl_, &completed](i32 idx) {
			RenderGraphResources resources(impl);

			auto& entry = impl->executeRenderPasses_[idx];
			auto& cmdList = impl->cmdLists_[idx];
			auto& cmdHandle = impl->cmdHandles_[idx];

			cmdList.Reset();
			entry->renderPass_->Execute(resources, cmdList);
			if(cmdList.NumCommands() > 0)
			{
				GPU::Manager::CompileCommandList(cmdHandle, cmdList);
			}

			Core::AtomicAdd(&completed, -1);
		});

		completed = numPasses;
		// Wait for all render pass execution to complete.
		Job::Counter* counter = nullptr;
		executeJob.RunMultiple(0, numPasses - 1, &counter);
		Job::Manager::WaitForCounter(counter, 0);

		DBG_ASSERT(completed == 0);
#endif

		//Core::Log("Execute done\n");
		// Submit all command lists with commands in sequential order.
		rmt_ScopedCPUSample(RenderGraph_SubmitCommandLists, RMTSF_None);
		for(i32 idx = 0; idx < numPasses; ++idx)
		{
			const auto& cmdList = impl_->cmdLists_[idx];
			auto cmdHandle = impl_->cmdHandles_[idx];
			if(cmdList.NumCommands() > 0)
			{
				GPU::Manager::SubmitCommandList(cmdHandle);
			}
		}
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
				renderPassNames[idx] = entry->name_.c_str();
			++idx;
		}
	}

	void RenderGraph::GetResourceName(RenderGraphResource res, const char** name) const
	{
		if(name)
			*name = impl_->resourceDescs_[res.idx_].name_.c_str();
	}

	void RenderGraph::InternalAddRenderPass(const char* name, RenderPass* renderPass)
	{
		RenderPassEntry entry;
		entry.idx_ = impl_->renderPassEntries_.size();
		entry.name_ = name;
		entry.renderPass_ = renderPass;
		impl_->renderPassEntries_.push_back(entry);
	}

	bool RenderGraph::GetBuffer(RenderGraphResource res, RenderGraphBufferDesc* outDesc) const
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

	bool RenderGraph::GetTexture(RenderGraphResource res, RenderGraphTextureDesc* outDesc) const
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

} // namespace Graphics
