#include "core/debug.h"

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>
#include <reporters/catch_reporter_teamcity.hpp>

#define SDL_MAIN_HANDLED
#include <SDL.h>

int main(int argc, char* const argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);

	auto RetVal = Catch::Session().run(argc, argv);
	if(Core::IsDebuggerAttached() && RetVal != 0)
	{
		DBG_BREAK;
	}
	return RetVal;
}
