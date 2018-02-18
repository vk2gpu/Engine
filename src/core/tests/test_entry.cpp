#include "core/debug.h"
#include "core/allocator_overrides.h"

DECLARE_MODULE_ALLOCATOR("General/" MODULE_NAME);

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
