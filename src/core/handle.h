#pragma once

#include "core/types.h"
#include "core/dll.h"

namespace Core
{
	/**
	 * Handle.
	 */
	class CORE_DLL Handle
	{
	public:
		enum
		{
			MAX_INDEX = 65536,
			MAX_MAGIC = 4096,
			MAX_TYPE = 16,
		};

		Handle()
		    : handle_(0)
		{
		}

		/**
		 * Get handle index.
		 */
		i32 GetIndex() const { return index_; }

		/**
		 * Get handle type.
		 */
		i32 GetType() const { return type_; }

		/**
		 * Get combined type & index.
		 */
		i32 GetCombined() const { return type_ << 16 | index_; }

		/**
		 * Validity check.
		 */
		operator bool() const { return handle_ != 0; }

		bool operator<(const Handle& other) { return handle_ < other.handle_; }
		bool operator==(const Handle& other) { return handle_ == other.handle_; }
		bool operator!=(const Handle& other) { return handle_ != other.handle_; }

	private:
		friend class HandleAllocator;

		union
		{
			struct
			{
				;
				u32 index_ : 16;
				u32 magic_ : 12;
				u32 type_ : 4;
			};
			u32 handle_;
		};
	};

	/**
	 * Handle allocator.
	 * Provides a mechanism for allocating and validating handles for use in
	 * various scenarios.
	 * This is not thread safe, so this needs to be managed at the higher level.
	 */
	class CORE_DLL HandleAllocator
	{
	public:
		/**
		 * Create handle allocator.
		 * @param numTypes Maximum number of types to support.
		 */
		HandleAllocator(i32 numTypes);

		/**
		 * Create handle allocator.
		 * @param numTypes Maximum number of types to support.
		 */
		template<typename TYPE_ENUM>
		HandleAllocator(TYPE_ENUM numTypes)
		    : HandleAllocator((i32)numTypes)
		{
		}

		~HandleAllocator();

		/**
		 * Allocate handle.
		 * @param type Type of handle.
		 */
		Handle Alloc(i32 type);

		/**
		 * Allocate handle using type enum.
		 * @param type Type of handle.
		 */
		template<typename HANDLE_TYPE, typename TYPE_ENUM>
		HANDLE_TYPE Alloc(TYPE_ENUM type)
		{
			return HANDLE_TYPE(Alloc((i32)type));
		}

		/**
		 * Free handle.
		 */
		void Free(Handle handle);

		/**
		 * Get total number of allocated handles for type.
		 * SLOW: Iterates internally to check.
		 */
		i32 GetTotalHandles(i32 type) const;

		/**
		 * Get total number of allocated handles using enum type type.
		 * SLOW: Iterates internally to check.
		 */
		template<typename TYPE_ENUM>
		Handle GetTotalHandles(TYPE_ENUM type) const
		{
			return GetTotalHandles((i32)type);
		}

		/**
		 * Get maximum handle index.
		 */
		i32 GetMaxHandleIndex(i32 type) const;

		/**
		 * Is handle index allocated?
		 */
		bool IsHandleIndexAllocated(i32 type, i32 index) const;

		/**
		 * Is a handle valid?
		 */
		bool IsValid(Handle handle) const
		{
			return magicIDs_[handle.index_ + (handle.type_ * Handle::MAX_MAGIC)] == handle.magic_;
		}

	private:
		/// Magic IDs array used to validate handles for types.
		u16* magicIDs_ = nullptr;
		struct HandleAllocatorImpl* impl_ = nullptr;
	};

} // namespace Core
