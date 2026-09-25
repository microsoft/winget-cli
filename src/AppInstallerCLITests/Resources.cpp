// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerStrings.h>
#include <ChannelStreams.h>
#include <ExecutionReporter.h>
#include <winget/Resources.h>

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

TEST_CASE("Resources_SetLanguageOverride", "[resources]")
{
    // When running unpackaged without a resources.pri next to the binary, no resource loader is
    // available. Probe for that case so this test is deterministic in both packaged and unpackaged
    // execution.
    constexpr std::wstring_view commandArgumentDescriptionKey = L"CommandArgumentDescription"sv;
    auto defaultValue = AppInstaller::StringResource::TryResolveString(commandArgumentDescriptionKey);

    if (!defaultValue)
    {
        // Must not crash, and must report that the override was not applied.
        REQUIRE(!AppInstaller::Resource::SetLanguageOverride("de-DE"));
        REQUIRE(!AppInstaller::StringResource::TryResolveString(commandArgumentDescriptionKey));
        return;
    }

    // Always restore the default language resolution for subsequent tests.
    auto resetOverride = wil::scope_exit([&]() { AppInstaller::Resource::SetLanguageOverride({}); });

    REQUIRE(AppInstaller::Resource::SetLanguageOverride("de-DE"));

    auto overriddenValue = AppInstaller::StringResource::TryResolveString(commandArgumentDescriptionKey);
    REQUIRE(overriddenValue.has_value());
    REQUIRE(overriddenValue.value().get() != defaultValue.value().get());

    // An empty tag resets back to the default language.
    REQUIRE(AppInstaller::Resource::SetLanguageOverride({}));

    auto restoredValue = AppInstaller::StringResource::TryResolveString(commandArgumentDescriptionKey);
    REQUIRE(restoredValue.has_value());
    REQUIRE(restoredValue.value().get() == defaultValue.value().get());
}
