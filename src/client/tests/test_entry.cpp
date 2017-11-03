#include "client/manager.h"
#include "core/debug.h"

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>



int main(int argc, char* const argv[])
{
	Client::Manager::Scoped clientManager;

	auto RetVal = Catch::Session().run(argc, argv);
	if(Core::IsDebuggerAttached() && RetVal != 0)
	{
		DBG_BREAK;
	}

	return RetVal;
}
