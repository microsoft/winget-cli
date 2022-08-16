// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerStrings.h>
#include <ChannelStreams.h>
#include <ExecutionReporter.h>

using namespace std::string_view_literals;
using namespace AppInstaller::CLI::Resource;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Utility::literals;
using namespace AppInstaller::CLI::Execution;

#define WINGET_TEST_OUTPUT_STREAM(_expected_, _input_) \
    std::istringstream iInput; \
    std::ostringstream oInput; \
    std::istringstream iExpected; \
    std::ostringstream oExpected; \
    Reporter(oInput, iInput).Info() << _input_; \
    Reporter(oExpected, iExpected).Info() << _expected_; \
    REQUIRE(oExpected.str()== oInput.str());

TEST_CASE("Resources_StringId", "[resources]")
{
    WINGET_TEST_OUTPUT_STREAM(
        "Filter results by command"_liv,
        String::CommandArgumentDescription
    );
}

TEST_CASE("Resources_StringIdWithPlaceholders", "[resources]")
{
    WINGET_TEST_OUTPUT_STREAM(
        "The value provided for the `First` argument is invalid; valid values are: Second"_liv ,
        String::InvalidArgumentValueError("First"_liv, "Second"_liv)
    );
}
