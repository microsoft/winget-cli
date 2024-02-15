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
            REQUIRE(EnableAdminSetting(AdminSetting::LocalManifestFiles));
        }

        SECTION("Enabled")
        {
            GroupPolicyTestOverride policies;
            policies.SetState(TogglePolicy::Policy::LocalManifestFiles, PolicyState::Enabled);
            REQUIRE(EnableAdminSetting(AdminSetting::LocalManifestFiles));
        }

        SECTION("Disabled")
        {
            GroupPolicyTestOverride policies;
            policies.SetState(TogglePolicy::Policy::LocalManifestFiles, PolicyState::Disabled);
            REQUIRE_FALSE(EnableAdminSetting(AdminSetting::LocalManifestFiles));
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
            REQUIRE(DisableAdminSetting(AdminSetting::LocalManifestFiles));
        }

        SECTION("Enabled")
        {
            GroupPolicyTestOverride policies;
            policies.SetState(TogglePolicy::Policy::LocalManifestFiles, PolicyState::Enabled);
            REQUIRE_FALSE(DisableAdminSetting(AdminSetting::LocalManifestFiles));
        }

        SECTION("Disabled")
        {
            GroupPolicyTestOverride policies;
            policies.SetState(TogglePolicy::Policy::LocalManifestFiles, PolicyState::Disabled);
            REQUIRE(DisableAdminSetting(AdminSetting::LocalManifestFiles));
        }
    }
}

TEST_CASE("AdminSetting_AllSettingsAreImplemented", "[adminSettings]")
{
    using AdminSetting_t = std::underlying_type_t<AdminSetting>;

    // Skip Unknown.
    for (AdminSetting_t i = 1 + static_cast<AdminSetting_t>(AdminSetting::Unknown); i < static_cast<AdminSetting_t>(AdminSetting::Max); ++i)
    {
        auto adminSetting = static_cast<AdminSetting>(i);

        // If we forget to add it to the conversion, it returns Unknown/None
        REQUIRE(AdminSettingToString(adminSetting) != AdminSettingToString(AdminSetting::Unknown));
        REQUIRE(StringToAdminSetting(AdminSettingToString(adminSetting)) != AdminSetting::Unknown);
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