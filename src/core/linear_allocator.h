#pragma once

#include "core/types.h"
#include "core/vector.h"

namespace Core
{
	class CORE_DLL LinearAllocator
	{
	public:
		LinearAllocator(i32 size, i32 alignment = PLATFORM_ALIGNMENT);
		~LinearAllocator();

		/**
		 * Reset allocator back to 0.
		 */
		void Reset();

		/**
		 * Allocate.
		 * @return Valid pointer if allocation was successful, nullptr if it wasn't.
		 */
		void* Allocate(i32 bytes);

		/**
		 * Allocate enough memory for @a count objects of @a TYPE
		 * @return Enough memory for objects, non-constructed. nullptr if unable to allocate.
		 */
		template<typename TYPE>
		TYPE* Allocate(i32 count)
		{
			return reinterpret_cast<TYPE*>(Allocate(count * sizeof(TYPE)));
		}

	private:
		LinearAllocator(const LinearAllocator&) = delete;

		Core::Vector<u8> base_;
		i32 size_;
		i32 alignment_;
		volatile i32 offset_;
	};
} // namespace Core
