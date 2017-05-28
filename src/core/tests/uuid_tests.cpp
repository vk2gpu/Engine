#include "core/uuid.h"
#include "core/debug.h"
#include "core/random.h"

#include "catch.hpp"

TEST_CASE("uuid-tests-string")
{
	Core::UUID uuid;
	char outBuffer[128] = {0};

	uuid = Core::UUID("uuid-tests-string::test_uuid_0");
	uuid.AsString(outBuffer);
	REQUIRE(strcmp(outBuffer, "4e7e8817-9538-562a-167c-dd649f04ead3") == 0);

	uuid = Core::UUID("uuid-tests-string::test_uuid_1");
	uuid.AsString(outBuffer);
	REQUIRE(strcmp(outBuffer, "102dffdb-5b8e-5511-016f-b9452eadbd85") == 0);
}

TEST_CASE("uuid-tests-random")
{
	Core::Random rng0;
	Core::Random rng1;
	Core::UUID uuid0;
	Core::UUID uuid1;

	// Check sequential.
	uuid0 = Core::UUID(rng0, 0);
	uuid1 = Core::UUID(rng0, 0);
	REQUIRE(uuid0 != uuid1);

	// Check sequential.
	uuid0 = Core::UUID(rng1, 1);
	uuid1 = Core::UUID(rng1, 1);
	REQUIRE(uuid0 != uuid1);

	// Check varient.
	uuid0 = Core::UUID(rng0, 0);
	uuid1 = Core::UUID(rng1, 1);
	REQUIRE(uuid0 != uuid1);
}

TEST_CASE("uuid-tests-as-from-string")
{
	char outBuffer[128] = {0};
	auto uuid = Core::UUID("uuid-tests-string::test_uuid_0");
	uuid.AsString(outBuffer);

	Core::UUID uuid2;
	REQUIRE(uuid2.FromString(outBuffer));

	REQUIRE(uuid == uuid2);
}
