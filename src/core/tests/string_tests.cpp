#include "core/string.h"
#include "core/hash.h"

#include "catch.hpp"

// Compare against std::string, since that's what we are partially implementing.
#include <string>

TEST_CASE("string-test-basic")
{
	Core::String str1;
	std::string str1_ref;
	REQUIRE(str1.size() == str1_ref.size());

	Core::String str2("1");
	std::string str2_ref("1");
	REQUIRE(str2.size() == str2_ref.size());

	Core::String str3("12");
	std::string str3_ref("12");
	REQUIRE(str3.size() == str3_ref.size());
}

TEST_CASE("string-test-assignment")
{
	Core::String str1;
	std::string str1_ref;
	REQUIRE(str1.size() == str1_ref.size());

	str1 = "Hello";
	str1_ref = "Hello";
	REQUIRE(str1.size() == str1_ref.size());

	str1 = ", world";
	str1_ref = ", world";
	REQUIRE(str1.size() == str1_ref.size());
}

TEST_CASE("string-test-append")
{
	Core::String str1;
	std::string str1_ref;
	REQUIRE(str1.size() == str1_ref.size());

	str1 += "Hello";
	str1_ref += "Hello";
	REQUIRE(str1.size() == str1_ref.size());

	str1 += ", world";
	str1_ref += ", world";
	REQUIRE(str1.size() == str1_ref.size());
}

TEST_CASE("string-test-compare")
{
	Core::String str1 = "B";
	std::string str1_ref = "B";

	REQUIRE(str1.compare("A") == str1_ref.compare("A"));
	REQUIRE(str1.compare("B") == str1_ref.compare("B"));
	REQUIRE(str1.compare("C") == str1_ref.compare("C"));

	REQUIRE((str1 == "A") == (str1_ref == "A"));
	REQUIRE((str1 == "B") == (str1_ref == "B"));
	REQUIRE((str1 == "C") == (str1_ref == "C"));

	REQUIRE((str1 != "A") == (str1_ref != "A"));
	REQUIRE((str1 != "B") == (str1_ref != "B"));
	REQUIRE((str1 != "C") == (str1_ref != "C"));

	REQUIRE((str1 < "A") == (str1_ref < "A"));
	REQUIRE((str1 < "B") == (str1_ref < "B"));
	REQUIRE((str1 < "C") == (str1_ref < "C"));

	REQUIRE((str1 > "A") == (str1_ref > "A"));
	REQUIRE((str1 > "B") == (str1_ref > "B"));
	REQUIRE((str1 > "C") == (str1_ref > "C"));

	REQUIRE((str1 <= "A") == (str1_ref <= "A"));
	REQUIRE((str1 <= "B") == (str1_ref <= "B"));
	REQUIRE((str1 <= "C") == (str1_ref <= "C"));

	REQUIRE((str1 >= "A") == (str1_ref >= "A"));
	REQUIRE((str1 >= "B") == (str1_ref >= "B"));
	REQUIRE((str1 >= "C") == (str1_ref >= "C"));
}

TEST_CASE("string-test-printf")
{
	Core::String str1;
	str1.Printf("Hello, %s", "world");
	REQUIRE(str1 == "Hello, world");

	str1.Printf("Hello, %s %u", "world", 0);
	REQUIRE(str1 == "Hello, world 0");
}

TEST_CASE("string-test-appendf")
{
	Core::String str1;
	str1.Appendf("Hello, %s", "world");
	REQUIRE(str1 == "Hello, world");

	str1.Appendf("%s %u", ":", 0);
	REQUIRE(str1 == "Hello, world: 0");
}

TEST_CASE("string-test-hash")
{
	Core::String str1("Test hash");
	REQUIRE(Core::Hash(0, str1) == Core::Hash(0, "Test hash"));
}
