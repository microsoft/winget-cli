// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSettings.h"
#include <winget/ExperimentalFeature.h>
#include <winget/Settings.h>

#include <AppInstallerErrors.h>

using namespace AppInstaller::Settings;
using namespace TestCommon;

TEST_CASE("ExperimentalFeature None", "[experimentalFeature]")
{
    // Make sure Feature::None is always enabled.
    REQUIRE(ExperimentalFeature::IsEnabled(ExperimentalFeature::Feature::None));

    // Make sure to throw requesting Feature::None
    REQUIRE_THROWS_HR(ExperimentalFeature::GetFeature(ExperimentalFeature::Feature::None), E_UNEXPECTED);

    // Make sure Feature::None is not disabled by Group Policy
    auto policiesKey = RegCreateVolatileTestRoot();
    SetRegistryValue(policiesKey.get(), ExperimentalFeaturesPolicyValueName, false);
    GroupPolicyTestOverride policies{ policiesKey.get() };
    REQUIRE(ExperimentalFeature::IsEnabled(ExperimentalFeature::Feature::None));
}

TEST_CASE("ExperimentalFeature ExperimentalCmd", "[experimentalFeature]")
{
    DeleteUserSettingsFiles();

    SECTION("Feature off default")
    {
        UserSettingsTest userSettingTest;

        REQUIRE_FALSE(ExperimentalFeature::IsEnabled(ExperimentalFeature::Feature::ExperimentalCmd, userSettingTest));
    }
    SECTION("Feature on")
    {
        std::string_view json = R"({ "experimentalFeatures": { "experimentalCmd": true } })";
        SetSetting(Streams::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(ExperimentalFeature::IsEnabled(ExperimentalFeature::Feature::ExperimentalCmd, userSettingTest));
    }
    SECTION("Feature off")
    {
        std::string_view json = R"({ "experimentalFeatures": { "experimentalCmd": false } })";
        SetSetting(Streams::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE_FALSE(ExperimentalFeature::IsEnabled(ExperimentalFeature::Feature::ExperimentalCmd, userSettingTest));
    }
    SECTION("Invalid value")
    {
        std::string_view json = R"({ "experimentalFeatures": { "experimentalCmd": "string" } })";
        SetSetting(Streams::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE_FALSE(ExperimentalFeature::IsEnabled(ExperimentalFeature::Feature::ExperimentalCmd, userSettingTest));
    }
    SECTION("Disabled by group policy")
    {
        auto policiesKey = RegCreateVolatileTestRoot();
        SetRegistryValue(policiesKey.get(), ExperimentalFeaturesPolicyValueName, false);
        GroupPolicyTestOverride policies{ policiesKey.get() };

        std::string_view json = R"({ "experimentalFeatures": { "experimentalCmd": true } })";
        SetSetting(Streams::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE_FALSE(ExperimentalFeature::IsEnabled(ExperimentalFeature::Feature::ExperimentalCmd, userSettingTest));
    }
}