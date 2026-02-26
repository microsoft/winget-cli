// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "sfsclient/Result.h"

#include <catch2/catch_test_macros.hpp>

#define TEST(...) TEST_CASE("[ResultTests] " __VA_ARGS__)

using namespace SFS;

TEST("Testing Result() class methods")
{
    SECTION("Default constructor")
    {
        Result resultSuccess(Result::Code::Success);

        REQUIRE(resultSuccess.GetCode() == Result::Code::Success);
        REQUIRE(resultSuccess.GetMsg().empty());
        REQUIRE(resultSuccess.IsSuccess());
        REQUIRE_FALSE(resultSuccess.IsFailure());

        INFO("Comparison operators");
        REQUIRE(resultSuccess == Result::Code::Success);
        REQUIRE(resultSuccess == Result::Success);
        REQUIRE(resultSuccess != Result::Code::NotSet);
        REQUIRE(resultSuccess != Result::NotSet);
        REQUIRE_FALSE(resultSuccess == Result::NotSet);

        INFO("bool operator");
        REQUIRE(resultSuccess);
    }

    SECTION("Constructor with message")
    {
        Result resultUnexpected(Result::Code::Unexpected, "message");
        REQUIRE(resultUnexpected.GetCode() == Result::Code::Unexpected);
        REQUIRE(resultUnexpected.GetMsg() == "message");
        REQUIRE_FALSE(resultUnexpected.IsSuccess());
        REQUIRE(resultUnexpected.IsFailure());

        INFO("Comparison operators on constructor with message");
        REQUIRE(resultUnexpected == Result::Code::Unexpected);
        REQUIRE(resultUnexpected != Result::Code::NotSet);

        INFO("bool operator");
        REQUIRE_FALSE(resultUnexpected);
    }
}

TEST("Testing ToString(Result)")
{
    REQUIRE(SFS::ToString(Result::Code::Success) == "Success");
    REQUIRE(SFS::ToString(Result::Code::NotImpl) == "NotImpl");
    REQUIRE(SFS::ToString(Result::Code::NotSet) == "NotSet");
    REQUIRE(SFS::ToString(Result::Code::OutOfMemory) == "OutOfMemory");
    REQUIRE(SFS::ToString(Result::Code::Unexpected) == "Unexpected");
}
