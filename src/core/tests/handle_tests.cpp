#include "core/handle.h"

#include "catch.hpp"

using namespace Core;

TEST_CASE("handle-tests-basic")
{
	HandleAllocator alloc(1);

	// Alloc.
	Handle handle0 = alloc.Alloc(0);
	REQUIRE(alloc.GetTotalHandles(0) == 1);
	Handle handle1 = alloc.Alloc(0);
	REQUIRE(alloc.GetTotalHandles(0) == 2);
	REQUIRE(handle0 != handle1);

	// Free.
	alloc.Free(handle1);
	REQUIRE(alloc.GetTotalHandles(0) == 1);

	// Reuse.
	handle0 = alloc.Alloc(0);
	REQUIRE(handle0 != handle1);
	REQUIRE(handle0.GetIndex() == handle1.GetIndex());
}

TEST_CASE("handle-tests-over-allocate")
{
	HandleAllocator alloc(1);

	for(i32 i = 0; i < Handle::MAX_INDEX; ++i)
	{
		alloc.Alloc(0);
	}
	REQUIRE(alloc.GetTotalHandles(0) == Handle::MAX_INDEX);

	// Now attempt to allocate again, failure expected.
	REQUIRE(alloc.Alloc(0) == Handle());
}

TEST_CASE("handle-tests-types")
{
	HandleAllocator alloc(2);

	Handle handle0 = alloc.Alloc(0);
	REQUIRE(handle0.GetType() == 0);
	REQUIRE(alloc.GetTotalHandles(0) == 1);
	REQUIRE(alloc.GetTotalHandles(1) == 0);

	Handle handle1 = alloc.Alloc(1);
	REQUIRE(handle1.GetType() == 1);
	REQUIRE(alloc.GetTotalHandles(0) == 1);
	REQUIRE(alloc.GetTotalHandles(1) == 1);

	// Free.
	alloc.Free(handle0);
	REQUIRE(alloc.GetTotalHandles(0) == 0);
	REQUIRE(alloc.GetTotalHandles(1) == 1);

	alloc.Free(handle1);
	REQUIRE(alloc.GetTotalHandles(0) == 0);
	REQUIRE(alloc.GetTotalHandles(1) == 0);
}
