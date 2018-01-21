#include "core/debug.h"
#include "core/misc.h"

#include "core/external_allocator.h"

#include "catch.hpp"

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
