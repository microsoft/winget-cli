// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Util.h"

#include <catch2/catch_test_macros.hpp>

#define TEST(...) TEST_CASE("[UtilTests] " __VA_ARGS__)

using namespace SFS::details::util;

TEST("Testing AreEqualI")
{
    SECTION("ASCII strings")
    {
        REQUIRE(AreEqualI("abc", "abc"));
        REQUIRE(AreEqualI("abc", "ABC"));
        REQUIRE(AreEqualI("abc", "Abc"));
        REQUIRE(AreEqualI("abc", "aBc"));
        REQUIRE(AreEqualI("abc", "abC"));
        REQUIRE(AreEqualI("abc", "ABC"));
        REQUIRE(AreEqualI("abc", "AbC"));
        REQUIRE(AreEqualI("abc", "aBC"));
    }
}

TEST("Testing AreNotEqualI")
{
    REQUIRE(AreNotEqualI("abc", "ab"));
    REQUIRE(AreNotEqualI("ab", "abc"));
    REQUIRE(AreNotEqualI("abc", "abd"));
}
