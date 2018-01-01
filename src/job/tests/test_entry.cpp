#include "core/debug.h"

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>


int main(int argc, char* const argv[])
{
	auto RetVal = Catch::Session().run(argc, argv);
	if(Core::IsDebuggerAttached() && RetVal != 0)
	{
		DBG_ASSERT(false);
	}
	return RetVal;
}
