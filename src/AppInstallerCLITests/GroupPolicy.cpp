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
    REQUIRE(groupPolicy.GetState(TogglePolicy::Policy::None) == PolicyState::NotConfigured);
    REQUIRE(groupPolicy.GetState(TogglePolicy::Policy::WinGet) == PolicyState::NotConfigured);
    REQUIRE(groupPolicy.GetState(TogglePolicy::Policy::Settings) == PolicyState::NotConfigured);
    REQUIRE(groupPolicy.GetState(TogglePolicy::Policy::ExperimentalFeatures) == PolicyState::NotConfigured);
    REQUIRE(groupPolicy.GetState(TogglePolicy::Policy::LocalManifestFiles) == PolicyState::NotConfigured);
    REQUIRE(groupPolicy.GetState(TogglePolicy::Policy::DefaultSource) == PolicyState::NotConfigured);
    REQUIRE(groupPolicy.GetState(TogglePolicy::Policy::MSStoreSource) == PolicyState::NotConfigured);
    REQUIRE(groupPolicy.GetState(TogglePolicy::Policy::AdditionalSources) == PolicyState::NotConfigured);
    REQUIRE(groupPolicy.GetState(TogglePolicy::Policy::AllowedSources) == PolicyState::NotConfigured);
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
        REQUIRE(groupPolicy.GetState(TogglePolicy::Policy::None) == PolicyState::NotConfigured);
        REQUIRE(groupPolicy.IsEnabled(TogglePolicy::Policy::None));
    }

    SECTION("Enabled")
    {
        SetRegistryValue(policiesKey.get(), WinGetPolicyValueName, 1);
        GroupPolicy groupPolicy{ policiesKey.get() };
        REQUIRE(groupPolicy.GetState(TogglePolicy::Policy::WinGet) == PolicyState::Enabled);
        REQUIRE(groupPolicy.IsEnabled(TogglePolicy::Policy::WinGet));
    }

    SECTION("Disabled")
    {
        SetRegistryValue(policiesKey.get(), LocalManifestsPolicyValueName, 0);
        GroupPolicy groupPolicy{ policiesKey.get() };
        REQUIRE(groupPolicy.GetState(TogglePolicy::Policy::LocalManifestFiles) == PolicyState::Disabled);
        REQUIRE_FALSE(groupPolicy.IsEnabled(TogglePolicy::Policy::LocalManifestFiles));
    }

    SECTION("Wrong type")
    {
        SetRegistryValue(policiesKey.get(), ExperimentalFeaturesPolicyValueName, L"Wrong");
        GroupPolicy groupPolicy{ policiesKey.get() };
        REQUIRE(groupPolicy.GetState(TogglePolicy::Policy::DefaultSource) == PolicyState::NotConfigured);
        REQUIRE(groupPolicy.IsEnabled(TogglePolicy::Policy::DefaultSource));
    }
}