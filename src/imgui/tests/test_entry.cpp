#include "client/manager.h"
#include "core/debug.h"
#include "core/file.h"

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>


int main(int argc, char* const argv[])
{
	Client::Manager::Scoped clientManager;

	// Change to executable path.
	char path[Core::MAX_PATH_LENGTH];
	if(Core::FileSplitPath(argv[0], path, Core::MAX_PATH_LENGTH, nullptr, 0, nullptr, 0))
	{
		Core::FileChangeDir(path);
	}

	auto RetVal = Catch::Session().run(argc, argv);
	if(Core::IsDebuggerAttached() && RetVal != 0)
	{
		DBG_BREAK;
	}

	return RetVal;
}
