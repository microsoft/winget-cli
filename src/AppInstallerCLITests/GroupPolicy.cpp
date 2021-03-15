// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/GroupPolicy.h>

using namespace TestCommon;
using namespace AppInstaller::Settings;
using namespace std::string_view_literals;

namespace
{
    const std::wstring AutoUpdateIntervalValueName = L"SourceAutoUpdateIntervalInMinutes";
    const std::wstring ProgressBarStyleValueName = L"ProgressBarStyle";
    const std::wstring IncludeSourcesKeyName = L"IncludeSources";

    const std::wstring DisableWinGetValueName = L"DisableWinGet";
    const std::wstring DisableSettingsCommandValueName = L"DisableSettingsCommand";
    const std::wstring DisableExperimentalFeaturesValueName = L"DisableExperimentalFeatures";
    const std::wstring DisableLocalManifestFilesValueName = L"DisableLocalManifestFiles";
    const std::wstring DisableSourceConfigurationValueName = L"DisableSourceConfiguration";
    const std::wstring ExcludeDefaultSourcesValueName = L"ExcludeDefaultSources";

    struct GroupPolicyTest : GroupPolicy
    {
        GroupPolicyTest(HKEY key) : GroupPolicy(AppInstaller::Registry::Key(key)) {}
    };
}

TEST_CASE("GroupPolicy_NoPolicies", "[groupPolicy]")
{
    auto policiesKey = RegCreateVolatileTestRoot();
    GroupPolicyTest groupPolicy{ policiesKey.get() };

    // Policies setting a value should be empty
    REQUIRE(!groupPolicy.GetValue<ValuePolicy::SourceAutoUpdateIntervalInMinutes>().has_value());
    REQUIRE(!groupPolicy.GetValue<ValuePolicy::ProgressBarStyle>().has_value());
    REQUIRE(!groupPolicy.GetValue<ValuePolicy::IncludeSources>().has_value());

    // Policies controlling behavior should allow everything
    REQUIRE(groupPolicy.IsAllowed(TogglePolicy::None));
    REQUIRE(groupPolicy.IsAllowed(TogglePolicy::DisableWinGet));
    REQUIRE(groupPolicy.IsAllowed(TogglePolicy::DisableSettingsCommand));
    REQUIRE(groupPolicy.IsAllowed(TogglePolicy::DisableExperimentalFeatures));
    REQUIRE(groupPolicy.IsAllowed(TogglePolicy::DisableLocalManifestFiles));
    REQUIRE(groupPolicy.IsAllowed(TogglePolicy::ExcludeDefaultSources));
    REQUIRE(groupPolicy.IsAllowed(TogglePolicy::DisableSourceConfiguration));
}

TEST_CASE("GroupPolicy_UpdateInterval", "[groupPolicy]")
{
    auto policiesKey = RegCreateVolatileTestRoot();

    SECTION("Good value")
    {
        SetRegistryValue(policiesKey.get(), AutoUpdateIntervalValueName, 5);
        GroupPolicyTest groupPolicy{ policiesKey.get() };

        auto policy = groupPolicy.GetValue<ValuePolicy::SourceAutoUpdateIntervalInMinutes>();
        REQUIRE(policy.has_value());
        REQUIRE(*policy == 5);
    }

    SECTION("Wrong type")
    {
        SetRegistryValue(policiesKey.get(), AutoUpdateIntervalValueName, L"Wrong");
        GroupPolicyTest groupPolicy{ policiesKey.get() };

        auto policy = groupPolicy.GetValue<ValuePolicy::SourceAutoUpdateIntervalInMinutes>();
        REQUIRE(!policy.has_value());
    }
}

TEST_CASE("GroupPolicy_ProgressBar", "[groupPolicy]")
{
    auto policiesKey = RegCreateVolatileTestRoot();

    SECTION("Good value")
    {
        SetRegistryValue(policiesKey.get(), ProgressBarStyleValueName, L"rainbow");
        GroupPolicyTest groupPolicy{ policiesKey.get() };

        auto policy = groupPolicy.GetValue<ValuePolicy::ProgressBarStyle>();
        REQUIRE(policy.has_value());
        REQUIRE(*policy == "rainbow");
    }

    SECTION("Wrong type")
    {
        SetRegistryValue(policiesKey.get(), ProgressBarStyleValueName, 0);
        GroupPolicyTest groupPolicy{ policiesKey.get() };

        auto policy = groupPolicy.GetValue<ValuePolicy::ProgressBarStyle>();
        REQUIRE(!policy.has_value());
    }
}

// TODO: included sources

TEST_CASE("GroupPolicy_Toggle", "[groupPolicy]")
{
    auto policiesKey = RegCreateVolatileTestRoot();

    SECTION("'None' is enabled")
    {
        GroupPolicyTest groupPolicy{ policiesKey.get() };
        REQUIRE(groupPolicy.IsAllowed(TogglePolicy::None));
    }

    SECTION("Enabled")
    {
        SetRegistryValue(policiesKey.get(), DisableWinGetValueName, 0);
        GroupPolicyTest groupPolicy{ policiesKey.get() };
        REQUIRE(groupPolicy.IsAllowed(TogglePolicy::DisableWinGet));
    }

    SECTION("Disabled")
    {
        SetRegistryValue(policiesKey.get(), DisableLocalManifestFilesValueName, 1);
        GroupPolicyTest groupPolicy{ policiesKey.get() };
        REQUIRE(!groupPolicy.IsAllowed(TogglePolicy::DisableLocalManifestFiles));
    }

    SECTION("Wrong type")
    {
        SetRegistryValue(policiesKey.get(), ExcludeDefaultSourcesValueName, L"Wrong");
        GroupPolicyTest groupPolicy{ policiesKey.get() };
        REQUIRE(groupPolicy.IsAllowed(TogglePolicy::ExcludeDefaultSources));
    }
}
