#include "core/allocator.h"
#include "core/array.h"
#include "core/concurrency.h"
#include "core/debug.h"
#include "core/misc.h"

#define ENABLE_GUARD_PAGES (1)
#define ENABLE_DEFAULT_ALLOCATION_TRACKER !defined(_RELEASE)

#define GENERAL_PURPOSE_MIN_POOL_SIZE (8 * 1024 * 1024)
#define GENERAL_PURPOSE_MAX_ALIGN (64 * 1024)

#include "core/allocator_proxy_tracker.h"
#include "core/allocator_proxy_thread_safe.h"
#include "core/allocator_virtual.h"
#include "core/allocator_tlsf.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Core
{
	namespace
	{
		struct AllocatorList
		{
			struct Entry
			{
				Entry* next_ = nullptr;
				IAllocator* allocator_ = nullptr;
			};

			Entry* entry_ = nullptr;

			AllocatorList() {}

			~AllocatorList()
			{
				Entry* nextEntry = nullptr;
				for(Entry* entry = entry_; entry != nullptr; entry = nextEntry)
				{
					nextEntry = entry->next_;
					UntrackedVirtualAllocator().Delete(entry->allocator_);
					UntrackedVirtualAllocator().Delete(entry);
				}
			}

			IAllocator& Add(IAllocator& allocator, const char* name)
			{
				Entry* entry = UntrackedVirtualAllocator().New<Entry>();
				entry->allocator_ = UntrackedVirtualAllocator().New<AllocatorProxyTracker>(allocator, name);

				for(;;)
				{
					Entry* oldEntry = entry_;
					entry->next_ = oldEntry;

					if(Core::AtomicCmpExchgPtr(&entry_, entry, oldEntry) == oldEntry)
						break;
				}

				return *entry->allocator_;
			}
		};

		static AllocatorList& GetAllocatorList()
		{
			static AllocatorList allocatorList;
			return allocatorList;
		}
	}

	IAllocator& UntrackedVirtualAllocator()
	{
		static AllocatorVirtual virtAlloc(ENABLE_GUARD_PAGES);
		return virtAlloc;
	}

	IAllocator& VirtualAllocator()
	{
#if ENABLE_DEFAULT_ALLOCATION_TRACKER
		static IAllocator& proxy = CreateAllocationTracker(UntrackedVirtualAllocator(), "Virtual");
		return proxy;
#else
		return virtAlloc;
#endif // ENABLE_DEFAULT_ALLOCATION_TRACKER
	}

	IAllocator& GeneralAllocator()
	{
		static AllocatorTLSF tlsfAlloc(VirtualAllocator(), GENERAL_PURPOSE_MIN_POOL_SIZE);
		static AllocatorProxyThreadSafe tsProxy(tlsfAlloc);
#if ENABLE_DEFAULT_ALLOCATION_TRACKER
		static IAllocator& proxy = CreateAllocationTracker(tsProxy, "General");
		return proxy;
#else
		return tsProxy;
#endif // ENABLE_DEFAULT_ALLOCATION_TRACKER
	}

	IAllocator& CreateAllocationTracker(IAllocator& allocator, const char* name)
	{
		return GetAllocatorList().Add(allocator, name);
	}

} // namespace Core
