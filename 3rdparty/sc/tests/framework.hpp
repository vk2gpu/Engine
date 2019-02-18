//---
// Copyright (c) 2016 Johan Sköld
// License: https://opensource.org/licenses/ISC
//---

#pragma once

#include <string>

// VS2017 has a new warning that triggers in <algorithm> with the current
// version of catch.
#if defined(_MSC_VER) && !defined(__clang__)
#   pragma warning(push)
#   pragma warning(disable: 4244) // 'argument': conversion from '_Diff' to 'Catch::RandomNumberGenerator::result_type', possible loss of data
#endif

#include <catch.hpp>

#if defined(_MSC_VER) && !defined(__clang__)
#   pragma warning(pop)
#endif

#define DESCRIBE(...)   TEST_CASE(__VA_ARGS__)
#define IT(desc)        SECTION(std::string("      It: ") + desc, "")

