#include "gpu/manager.h"
#include "gpu/utils.h"
#include "client/window.h"

#include "catch.hpp"


namespace
{
}

TEST_CASE("gpu-tests-formats")
{
	for(i32 i = 0; i < (i32)GPU::Format::MAX; ++i)
	{
		auto info = GPU::GetFormatInfo((GPU::Format)i);
		REQUIRE(info.blockW_ > 0);
		REQUIRE(info.blockH_ > 0);
		REQUIRE(info.blockBits_> 0);
	}
}
