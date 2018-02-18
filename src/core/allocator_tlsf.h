#pragma once

#include "core/dll.h"
#include "core/allocator.h"
#include "core/array.h"

// Forward declarations.
extern "C" {
typedef void* tlsf_t;
typedef void* pool_t;
}

namespace Core
{
	/**
	 * This allocator is uses the two-level segregated fit implementation
	 * by Matthew Conte (http://tlsf.baisoku.org).
	 * This is a fast general purpose allocator with O(1) alloc/free when
	 * there is memory available to allocate, otherwise it will allocate an additional
	 * pool rounded up to the minimum pool size specified, and try again.
	 * It is not thread safe.
	 */
	class CORE_DLL AllocatorTLSF : public IAllocator
	{
	public:
		/**
		 * @param parent Parent allocator to pull memory in via.
		 * @param minPoolSize Minimum size to allocate pools from.
		 */
		AllocatorTLSF(IAllocator& parent, i64 minPoolSize = 32 * 1024 * 1024);
		~AllocatorTLSF();

		void* Allocate(i64 bytes, i64 align) override;
		void Deallocate(void* mem) override;
		bool OwnAllocation(void* mem) override;
		i64 GetAllocationSize(void* mem) override;
		void LogStats() const override;

		bool CheckIntegrity();

	private:
		bool AddPool(i64 minSize, i64 minAlign);

		struct Pool
		{
			u8* mem_ = nullptr;
			i64 size_ = 0;
			pool_t pool_;
			Pool* next_ = nullptr;
		};

		IAllocator& parent_;
		tlsf_t tlsf_ = nullptr;
		i64 minPoolSize_ = 0;
		Pool* pool_ = nullptr;
	};

} // namespace Core
