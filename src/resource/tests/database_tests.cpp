#include "catch.hpp"

#include "core/debug.h"
#include "core/uuid.h"

#include "resource/private/database.h"

TEST_CASE("resource-tests-database")
{
	Resource::Database database;

	Core::UUID resA;
	Core::UUID resB;
	Core::UUID resC;

	// Add 2 resources.
	REQUIRE(database.AddResource(resA, "my/resource/A.png"));
	REQUIRE(database.AddResource(resB, "my/resource/B.png"));

	// Make A depend on B, and try to make B depend on A.
	REQUIRE(database.AddDependencies(resA, &resB, 1));
	REQUIRE(!database.AddDependencies(resB, &resA, 1));

	// Add a 3rd resource.
	REQUIRE(database.AddResource(resC, "my/resource/C.png"));

	// Make B depend on C, then try to make C depend on A.
	REQUIRE(database.AddDependencies(resB, &resC, 1));
	REQUIRE(!database.AddDependencies(resC, &resA, 1));


	// Try to make A depend on an invalid resource.
	Core::UUID resInvalid("Invalid resource!");

	REQUIRE(!database.AddDependencies(resA, &resInvalid, 1));
}
