#pragma once

#include "core/types.h"

namespace Core
{
	/**
	 * Allocator for use with containers (and other)
	 * TODO: Make it always allocate with correct alignment for platform.
	 */
	class Allocator
	{
	public:
		using index_type = i32; // TODO: Template arg?

		Allocator() = default;
		Allocator(const Allocator&) = default;
		~Allocator() = default;

		void* allocate(index_type count, index_type size) { return InternalAllocate(count, size); }

		void deallocate(void* mem, index_type /* count */, index_type size) { InternalDeallocate(mem, sizeof(size)); }

	private:
		void* InternalAllocate(index_type count, index_type size) { return ::new u8[count * size]; }

		void InternalDeallocate(void* mem, index_type /*size*/) { delete[] static_cast<u8*>(mem); }
	};


} // namespace Core
