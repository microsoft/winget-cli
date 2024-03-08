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
