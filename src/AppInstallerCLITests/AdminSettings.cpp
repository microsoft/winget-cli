// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSettings.h"
#include <winget/AdminSettings.h>
#include <AppInstallerRuntime.h>
#include <fstream>

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

TEST_CASE("AdminSetting_CorruptedVerificationFile", "[adminSettings]")
{
    GroupPolicyTestOverride policies;
    policies.SetState(TogglePolicy::Policy::LocalManifestFiles, PolicyState::NotConfigured);

    // Clean up any existing admin settings
    ResetAllAdminSettings();

    // Enable the setting to create both the data and verification files
    REQUIRE(EnableAdminSetting(BoolAdminSetting::LocalManifestFiles));
    REQUIRE(IsAdminSettingEnabled(BoolAdminSetting::LocalManifestFiles));

    // Get the path to the verification file.
    // Note: The data file may be stored in ApplicationData (packaged context) rather than the filesystem,
    // so we only verify the verification file, which is always on the filesystem.
    std::filesystem::path secureSettingsDir = AppInstaller::Runtime::GetPathTo(AppInstaller::Runtime::PathName::SecureSettingsForRead);
    std::filesystem::path verificationFilePath = secureSettingsDir / Stream::AdminSettings.Name;

    // Verify the verification file exists
    REQUIRE(std::filesystem::exists(verificationFilePath));

    // Corrupt the verification file by writing invalid content to it
    {
        std::ofstream corruptFile(verificationFilePath);
        corruptFile << "corrupted data that is not valid YAML or a valid hash";
    }

    // Now when we try to read the setting, it should fall back to the default value (false)
    // instead of throwing an exception
    REQUIRE_FALSE(IsAdminSettingEnabled(BoolAdminSetting::LocalManifestFiles));

    // Clean up
    ResetAllAdminSettings();
}