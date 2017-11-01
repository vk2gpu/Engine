#pragma once

#include "core/dll.h"
#include "core/types.h"

// Forward declarations.
extern "C" {
struct etlsf_private_t;
typedef struct etlsf_private_t* etlsf_t;
}

namespace Core
{
	/// Allocation.
	struct ExternalAlloc
	{
		i32 offset_ = -1;
		i32 size_ = -1;
	};

	/**
	 * External allocator where metadata should not be stored within target memory.
	 * This uses Mykhailo Parfeniuk's external TLSF implementation.
	 * See etlsf.h for copyright & license.
	 */
	class CORE_DLL ExternalAllocator
	{
	public:
		/// Size in which allocations are aligned up to.
		static const i32 SIZE_ALIGNMENT = 256;

		/**
		 * @param size Size of arena to allow allocation from.
		 * @param maxAllocations Maximum number of allocations allowed.
		 */
		ExternalAllocator(i32 size, i32 maxAllocations);

		~ExternalAllocator();

		/**
		 * Allocate range.
		 * @param size Allocation size.
		 * @return Allocation ID.
		 */
		u16 AllocRange(i32 size);

		/**
		 * Free range.
		 * @param id Allocation ID.
		 */
		void FreeRange(u16 id);

		/**
		 * Get allocation.
		 * @return Structure containing offset and size of allocation.
		 */
		ExternalAlloc GetAlloc(u16 id) const;

	private:
		ExternalAllocator(const ExternalAllocator&) = delete;
		ExternalAllocator& operator=(const ExternalAllocator&) = delete;

		etlsf_t arena_ = nullptr;
	};
} // namespace Core

#if CODE_INLINE
#include "core/private/external_allocator.inl"
#endif
