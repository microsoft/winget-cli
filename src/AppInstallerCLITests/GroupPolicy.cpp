// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSettings.h"
#include "winget/GroupPolicy.h"

using namespace TestCommon;
using namespace AppInstaller::Settings;
using namespace std::string_view_literals;

TEST_CASE("GroupPolicy_NoPolicies", "[groupPolicy]")
{
    auto policiesKey = RegCreateVolatileTestRoot();
    GroupPolicy groupPolicy{ policiesKey.get() };

    // Policies setting a value should be empty
    REQUIRE(!groupPolicy.GetValue<ValuePolicy::SourceAutoUpdateIntervalInMinutes>().has_value());
    REQUIRE(!groupPolicy.GetValue<ValuePolicy::AdditionalSources>().has_value());
    REQUIRE(!groupPolicy.GetValue<ValuePolicy::AllowedSources>().has_value());

    // Everything should be not configured
    REQUIRE(groupPolicy.GetState(TogglePolicy::None) == PolicyState::NotConfigured);
    REQUIRE(groupPolicy.GetState(TogglePolicy::WinGet) == PolicyState::NotConfigured);
    REQUIRE(groupPolicy.GetState(TogglePolicy::SettingsCommand) == PolicyState::NotConfigured);
    REQUIRE(groupPolicy.GetState(TogglePolicy::ExperimentalFeatures) == PolicyState::NotConfigured);
    REQUIRE(groupPolicy.GetState(TogglePolicy::LocalManifestFiles) == PolicyState::NotConfigured);
    REQUIRE(groupPolicy.GetState(TogglePolicy::DefaultSource) == PolicyState::NotConfigured);
    REQUIRE(groupPolicy.GetState(TogglePolicy::MSStoreSource) == PolicyState::NotConfigured);
    REQUIRE(groupPolicy.GetState(TogglePolicy::AdditionalSources) == PolicyState::NotConfigured);
    REQUIRE(groupPolicy.GetState(TogglePolicy::AllowedSources) == PolicyState::NotConfigured);
}

TEST_CASE("GroupPolicy_UpdateInterval", "[groupPolicy]")
{
    auto policiesKey = RegCreateVolatileTestRoot();

    SECTION("Good value")
    {
        SetRegistryValue(policiesKey.get(), SourceUpdateIntervalPolicyValueName, 5);
        GroupPolicy groupPolicy{ policiesKey.get() };

        auto policy = groupPolicy.GetValue<ValuePolicy::SourceAutoUpdateIntervalInMinutes>();
        REQUIRE(policy.has_value());
        REQUIRE(*policy == 5);
    }

    SECTION("Wrong type")
    {
        SetRegistryValue(policiesKey.get(), SourceUpdateIntervalPolicyValueName, L"Wrong");
        GroupPolicy groupPolicy{ policiesKey.get() };

        auto policy = groupPolicy.GetValue<ValuePolicy::SourceAutoUpdateIntervalInMinutes>();
        REQUIRE(!policy.has_value());
    }
}

// TODO: additional/allowed sources

TEST_CASE("GroupPolicy_Toggle", "[groupPolicy]")
{
    auto policiesKey = RegCreateVolatileTestRoot();

    SECTION("'None' is not configured")
    {
        GroupPolicy groupPolicy{ policiesKey.get() };
        REQUIRE(groupPolicy.GetState(TogglePolicy::None) == PolicyState::NotConfigured);
        REQUIRE(groupPolicy.IsEnabled(TogglePolicy::None));
    }

    SECTION("Enabled")
    {
        SetRegistryValue(policiesKey.get(), WinGetPolicyValueName, 1);
        GroupPolicy groupPolicy{ policiesKey.get() };
        REQUIRE(groupPolicy.GetState(TogglePolicy::WinGet) == PolicyState::Enabled);
        REQUIRE(groupPolicy.IsEnabled(TogglePolicy::WinGet));
    }

    SECTION("Disabled")
    {
        SetRegistryValue(policiesKey.get(), LocalManifestsPolicyValueName, 0);
        GroupPolicy groupPolicy{ policiesKey.get() };
        REQUIRE(groupPolicy.GetState(TogglePolicy::LocalManifestFiles) == PolicyState::Disabled);
        REQUIRE_FALSE(groupPolicy.IsEnabled(TogglePolicy::LocalManifestFiles));
    }

    SECTION("Wrong type")
    {
        SetRegistryValue(policiesKey.get(), ExperimentalFeaturesPolicyValueName, L"Wrong");
        GroupPolicy groupPolicy{ policiesKey.get() };
        REQUIRE(groupPolicy.GetState(TogglePolicy::DefaultSource) == PolicyState::NotConfigured);
        REQUIRE(groupPolicy.IsEnabled(TogglePolicy::DefaultSource));
    }
}