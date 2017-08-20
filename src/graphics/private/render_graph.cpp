#include "graphics/render_graph.h"
#include "graphics/render_pass.h"

#include "core/concurrency.h"
#include "core/misc.h"
#include "core/string.h"
#include "core/vector.h"

namespace Graphics
{
	// Memory for to be allocated from the render graph at runtime.
	static constexpr i32 MAX_FRAME_DATA = 64 * 1024;

	struct RenderGraphImpl
	{
		struct RenderPassEntry
		{
			Core::String name_;
			RenderPass* renderPass_ = nullptr;
		};

		Core::Vector<RenderPassEntry> renderPasses_;
		Core::Vector<i32> renderPassStack_;
		i32 currRenderPass_ = 0;

		struct ResourceDesc
		{
			i32 id_ = 0;
			GPU::ResourceType resType_ = GPU::ResourceType::INVALID;
			GPU::Handle handle_;
			RenderGraphBufferDesc bufferDesc_;
			RenderGraphTextureDesc textureDesc_;
		};

		Core::Vector<ResourceDesc> resourceDescs_;

		struct Link
		{
			Link() = default;
			Link(i32 in, i32 out)
			    : in_(in)
			    , out_(out)
			{
			}

			i32 in_ = -1;
			i32 out_ = -1;
		};

		Core::Vector<Link> inputLinks_;
		Core::Vector<Link> outputLinks_;


		// Frame data for allocation.
		Core::Vector<u8> frameData_;
		volatile i32 frameDataOffset_ = 0;
	};


	RenderGraphBuilder::RenderGraphBuilder(RenderGraphImpl* impl)
	    : impl_(impl)
	{
	}

	RenderGraphBuilder::~RenderGraphBuilder() {}

	RenderGraphResource RenderGraphBuilder::CreateBuffer(const char* name, const RenderGraphBufferDesc& desc)
	{
		RenderGraphImpl::ResourceDesc resDesc;
		resDesc.id_ = impl_->resourceDescs_.size();
		resDesc.resType_ = GPU::ResourceType::BUFFER;
		resDesc.bufferDesc_ = desc;
		impl_->resourceDescs_.push_back(resDesc);
		return RenderGraphResource(resDesc.id_, 0);
	}

	RenderGraphResource RenderGraphBuilder::CreateTexture(const char* name, const RenderGraphTextureDesc& desc)
	{
		RenderGraphImpl::ResourceDesc resDesc;
		resDesc.id_ = impl_->resourceDescs_.size();
		resDesc.resType_ = GPU::ResourceType::TEXTURE;
		resDesc.textureDesc_ = desc;
		impl_->resourceDescs_.push_back(resDesc);
		return RenderGraphResource(resDesc.id_, 0);
	}

	RenderGraphResource RenderGraphBuilder::UseSRV(RenderGraphResource res)
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

		impl_->inputLinks_.emplace_back(res.GetID(), impl_->currRenderPass_);

		return res;
	}

	RenderGraphResource RenderGraphBuilder::UseRTV(RenderGraphResource res)
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

		impl_->outputLinks_.emplace_back(res.GetID(), impl_->currRenderPass_);

		res.version_++;
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
		RenderGraphImpl::ResourceDesc resDesc;
		resDesc.id_ = impl_->resourceDescs_.size();
		resDesc.resType_ = handle.GetType();
		resDesc.handle_ = handle;
		impl_->resourceDescs_.push_back(resDesc);
		return RenderGraphResource(resDesc.id_, 0);
	}

	void RenderGraph::Clear()
	{
		for(auto& renderPassEntry : impl_->renderPasses_)
		{
			renderPassEntry.renderPass_->~RenderPass();
		}
		impl_->renderPasses_.clear();
		impl_->resourceDescs_.clear();
		impl_->frameDataOffset_ = 0;
		impl_->frameData_.fill(0);
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

	void RenderGraph::InternalPushPass()
	{
		impl_->currRenderPass_ = impl_->renderPasses_.size();
		impl_->renderPassStack_.push_back(impl_->currRenderPass_);
	}

	void RenderGraph::InternalPopPass()
	{
		impl_->currRenderPass_ = impl_->renderPassStack_.back();
		impl_->renderPassStack_.pop_back();
	}

	void RenderGraph::InternalAddRenderPass(const char* name, RenderPass* renderPass)
	{
		RenderGraphImpl::RenderPassEntry entry;
		entry.name_ = name;
		entry.renderPass_ = renderPass;
		impl_->renderPasses_.push_back(entry);
	}

} // namespace Graphics
