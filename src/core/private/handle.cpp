#include "core/handle.h"
#include "core/array.h"
#include "core/debug.h"
#include "core/vector.h"

#include <utility>

namespace Core
{
	struct HandleAllocatorImpl
	{
		struct TypeData
		{
			Core::Vector<u32> freeList_;
			Core::Vector<u8> allocated_;
		};

		Core::Array<TypeData, Handle::MAX_TYPE> types_;
	};

	namespace
	{
		inline u16& GetMagicID(u16* magicIds, u32 index, u32 type)
		{
			return magicIds[index + (type * Handle::MAX_MAGIC)];
		}
	}

	HandleAllocator::HandleAllocator(i32 numTypes)
	{
		const i32 magicSize = Handle::MAX_TYPE * Handle::MAX_MAGIC;
		magicIDs_ = new u16[magicSize];
		for(i32 i = 0; i < magicSize; ++i)
			magicIDs_[i] = 1;
		impl_ = new HandleAllocatorImpl();
	}

	HandleAllocator::~HandleAllocator()
	{
		delete impl_;
		delete[] magicIDs_;
	}

	Handle HandleAllocator::Alloc(i32 type)
	{
		DBG_ASSERT(type >= 0 && type < impl_->types_.size());
		HandleAllocatorImpl::TypeData& typeData = impl_->types_[type];

		Handle handle;

		// Reuse index from freelist if we can.
		bool validHandle = false;
		if(typeData.freeList_.size() > 0)
		{
			handle.index_ = typeData.freeList_.back();
			typeData.freeList_.pop_back();
			handle.type_ = type;
			validHandle = true;
		}
		else
		{
			// Nothing in the free list, push new one into allocated.
			if(typeData.allocated_.size() < Handle::MAX_INDEX)
			{
				typeData.allocated_.push_back(0);
				handle.index_ = typeData.allocated_.size() - 1;
				handle.type_ = type;
				validHandle = true;
			}
		}

		if(validHandle)
		{
			if((i32)handle.index_ >= typeData.allocated_.size())
			{
				typeData.allocated_.resize((i32)handle.index_ + 1, 0);
			}
			DBG_ASSERT(typeData.allocated_[handle.index_] == 0);
			typeData.allocated_[handle.index_] = 1;
			handle.magic_ = GetMagicID(magicIDs_, handle.index_, handle.type_);
			DBG_ASSERT(handle.magic_ != 0);
		}
		return handle;
	}

	void HandleAllocator::Free(Handle handle)
	{
		DBG_ASSERT_MSG(IsValid(handle), "Attempting to free invalid handle.");
		DBG_ASSERT((i32)handle.type_ < impl_->types_.size());
		HandleAllocatorImpl::TypeData& typeData = impl_->types_[handle.type_];

		// Increment magic, wrap if hit zero.
		u16& magic = GetMagicID(magicIDs_, handle.index_, handle.type_);
		if((++magic) >= Handle::MAX_MAGIC)
		{
			magic = 1;
		}

		// Mark unallocated.
		typeData.allocated_[handle.index_] = 0;

		// Add to free list.
		typeData.freeList_.push_back(handle.index_);
	}


	i32 HandleAllocator::GetTotalHandles(i32 type) const
	{
		DBG_ASSERT((i32)type < impl_->types_.size());
		HandleAllocatorImpl::TypeData& typeData = impl_->types_[type];
		i32 totalHandles = 0;
		for(auto alloc : typeData.allocated_)
		{
			if(alloc)
				++totalHandles;
		}
		return totalHandles;
	}


} // namespace Core
