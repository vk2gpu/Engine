#include "catch.hpp"

#include "core/debug.h"
#include "core/uuid.h"
#include "core/vector.h"

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
	//
	// A -----> B
	//
	REQUIRE(database.AddDependencies(resA, &resB, 1));
	REQUIRE(!database.AddDependencies(resB, &resA, 1));

	// Add a 3rd resource.
	REQUIRE(database.AddResource(resC, "my/resource/C.png"));

	// Make B depend on C, then try to make C depend on A.
	//
	// A -----> B -----> C
	//
	REQUIRE(database.AddDependencies(resB, &resC, 1));
	REQUIRE(!database.AddDependencies(resC, &resA, 1));


	// Try to make A depend on an invalid resource.
	Core::UUID resInvalid("Invalid resource!");

	REQUIRE(!database.AddDependencies(resA, &resInvalid, 1));

	// Check that we can gathered dependencies correctly.
	Core::Vector<Core::UUID> depUuids;
	depUuids.resize(100);

	REQUIRE(1 == database.GetDependencies(depUuids.data(), depUuids.size(), resA, false));

	REQUIRE(2 == database.GetDependencies(depUuids.data(), depUuids.size(), resA, true));

	// Add a root resource and make depend on 2 others.
	//
	// /- Root -\
	// v        v
	// A -----> B -----> C
	//
	Core::UUID resRoot;
	REQUIRE(database.AddResource(resRoot, "my/resource/Root.level"));
	REQUIRE(database.AddDependencies(resRoot, &resA, 1));
	REQUIRE(database.AddDependencies(resRoot, &resB, 1));

	// Check dependencies match up.
	REQUIRE(2 == database.GetDependencies(depUuids.data(), depUuids.size(), resRoot, false));
	REQUIRE(3 == database.GetDependencies(depUuids.data(), depUuids.size(), resRoot, true));

	// Now make root depend on C
	//
	// /- Root ----------\
	// v        v        v
	// A -----> B -----> C
	REQUIRE(database.AddDependencies(resRoot, &resC, 1));
	REQUIRE(3 == database.GetDependencies(depUuids.data(), depUuids.size(), resRoot, false));
	REQUIRE(3 == database.GetDependencies(depUuids.data(), depUuids.size(), resRoot, true));

	// Try to make C depend on root.
	REQUIRE(!database.AddDependencies(resC, &resRoot, 1));
	REQUIRE(3 == database.GetDependencies(depUuids.data(), depUuids.size(), resRoot, false));

	// Remove dependency on C.
	//
	// /- Root -\
	// v        v
	// A -----> B -----> C
	REQUIRE(database.RemoveDependency(resC, &resRoot, 1));
	REQUIRE(2 == database.GetDependencies(depUuids.data(), depUuids.size(), resRoot, false));
	REQUIRE(0 == database.GetDependents(depUuids.data(), depUuids.size(), resRoot));
	REQUIRE(1 == database.GetDependents(depUuids.data(), depUuids.size(), resA));
	REQUIRE(2 == database.GetDependents(depUuids.data(), depUuids.size(), resB));
	REQUIRE(3 == database.GetDependents(depUuids.data(), depUuids.size(), resC));

	// Remove dependency on A.
	//
	//    Root -\
	//          v
	// A -----> B -----> C
	REQUIRE(database.RemoveDependency(resA, &resRoot, 1));
	REQUIRE(1 == database.GetDependencies(depUuids.data(), depUuids.size(), resRoot, false));
	REQUIRE(0 == database.GetDependents(depUuids.data(), depUuids.size(), resRoot));
	REQUIRE(0 == database.GetDependents(depUuids.data(), depUuids.size(), resA));
	REQUIRE(2 == database.GetDependents(depUuids.data(), depUuids.size(), resB));
	REQUIRE(3 == database.GetDependents(depUuids.data(), depUuids.size(), resC));

	// Add dependency on A.
	//
	// /- Root -\
	// v        v
	// A -----> B -----> C
	REQUIRE(database.AddDependencies(resRoot, &resA, 1));
	REQUIRE(2 == database.GetDependencies(depUuids.data(), depUuids.size(), resRoot, false));
	REQUIRE(0 == database.GetDependents(depUuids.data(), depUuids.size(), resRoot));
	REQUIRE(1 == database.GetDependents(depUuids.data(), depUuids.size(), resA));
	REQUIRE(2 == database.GetDependents(depUuids.data(), depUuids.size(), resB));
	REQUIRE(3 == database.GetDependents(depUuids.data(), depUuids.size(), resC));

	// Remove root.
	//
	// A -----> B -----> C
	//
	REQUIRE(database.RemoveResource(resRoot, false));

	REQUIRE(!database.GetResourceData(nullptr, resRoot));
	REQUIRE(database.GetResourceData(nullptr, resA));
	REQUIRE(database.GetResourceData(nullptr, resB));
	REQUIRE(database.GetResourceData(nullptr, resC));

	REQUIRE(0 == database.GetDependencies(depUuids.data(), depUuids.size(), resRoot, false));
	REQUIRE(0 == database.GetDependents(depUuids.data(), depUuids.size(), resRoot));
	REQUIRE(0 == database.GetDependents(depUuids.data(), depUuids.size(), resA));
	REQUIRE(1 == database.GetDependents(depUuids.data(), depUuids.size(), resB));
	REQUIRE(2 == database.GetDependents(depUuids.data(), depUuids.size(), resC));

	// Remove B and all dependents.
	//
	// C
	//
	REQUIRE(database.RemoveResource(resB, true));

	REQUIRE(!database.GetResourceData(nullptr, resRoot));
	REQUIRE(!database.GetResourceData(nullptr, resA));
	REQUIRE(!database.GetResourceData(nullptr, resB));
	REQUIRE(database.GetResourceData(nullptr, resC));

	REQUIRE(0 == database.GetDependencies(depUuids.data(), depUuids.size(), resC, false));
	REQUIRE(0 == database.GetDependents(depUuids.data(), depUuids.size(), resC));
}
