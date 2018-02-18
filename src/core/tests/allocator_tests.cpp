#include "core/debug.h"
#include "core/misc.h"

#include "core/allocator.h"
#include "core/allocator_tlsf.h"
#include "core/allocator_virtual.h"
#include "core/allocator_proxy_tracker.h"
#include "core/external_allocator.h"
#include "core/random.h"
#include "core/string.h"
#include "core/timer.h"
#include "math/vec4.h"

#include "catch.hpp"

TEST_CASE("allocator-tests-allocator-virtual")
{
	Core::AllocatorVirtual virtAlloc(true);

	const i64 size = 64 * 1024;
	const i32 iters = 32;
	for(i32 i = 0; i < iters; ++i)
	{
		u8* mem = (u8*)virtAlloc.Allocate(size, 4096);
		REQUIRE(mem);

		for(i32 j = 0; j < size; ++j)
			mem[j] = (u8)j;

		virtAlloc.Deallocate(mem);
	}
}

TEST_CASE("allocator-tests-allocator-tlsf")
{
	Core::AllocatorVirtual virtAlloc(true);
	Core::AllocatorTLSF tlsfAlloc(virtAlloc, 1024 * 1024);

	auto testAllocations = [&](i64 maxSize, i64 align, i32 iters) {
		Core::Random rng;
		Core::Array<void*, 256> allocs = {};
		REQUIRE(iters <= allocs.size());
		for(i32 i = 0; i < iters; ++i)
		{
			i32 rand = rng.Generate();
			i64 size = Core::Max(1, rand % maxSize);

			u8* mem = (u8*)tlsfAlloc.Allocate(size, align);
			REQUIRE(mem);
			REQUIRE(((i64)mem & (align - 1)) == 0);

			REQUIRE(tlsfAlloc.OwnAllocation(mem));
			REQUIRE(tlsfAlloc.GetAllocationSize(mem) >= size);

			REQUIRE(!virtAlloc.OwnAllocation(mem));
			REQUIRE(virtAlloc.GetAllocationSize(mem) < 0);

			for(i32 j = 0; j < size; ++j)
				mem[j] = (u8)j;

			if((i % 4) == 0)
				tlsfAlloc.Deallocate(mem);
			else
				allocs[i] = mem;
		}

		REQUIRE(tlsfAlloc.CheckIntegrity());

		for(auto* alloc : allocs)
			tlsfAlloc.Deallocate(alloc);
	};

	for(i32 size = 1; size <= (8 * 1024 * 1024); size *= 4)
		for(i32 align = 1; align <= 4096; align *= 2)
			testAllocations(size, align, 4);
}

TEST_CASE("allocator-tests-allocator-general")
{
	Core::AllocatorVirtual virtAlloc(true);
	Core::AllocatorTLSF tlsfAlloc(virtAlloc, 1024 * 1024);
	Core::AllocatorProxyTracker trackerAlloc(tlsfAlloc, "Test");

	Core::IAllocator& allocator = tlsfAlloc;

	auto testAllocations = [&](i64 maxSize, i64 align, i32 iters) {
		Core::Random rng;
		Core::Array<void*, 256> allocs = {};
		REQUIRE(iters <= allocs.size());
		for(i32 i = 0; i < iters; ++i)
		{
			i32 rand = rng.Generate();
			i64 size = Core::Max(1, rand % maxSize);

			u8* mem = (u8*)allocator.Allocate(size, align);
			REQUIRE(mem);
			REQUIRE(((i64)mem & (align - 1)) == 0);

			if((i % 4) == 0)
				allocator.Deallocate(mem);
			else
				allocs[i] = mem;
		}

		for(auto* alloc : allocs)
		{
			allocator.Deallocate(alloc);
		}
	};

	auto testAllocations2 = [&](i64 maxSize, i64 align, i32 iters) {
		Core::Random rng;
		Core::Array<void*, 256> allocs = {};
		REQUIRE(iters <= allocs.size());
		for(i32 i = 0; i < iters; ++i)
		{
			i32 rand = rng.Generate();
			i64 size = Core::Max(1, rand % maxSize);

			u8* mem = (u8*)std::malloc(size);
			REQUIRE(mem);

			if((i % 4) == 0)
				std::free(mem);
			else
				allocs[i] = mem;
		}

		for(auto* alloc : allocs)
			std::free(alloc);
	};

	{
		Core::Timer timer;
		timer.Mark();
		for(i32 size = 1; size <= (32 * 1024 * 1024); size *= 4)
			for(i32 align = 1; align <= 4096; align *= 2)
				testAllocations(size, align, 16);
		Core::Log("TLSF->Virtual Allocator: %f us\n", (f32)(timer.GetTime() * 1000000.0));
	}

	{
		Core::Timer timer;
		timer.Mark();
		for(i32 size = 1; size <= (32 * 1024 * 1024); size *= 4)
			for(i32 align = 1; align <= 4096; align *= 2)
				testAllocations2(size, align, 16);
		Core::Log("Standard Allocator: %f us\n", (f32)(timer.GetTime() * 1000000.0));
	}
}


TEST_CASE("allocator-tests-etlsf-small")
{
	const i32 MAX_SIZE = 1024 * 1024;
	Core::ExternalAllocator allocator(MAX_SIZE, 0xffff);

	auto allocId0 = allocator.AllocRange(1);
	auto alloc0 = allocator.GetAlloc(allocId0);
	REQUIRE(alloc0.offset_ >= 0);
	REQUIRE(alloc0.size_ == 1);

	auto allocId1 = allocator.AllocRange(2);
	auto alloc1 = allocator.GetAlloc(allocId1);
	REQUIRE(alloc1.offset_ != alloc0.offset_);
	REQUIRE(alloc1.offset_ >= 0);
	REQUIRE(alloc1.size_ == 2);

	auto allocId2 = allocator.AllocRange(3);
	auto alloc2 = allocator.GetAlloc(allocId2);
	REQUIRE(alloc2.offset_ != alloc0.offset_);
	REQUIRE(alloc2.offset_ != alloc1.offset_);
	REQUIRE(alloc2.offset_ >= 0);
	REQUIRE(alloc2.size_ == 3);

	// Test failure.
	auto allocId3 = allocator.AllocRange(MAX_SIZE);
	auto alloc3 = allocator.GetAlloc(allocId3);
	REQUIRE(alloc3.offset_ == -1);
	REQUIRE(alloc3.size_ == -1);

	// Free.
	allocator.FreeRange(allocId0);
	allocator.FreeRange(allocId1);
	allocator.FreeRange(allocId2);

	// Now try allocate max.
	auto allocId4 = allocator.AllocRange(MAX_SIZE);
	auto alloc4 = allocator.GetAlloc(allocId4);
	REQUIRE(alloc4.offset_ >= 0);
	REQUIRE(alloc4.size_ == MAX_SIZE);
}
