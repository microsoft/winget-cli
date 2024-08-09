// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSettings.h"
#include <winget/AdminSettings.h>

using namespace AppInstaller::Settings;
using namespace TestCommon;

TEST_CASE("AdminSetting_Enable", "[adminSettings]")
{
    WHEN("Group policy")
    {
        SECTION("Not configured")
        {
            GroupPolicyTestOverride policies;
            policies.SetState(TogglePolicy::Policy::LocalManifestFiles, PolicyState::NotConfigured);
            REQUIRE(EnableAdminSetting(BoolAdminSetting::LocalManifestFiles));
        }

        SECTION("Enabled")
        {
            GroupPolicyTestOverride policies;
            policies.SetState(TogglePolicy::Policy::LocalManifestFiles, PolicyState::Enabled);
            REQUIRE(EnableAdminSetting(BoolAdminSetting::LocalManifestFiles));
        }

        SECTION("Disabled")
        {
            GroupPolicyTestOverride policies;
            policies.SetState(TogglePolicy::Policy::LocalManifestFiles, PolicyState::Disabled);
            REQUIRE_FALSE(EnableAdminSetting(BoolAdminSetting::LocalManifestFiles));
        }
    }
}

TEST_CASE("AdminSetting_Disable", "[adminSettings]")
{
    WHEN("Group policy")
    {
        SECTION("Not configured")
        {
            GroupPolicyTestOverride policies;
            policies.SetState(TogglePolicy::Policy::LocalManifestFiles, PolicyState::NotConfigured);
            REQUIRE(DisableAdminSetting(BoolAdminSetting::LocalManifestFiles));
        }

        SECTION("Enabled")
        {
            GroupPolicyTestOverride policies;
            policies.SetState(TogglePolicy::Policy::LocalManifestFiles, PolicyState::Enabled);
            REQUIRE_FALSE(DisableAdminSetting(BoolAdminSetting::LocalManifestFiles));
        }

        SECTION("Disabled")
        {
            GroupPolicyTestOverride policies;
            policies.SetState(TogglePolicy::Policy::LocalManifestFiles, PolicyState::Disabled);
            REQUIRE(DisableAdminSetting(BoolAdminSetting::LocalManifestFiles));
        }
    }
}

TEST_CASE("AdminSetting_AllSettingsAreImplemented", "[adminSettings]")
{
    for (auto adminSetting : GetAllBoolAdminSettings())
    {
        // If we forget to add it to the conversion, it returns Unknown/None
        REQUIRE(AdminSettingToString(adminSetting) != AdminSettingToString(BoolAdminSetting::Unknown));
        REQUIRE(StringToBoolAdminSetting(AdminSettingToString(adminSetting)) != BoolAdminSetting::Unknown);
        REQUIRE(GetAdminSettingPolicy(adminSetting) != TogglePolicy::Policy::None);

        GroupPolicyTestOverride policies;
        policies.SetState(GetAdminSettingPolicy(adminSetting), PolicyState::NotConfigured);

        // We should be able to configure the state.
        // If we forget to add it, it won't persist.
        REQUIRE(EnableAdminSetting(adminSetting));
        REQUIRE(IsAdminSettingEnabled(adminSetting));
        REQUIRE(DisableAdminSetting(adminSetting));
        REQUIRE_FALSE(IsAdminSettingEnabled(adminSetting));
    }
}