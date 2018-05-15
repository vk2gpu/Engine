#pragma once

#include "graphics/virtual_texture.h"
#include "gpu/fwd_decls.h"
#include "gpu/resources.h"
#include "core/function.h"
#include "core/misc.h"
#include "core/vector.h"
#include "math/vec2.h"

namespace Graphics
{
	struct VTPoint
	{
		i32 x;
		i32 y;
	};

	struct VTRect
	{
		i32 x;
		i32 y;
		i32 w;
		i32 h;
	};

	bool IsOverlap(const VTRect a, const VTRect b, VTRect* o = nullptr)
	{
		const i32 axmin = a.x;
		const i32 axmax = a.x + a.w;
		const i32 aymin = a.y;
		const i32 aymax = a.y + a.h;
		const i32 bxmin = b.x;
		const i32 bxmax = b.x + b.w;
		const i32 bymin = b.y;
		const i32 bymax = b.y + b.h;

		if((axmin >= bxmax || axmax <= bxmin || aymin >= bymax || aymax <= bymin))
		{
			return false;
		}

		if(o)
		{
			const i32 minx = Core::Max(axmin, bxmin);
			const i32 maxx = Core::Min(axmax, bxmax);
			const i32 miny = Core::Max(aymin, bymin);
			const i32 maxy = Core::Min(aymax, bymax);
			o->x = minx;
			o->y = miny;
			o->w = maxx - minx;
			o->h = maxy - miny;
		}

		return true;
	}

	struct VTTexture
	{
		i32 w_ = 0;
		i32 h_ = 0;
		i32 levels_ = 0;
	};

	enum class VTRelationship : u32
	{
		TL = 0,
		TR,
		BL,
		BR,
	};

	struct VTNode
	{
		/// Id within level. Used for debugging.
		i32 id_ = 0;

		/// VTRect this node covers.
		VTRect rect_ = {};

		/// Pointer to our 4 children.
		VTNode* children_ = nullptr;

		/// Node used count?
		i32 usedCount_ = 0;

		/// Is node dirty?
		bool isDirty_ = false;
	};

	struct VTNodeAllocation
	{
		/// Node that the allocation fits inside of.
		VTNode* node_ = nullptr;
	};

	/**
	 * Virtual Texture Allocator.
	 * This is used for allocating areas of a 2D area for use as
	 * individual pages of a fixed size.
	 */
	struct GRAPHICS_DLL VTAllocator
	{
		/**
		 * @param vtDim Dimensions of virtual texture (vtDim x vtDim)
		 * @param pageDim Dimensions of each page (pageDim x pageDim)
		 * @param maxResident Maximum pages that can be resident.
		 */
		VTAllocator(i32 vtDim, i32 pageDim, i32 maxResident);

		~VTAllocator();

		using VisitFunc = Core::Function<void(VTNode& node, VTNode& parent, VTRelationship rel)>;

		/**
		 * Recurse nodes and call @a visitFunc.
		 */
		void RecurseNodes(VTNode& node, VisitFunc visitFunc);

		/**
		 * Find free node large enough to fit dimensions @a w and @a h, and begin 
		 * search from corner @a startCorner of each node.
		 */
		VTNode* FindFreeNode(i32 w, i32 h, VTRelationship startCorner = VTRelationship::TL);
		VTNode* FindFreeNode(VTNode& node, i32 w, i32 h, VTRelationship startCorner);

		/**
		 * Mark/unmark all nodes that overlap with rect.
		 */
		void MarkNodes(VTNode& node, VTRect rect, bool markUsed);

		/**
		 * Allocate pages for texture.
		 */
		VTNodeAllocation AllocPages(i32 w, i32 h);

		/**
		 * Free pages.
		 */
		void FreePages(VTNodeAllocation alloc);

		/**
		 * Get total number of allocations.
		 */
		i32 GetTotalAllocs() const;

		i32 vtDim_ = 0;
		i32 pageDim_ = 0;
		i32 maxPages_ = 0;
		i32 maxResident_ = 0;

		Core::Vector<VTNode> nodes_;
	};

	/**
	 * Indirection texture.
	 * This is used to map areas of the virtual texture to a page that is resident.
	 */
	struct GRAPHICS_DLL VTIndirection
	{
		/**
		 * @param vtDim Dimensions of virtual texture (vtDim x vtDim)
		 * @param pageDim Dimensions of each page (pageDim x pageDim)
		 */
		VTIndirection(i32 vtDim, i32 pageDim);

		~VTIndirection();

		void SetIndirection(i32 level, i32 pageX, i32 pageY, u8 cacheX, u8 cacheY);

		void FlushIndirection(GPU::CommandList& cmdList);

		struct Indirection
		{
			u8 x;
			u8 y;
			u8 l;
			u8 unused;
		};

		i32 vtDim_ = 0;
		i32 pageDim_ = 0;

		Core::Vector<Indirection> data_;
		Core::Vector<Indirection*> levels_;

		GPU::TextureDesc texDesc_;
		GPU::Handle tex_;
	};

	/**
	 * Page provider.
	 * This is used to provide data when pages of a virtual texture should
	 * be made resident.
	 */
	struct GRAPHICS_DLL IVTPageProvider
	{
		virtual ~IVTPageProvider() {}

		virtual bool RequestPage(i32 idx, i32 level, Graphics::VTPoint dstPoint, Graphics::VTRect srcRect) = 0;
	};

	/**
	 * Manager.
	 */
	struct GRAPHICS_DLL VTManager
	{
		/**
		 * @param vtDim Dimensions of virtual texture (vtDim x vtDim)
		 * @param pageDim Dimensions of each page (pageDim x pageDim)
		 * @param maxResident Maximum pages that can be resident.
		 * @param formats Texture formats.
		 */
		VTManager(i32 vtDim, i32 pageDim, i32 maxResident, Core::ArrayView<const GPU::Format> formats,
		    IVTPageProvider* provider);

		~VTManager();

		i32 CreateTexture(i32 w, i32 h);
		void DestroyTexture(i32 idx);

		void RequestPages();

		/**
		 * Get point for page within cache.
		 */
		VTPoint GetPageCachePoint(i32 idx);

		/**
		 * Get allocation for texture.
		 */
		VTNodeAllocation GetAllocation(i32 idx) const { return allocatedTextures_[idx]; }

		VTAllocator allocator_;
		VTIndirection indirection_;

		Core::Vector<GPU::Format> formats_;
		Core::Vector<GPU::Handle> textures_;
		Core::Vector<GPU::TextureDesc> textureDescs_;

		// Texture allocations.
		Core::Vector<VTNodeAllocation> allocatedTextures_;
		Core::Vector<i32> texturesFreeList_;

		// Page allocation.
		i32 numPagesDim_ = 0;
		Core::Vector<i32> pagesFreeList_;

		IVTPageProvider* provider_ = nullptr;
	};


} // namespace Graphics