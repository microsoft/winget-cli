// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerStrings.h>
#include <ChannelStreams.h>
#include <ExecutionReporter.h>

using namespace std::string_view_literals;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Utility::literals;
using namespace AppInstaller::CLI;

#define WINGET_TEST_OUTPUT_STREAM(_expected_, _input_) \
    do { \
        std::istringstream iInput; \
        std::ostringstream oInput; \
        std::istringstream iExpected; \
        std::ostringstream oExpected; \
        Execution::Reporter(oInput, iInput).Info() << _input_; \
        Execution::Reporter(oExpected, iExpected).Info() << _expected_; \
        REQUIRE(oExpected.str()== oInput.str()); \
    } while(0);

TEST_CASE("Resources_StringId", "[resources]")
{
    WINGET_TEST_OUTPUT_STREAM(
        "Filter results by command"_liv,
        Resource::String::CommandArgumentDescription
    );
}

TEST_CASE("Resources_StringIdWithPlaceholders_LocIndString", "[resources]")
{
    WINGET_TEST_OUTPUT_STREAM(
        "The value provided for the `First` argument is invalid; valid values are: Second"_liv ,
        Resource::String::InvalidArgumentValueError("First"_liv, "Second"_liv)
    );
}

TEST_CASE("Resources_StringIdWithPlaceholders_StringId", "[resources]")
{
    WINGET_TEST_OUTPUT_STREAM(
        "This operation is disabled by Group Policy: Enable Additional Windows App Installer Sources"_liv ,
        Resource::String::DisabledByGroupPolicy(AppInstaller::StringResource::String::PolicyAdditionalSources)
    );
}

TEST_CASE("Resources_StringIdWithPlaceholders_Arithmetic", "[resources]")
{
    WINGET_TEST_OUTPUT_STREAM(
        "42 upgrades available."_liv ,
        Resource::String::AvailableUpgrades(42)
    );
}
