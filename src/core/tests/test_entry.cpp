#define CATCH_CONFIG_RUNNER
#include <catch.hpp>
#include <reporters/catch_reporter_teamcity.hpp>

int main(int argc, char* const argv[])
{
	auto RetVal = Catch::Session().run(argc, argv);
	return 0;
}
