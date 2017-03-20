#include "client/window.h"

#include "catch.hpp"

#if PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include "core/os.h"
#endif

using namespace Core;
using namespace Client;

namespace
{
}

TEST_CASE("window-tests-create")
{
	auto* window = new Window("window-tests-create", 0, 0, 640, 480);
	i32 x, y, w, h;
	window->GetPosition(x, y);
	window->GetSize(w, h);
	REQUIRE(x == 0);
	REQUIRE(y == 0);
	REQUIRE(w == 640);
	REQUIRE(h == 480);
	delete window;
}

TEST_CASE("window-tests-position")
{
	auto* window = new Window("window-tests-position", 0, 0, 640, 480);
	i32 x = 32, y = 32;
	window->SetPosition(x, y);
	x = 0;
	y = 0;
	window->GetPosition(x, y);
	REQUIRE(x == 32);
	REQUIRE(y == 32);
	delete window;
}

TEST_CASE("window-tests-size")
{
	auto* window = new Window("window-tests-size", 0, 0, 640, 480);
	i32 w = 32, h = 32;
	window->SetPosition(w, h);
	w = 0;
	h = 0;
	window->GetPosition(w, h);
	REQUIRE(w == 32);
	REQUIRE(h == 32);
	delete window;
}

TEST_CASE("window-tests-platformdata")
{
	auto* window = new Window("window-tests-size", 0, 0, 640, 480);
	auto platformData = window->GetPlatformData();
#if PLATFORM_WINDOWS
	HWND hwnd = (HWND)platformData.handle_;
	REQUIRE(::IsWindow(hwnd));
#else
#error "Need test for platform!"
#endif
	delete window;
}
