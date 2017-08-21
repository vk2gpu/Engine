#include "graphics/render_graph.h"
#include "graphics/render_pass.h"
#include "graphics/private/render_pass_impl.h"

#include "core/concurrency.h"
#include "core/misc.h"
#include "core/string.h"
#include "core/vector.h"

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

		// Frame data for allocation.
		Core::Vector<u8> frameData_;
		volatile i32 frameDataOffset_ = 0;

		void AddDependencies(Core::Vector<RenderPassEntry*> outRenderPasses, 
			const Core::ArrayView<RenderGraphResource>& resources)
		{
			i32 beginIdx = outRenderPasses.size();
			i32 endIdx = beginIdx;
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
		};

		void FilterRenderPasses(Core::Vector<RenderPassEntry*> outRenderPasses)
		{
			RenderPassEntry* lastEntry = nullptr;
			for(auto it = outRenderPasses.begin(); it != outRenderPasses.end(); )
			{
				RenderPassEntry* entry = *it;
				if(entry == lastEntry)
				{
					it = outRenderPasses.erase(it);
				}
				else
				{
					++it;
				}
				lastEntry = entry;
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
		resDesc.resType_ = GPU::ResourceType::BUFFER;
		resDesc.bufferDesc_ = desc;
		impl_->resourceDescs_.push_back(resDesc);
		return RenderGraphResource(resDesc.id_, 0);
	}

	RenderGraphResource RenderGraphBuilder::CreateTexture(const char* name, const RenderGraphTextureDesc& desc)
	{
		ResourceDesc resDesc;
		resDesc.id_ = impl_->resourceDescs_.size();
		resDesc.resType_ = GPU::ResourceType::TEXTURE;
		resDesc.textureDesc_ = desc;
		impl_->resourceDescs_.push_back(resDesc);
		return RenderGraphResource(resDesc.id_, 0);
	}

	RenderGraphResource RenderGraphBuilder::UseSRV(RenderPass* renderPass, RenderGraphResource res)
	{
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
		// Patch up required bind flags.
		auto& resource = impl_->resourceDescs_[res.idx_];
		switch(resource.resType_)
		{
		case GPU::ResourceType::BUFFER:
			resource.bufferDesc_.bindFlags_ |= GPU::BindFlags::RENDER_TARGET;
			break;
		case GPU::ResourceType::TEXTURE:
		case GPU::ResourceType::SWAP_CHAIN:
			resource.textureDesc_.bindFlags_ |= GPU::BindFlags::RENDER_TARGET;
			break;

		default:
			DBG_BREAK;
			break;
		}

		res.version_++;
		renderPass->impl_->AddOutput(res);
		return res;
	}

	RenderGraph::RenderGraph()
	{
		impl_ = new RenderGraphImpl;
		impl_->frameData_.resize(MAX_FRAME_DATA);
	}

	RenderGraph::~RenderGraph() { delete impl_; }

	RenderGraphResource RenderGraph::ImportResource(GPU::Handle handle)
	{
		ResourceDesc resDesc;
		resDesc.id_ = impl_->resourceDescs_.size();
		resDesc.resType_ = handle.GetType();
		resDesc.handle_ = handle;
		impl_->resourceDescs_.push_back(resDesc);
		return RenderGraphResource(resDesc.id_, 0);
	}

	void RenderGraph::Clear()
	{
		for(auto& renderPassEntry : impl_->renderPassEntries_)
		{
			renderPassEntry.renderPass_->~RenderPass();
		}
		impl_->renderPassEntries_.clear();
		impl_->resourceDescs_.clear();
		impl_->frameDataOffset_ = 0;
		impl_->frameData_.fill(0);
	}

	void RenderGraph::Compile(RenderGraphResource finalRes)
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
		Core::Vector<RenderPassEntry*> renderPassEntries;
		renderPassEntries.reserve(renderPasses.size());

		impl_->AddDependencies(renderPassEntries, outputs);
		impl_->FilterRenderPasses(renderPassEntries);

#if 0
		for(;;)
		{
			auto renderPassIt = std::find_if(renderPasses.begin(), renderPasses.end(),
				[](const RenderPassEntry& entry)
				{
					return entry.idx_ == ....
				});
			++renderPassIt;

			
		}
#endif
	}

	void* RenderGraph::Alloc(i32 size)
	{
		size = Core::PotRoundUp(size, PLATFORM_ALIGNMENT);

		i32 nextOffset = Core::AtomicAddAcq(&impl_->frameDataOffset_, size) - size;
		DBG_ASSERT(nextOffset <= impl_->frameData_.size());
		if(nextOffset > impl_->frameData_.size())
			return nullptr;
		return impl_->frameData_.data() + (nextOffset - size);
	}

	void RenderGraph::InternalAddRenderPass(const char* name, RenderPass* renderPass)
	{
		RenderPassEntry entry;
		entry.idx_ = impl_->renderPassEntries_.size();
		entry.name_ = name;
		entry.renderPass_ = renderPass;
		impl_->renderPassEntries_.push_back(entry);
	}

} // namespace Graphics
