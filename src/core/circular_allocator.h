#pragma once

#include "core/types.h"
#include "core/concurrency.h"
#include "core/vector.h"

namespace Core
{
	class CORE_DLL CircularAllocator
	{
	public:
		CircularAllocator(i32 size, i32 alignment = PLATFORM_ALIGNMENT);
		~CircularAllocator();

		/**
		 * Release a number of bytes.
		 * @param bytes Number of bytes to release.
		 */
		void Release(i32 bytes);

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
		CircularAllocator(const CircularAllocator&) = delete;

		Core::Vector<u8> base_;
		i32 size_;
		i32 mask_;
		i32 alignment_;

		Core::SpinLock spinLock_;
		volatile u64 offset_;
		volatile u64 released_;
	};
} // namespace Core
