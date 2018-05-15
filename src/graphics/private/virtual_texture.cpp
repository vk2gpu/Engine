#include "graphics/virtual_texture.h"
#include "graphics/private/virtual_texture_impl.h"
#include "gpu/manager.h"
#include "gpu/utils.h"

#include "core/misc.h"

#include <algorithm>

namespace Graphics
{
	VTAllocator::VTAllocator(i32 vtDim, i32 pageDim, i32 maxResident)
	    : vtDim_(vtDim)
	    , pageDim_(pageDim)
	    , maxResident_(maxResident)
	{
		DBG_ASSERT(Core::Pot(vtDim));
		DBG_ASSERT(Core::Pot(pageDim));

		i32 numPagesSqrt = (vtDim_ / pageDim_);
		maxPages_ = numPagesSqrt * numPagesSqrt;

		i32 maxNodeLevels = 32 - Core::CountLeadingZeros((u32)numPagesSqrt);

		i32 numTotalNodes = 0;
		for(i32 level = 0; level < maxNodeLevels; ++level)
			numTotalNodes += maxPages_ >> (level * 2);
		nodes_.resize(numTotalNodes);

		// Setup links to children.
		i32 nodeIdx = 0;
		i32 nextLevelBaseIdx = 0;
		for(i32 level = 0; level < maxNodeLevels; ++level)
		{
			i32 numLevelNodes = (1 << (level * 2));

			nextLevelBaseIdx += numLevelNodes;
			for(i32 idx = 0; idx < numLevelNodes; ++idx)
			{
				auto& node = nodes_[nodeIdx];
				node.id_ = idx;

				i32 childNodesIdx = nextLevelBaseIdx + (idx * 4);
				if(childNodesIdx < nodes_.size())
				{
					node.children_ = &nodes_[childNodesIdx];
				}
				++nodeIdx;
			}
		}

		// Setup rect for nodes.
		auto& rootNode = nodes_[0];
		rootNode.rect_.x = 0;
		rootNode.rect_.y = 0;
		rootNode.rect_.w = vtDim;
		rootNode.rect_.h = vtDim;

		RecurseNodes(rootNode, [](VTNode& node, VTNode& parent, VTRelationship rel) {
			const i32 hw = parent.rect_.w / 2;
			const i32 hh = parent.rect_.h / 2;

			node.rect_.x = parent.rect_.x + ((i32)rel % 2 == 0 ? 0 : hw);
			node.rect_.y = parent.rect_.y + ((i32)rel / 2 == 0 ? 0 : hh);
			node.rect_.w = hw;
			node.rect_.h = hh;
		});
	}

	VTAllocator::~VTAllocator() {}

	void VTAllocator::RecurseNodes(VTNode& node, VisitFunc visitFunc)
	{
		if(node.children_)
		{
			visitFunc(node.children_[(i32)VTRelationship::TL], node, VTRelationship::TL);
			RecurseNodes(node.children_[(i32)VTRelationship::TL], visitFunc);

			visitFunc(node.children_[(i32)VTRelationship::TR], node, VTRelationship::TR);
			RecurseNodes(node.children_[(i32)VTRelationship::TR], visitFunc);

			visitFunc(node.children_[(i32)VTRelationship::BL], node, VTRelationship::BL);
			RecurseNodes(node.children_[(i32)VTRelationship::BL], visitFunc);

			visitFunc(node.children_[(i32)VTRelationship::BR], node, VTRelationship::BR);
			RecurseNodes(node.children_[(i32)VTRelationship::BR], visitFunc);
		}
	}

	VTNode* VTAllocator::FindFreeNode(i32 w, i32 h, VTRelationship startCorner)
	{
		return FindFreeNode(nodes_[0], w, h, startCorner);
	}

	VTNode* VTAllocator::FindFreeNode(VTNode& node, i32 w, i32 h, VTRelationship startCorner)
	{
		// Do we fit?
		if(w <= node.rect_.w && h <= node.rect_.h)
		{
			// Try children.
			if(node.children_)
			{
				for(i32 idx = 0; idx < 4; ++idx)
				{
					i32 childIdx = (idx + (i32)startCorner) % 4;
					VTNode* child = FindFreeNode(node.children_[childIdx], w, h, startCorner);
					if(child)
						return child;
				}
			}

			if(node.usedCount_ == 0)
				return &node;
		}

		return nullptr;
	}

	void VTAllocator::MarkNodes(VTNode& node, VTRect rect, bool markUsed)
	{
		if(IsOverlap(node.rect_, rect))
		{
			node.usedCount_ += markUsed ? 1 : -1;
			node.isDirty_ |= markUsed;
			if(node.children_)
			{
				for(i32 idx = 0; idx < 4; ++idx)
				{
					MarkNodes(node.children_[idx], rect, markUsed);
				}
			}
		}
	}

	VTNodeAllocation VTAllocator::AllocPages(i32 w, i32 h)
	{
		w = Core::PotRoundUp(w, pageDim_);
		h = Core::PotRoundUp(h, pageDim_);

		VTNodeAllocation alloc = {};
		alloc.node_ = FindFreeNode(w, h);
		if(alloc.node_)
		{
			MarkNodes(nodes_[0], alloc.node_->rect_, true);
		}

		return alloc;
	}

	void VTAllocator::FreePages(VTNodeAllocation alloc) { MarkNodes(nodes_[0], alloc.node_->rect_, false); }

	i32 VTAllocator::GetTotalAllocs() const { return nodes_[0].usedCount_; }

	VTIndirection::VTIndirection(i32 vtDim, i32 pageDim)
		: vtDim_(vtDim)
		, pageDim_(pageDim)
	{
		DBG_ASSERT(Core::Pot(vtDim));
		DBG_ASSERT(Core::Pot(pageDim));

		const i32 dim = vtDim / pageDim;
		texDesc_.type_ = GPU::TextureType::TEX2D;
		texDesc_.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE | GPU::BindFlags::UNORDERED_ACCESS;
		texDesc_.width_ = dim;
		texDesc_.height_ = dim;
		texDesc_.levels_ = i16(32 - Core::CountLeadingZeros(Core::Min((u32)dim, (u32)pageDim)));
		texDesc_.format_ = GPU::Format::R8G8B8A8_UINT;

		i32 numEntries = 0;
		i32 levelSize = texDesc_.width_ * texDesc_.height_;
		for(i32 i = 0; i < texDesc_.levels_; ++i)
		{
			numEntries += levelSize;
			levelSize /= 4;
		}
		data_.resize(numEntries, { 0xff, 0xff, 0xff, 0xff });

		Core::Vector<GPU::ConstTextureSubResourceData> subRscDatas;
		GPU::ConstTextureSubResourceData subRscData;
		numEntries = 0;
		levelSize = texDesc_.width_ * texDesc_.height_;
		for(i32 i = 0; i < texDesc_.levels_; ++i)
		{
			const i32 w = Core::Max(1, texDesc_.width_ >> i);
			const i32 h = Core::Max(1, texDesc_.height_ >> i);
			levels_.push_back(data_.data() + numEntries);
			numEntries += levelSize;
			levelSize /= 4;

			subRscData.data_ = levels_[i];
			subRscData.rowPitch_ = w * sizeof(Indirection);
			subRscData.slicePitch_ = w * h * sizeof(Indirection);
			subRscDatas.push_back(subRscData);
		}

		tex_ = GPU::Manager::CreateTexture(texDesc_, subRscDatas.data(), "VTIndirection");
	}

	VTIndirection::~VTIndirection()
	{ //
		GPU::Manager::DestroyResource(tex_);
	}

	void VTIndirection::SetIndirection(i32 level, i32 pageX, i32 pageY, u8 cacheX, u8 cacheY)
	{
		const i32 w = Core::Max(1, texDesc_.width_ >> level);
		auto& data = levels_[level][pageY * w + pageX];
		data.x = cacheX;
		data.y = cacheY;
		data.l = (u8)level;
	}

	void VTIndirection::FlushIndirection(GPU::CommandList& cmdList)
	{
		GPU::ConstTextureSubResourceData subRscData;

		for(i32 level = 0; level < texDesc_.levels_; ++level)
		{
			const i32 w = Core::Max(1, texDesc_.width_ >> level);
			const i32 h = Core::Max(1, texDesc_.height_ >> level);

			auto footprint = GPU::GetTextureFootprint(texDesc_.format_, w, h);
			cmdList.UpdateTextureSubResource(tex_, level, GPU::Point(), GPU::Box(), levels_[level], footprint);
		}
	}

	VTManager::VTManager(i32 vtDim, i32 pageDim, i32 maxResident, Core::ArrayView<const GPU::Format> formats, IVTPageProvider* provider)
		: allocator_(vtDim, pageDim, maxResident)
		, indirection_(vtDim, pageDim)
		, formats_(formats)
		, provider_(provider)
	{
		numPagesDim_ = (i32)std::ceil(std::sqrtf((f32)maxResident));
		DBG_ASSERT((numPagesDim_ * numPagesDim_) >= maxResident);

		for(i32 idx = 0; idx < maxResident; ++idx)
		{
			pagesFreeList_.push_back((maxResident - idx) - 1);
		}

		GPU::TextureDesc desc;
		desc.type_ = GPU::TextureType::TEX2D;
		desc.bindFlags_ = GPU::BindFlags::SHADER_RESOURCE;
		desc.width_ = pageDim * numPagesDim_;
		desc.height_ = pageDim * numPagesDim_;
		desc.levels_ = 1;

		textures_.reserve(formats_.size());
		for(const auto format : formats_)
		{
			desc.format_ = format;
			auto texture = GPU::Manager::CreateTexture(desc, nullptr, "VTManager Texture %i", textures_.size());
			textures_.push_back(texture);
			textureDescs_.push_back(desc);
		}
	}

	VTManager::~VTManager()
	{
		for(const auto texture : textures_)
			GPU::Manager::DestroyResource(texture);
	}

	i32 VTManager::CreateTexture(i32 w, i32 h)
	{
		i32 idx = -1;
		auto alloc = allocator_.AllocPages(w, h);
		if(texturesFreeList_.size() == 0)
		{
			allocatedTextures_.push_back(alloc);
			idx = allocatedTextures_.size() - 1;
		}
		else
		{
			idx = texturesFreeList_.back();
			allocatedTextures_[idx] = alloc;
			texturesFreeList_.pop_back();
		}
		return idx;
	}

	void VTManager::DestroyTexture(i32 idx)
	{
		allocator_.FreePages(allocatedTextures_[idx]);
		allocatedTextures_[idx].node_ = nullptr;
		texturesFreeList_.push_back(idx);
	}

	void VTManager::RequestPages()
	{
		struct DirtyNode
		{
			VTNode* node_ = nullptr;
			i32 level_ = -1;
		};
		Core::Vector<DirtyNode> dirtyNodes;

		// Find all dirty nodes.
		const auto visitNode = [&](VTNode& node, VTNode& parent, VTRelationship rel)
		{
			const i32 level = 31 - Core::CountLeadingZeros((u32)(node.rect_.w / allocator_.pageDim_));

			if(level < indirection_.levels_.size() && node.isDirty_)
				dirtyNodes.push_back({&node, level});
		};

		allocator_.RecurseNodes(allocator_.nodes_[0], visitNode);

		std::sort(dirtyNodes.begin(), dirtyNodes.end(), 
			[](const DirtyNode& a, const DirtyNode& b)
			{
				return a.level_ > b.level_;
			});

		for(const auto& dirtyNode : dirtyNodes)
		{
			if(pagesFreeList_.size() > 0)
			{
				auto* node = dirtyNode.node_;
				const i32 level = dirtyNode.level_;

				// Allocate a page.
				const i32 pageIdx = pagesFreeList_.back();
				pagesFreeList_.pop_back();

				bool usedPage = false;

				// Get all textures this node contains.
				for(i32 idx = 0; idx < allocatedTextures_.size(); ++idx)
				{
					const auto& alloc = allocatedTextures_[idx];
					VTRect overlapRect = {};
					if(IsOverlap(node->rect_, alloc.node_->rect_, &overlapRect))
					{
						// Get destination point.
						VTPoint basePoint = GetPageCachePoint(pageIdx);
						VTPoint dstPoint = basePoint;

						// Offset destination.
						auto dstRect = overlapRect;
						dstRect.x >>= level;
						dstRect.y >>= level;
						dstRect.w >>= level;
						dstRect.h >>= level;
						dstPoint.x += dstRect.x % allocator_.pageDim_;
						dstPoint.y += dstRect.y % allocator_.pageDim_;

						DBG_ASSERT(dstPoint.x >= basePoint.x && (dstPoint.x + dstRect.w) <= (basePoint.x + allocator_.pageDim_));
						DBG_ASSERT(dstPoint.y >= basePoint.y && (dstPoint.y + dstRect.h) <= (basePoint.y + allocator_.pageDim_));

						// Calculate rect for level of texture.
						auto srcRect = overlapRect;
						srcRect.x -= alloc.node_->rect_.x;
						srcRect.y -= alloc.node_->rect_.y;
						srcRect.x >>= level;
						srcRect.y >>= level;
						srcRect.w >>= level;
						srcRect.h >>= level;

						if(srcRect.w > 0 && srcRect.h > 0)
						{
							static int jj = 0;
							Core::Log("%u: Base (%u, %u), Dest (%u, %u)\n", jj, basePoint.x, basePoint.y, dstPoint.x, dstPoint.y);

							++jj;
							if(provider_->RequestPage(idx, level, dstPoint, srcRect))
							{
								const i32 pageX = (node->rect_.x / allocator_.pageDim_) >> level;
								const i32 pageY = (node->rect_.y / allocator_.pageDim_) >> level;
								const u8 cacheX = (u8)(GetPageCachePoint(pageIdx).x / allocator_.pageDim_);
								const u8 cacheY = (u8)(GetPageCachePoint(pageIdx).y / allocator_.pageDim_);

								// TODO: Defer until page is marked ready?
								indirection_.SetIndirection(level, pageX, pageY, cacheX, cacheY);

								usedPage = true;
							}
						}
					}
				}

				if(!usedPage)
					pagesFreeList_.push_back(pageIdx);
#if 0
				const i32 pageIdx = pagesFreeList_.back();
				pagesFreeList_.pop_back();

				if(node->children_)
				{
					Core::Log("Dirty mip node: %u x %u @ (%u, %u, %u)\n",
						node->rect_.x >> level, node->rect_.y >> level, node->rect_.w >> level, node->rect_.h >> level, level);
				}
				else
				{
					Core::Log("Dirty leaf node: %u x %u @ (%u, %u, %u)\n",
						node->rect_.x >> level, node->rect_.y >> level, node->rect_.w >> level, node->rect_.h >> level, level);
				}
#endif
			}
		}
	}


	VTPoint VTManager::GetPageCachePoint(i32 idx)
	{
		const i32 x = idx % numPagesDim_;
		const i32 y = idx / numPagesDim_;
		const i32 pageDim = allocator_.pageDim_;
		return { x * pageDim, y * pageDim };
	}

	VirtualTexture::VirtualTexture() {}

	VirtualTexture::~VirtualTexture() {}

} // namespace Graphics