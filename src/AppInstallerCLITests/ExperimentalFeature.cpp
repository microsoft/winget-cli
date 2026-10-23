// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSettings.h"
#include <winget/ExperimentalFeature.h>
#include <winget/Settings.h>

#include <AppInstallerErrors.h>
#include <algorithm>
#include <utility>

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

// Registering a feature takes four separate edits in three files, and nothing connects them. A
// flag added to the enum without a GetFeature case makes `winget features` throw rather than
// merely omit the feature, and one without a setting throws from IsEnabled. Both are caught here
// for every feature at once, so the cost of the split registration is paid by this test rather
// than by whoever adds the next one.
TEST_CASE("ExperimentalFeature AllFeaturesAreRegistered", "[experimentalFeature]")
{
    auto again = DeleteUserSettingsFiles();

    std::set<std::string_view> jsonNames;

    for (const auto& feature : ExperimentalFeature::GetAllFeatures())
    {
        INFO(feature.Name());

        REQUIRE(!feature.Name().empty());
        REQUIRE(!feature.JsonName().empty());

        // The json name is what a user types, so two features cannot share one.
        REQUIRE(jsonNames.insert(feature.JsonName()).second);

        // An experimental feature is off until it is asked for. The empty settings are written
        // explicitly so that this does not depend on what a previous iteration left behind.
        {
            SetSetting(Stream::PrimaryUserSettings, "{}");
            UserSettingsTest userSettingTest;

            REQUIRE_FALSE(ExperimentalFeature::IsEnabled(feature.GetFeature(), userSettingTest));
        }

        // The name that `winget features` prints has to be the one that actually enables the
        // feature, which is the only thing tying the enum value to its setting.
        {
            std::string json = R"({ "experimentalFeatures": { ")" + std::string{ feature.JsonName() } + R"(": true } })";
            SetSetting(Stream::PrimaryUserSettings, json);
            UserSettingsTest userSettingTest;

            REQUIRE(ExperimentalFeature::IsEnabled(feature.GetFeature(), userSettingTest));
        }
    }
}

TEST_CASE("ExperimentalFeature ExperimentalCmd", "[experimentalFeature]")
{
    auto again = DeleteUserSettingsFiles();

    SECTION("Feature off default")
    {
        UserSettingsTest userSettingTest;

        REQUIRE_FALSE(ExperimentalFeature::IsEnabled(ExperimentalFeature::Feature::ExperimentalCmd, userSettingTest));
    }
    SECTION("Feature on")
    {
        std::string_view json = R"({ "experimentalFeatures": { "experimentalCmd": true } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE(ExperimentalFeature::IsEnabled(ExperimentalFeature::Feature::ExperimentalCmd, userSettingTest));
    }
    SECTION("Feature off")
    {
        std::string_view json = R"({ "experimentalFeatures": { "experimentalCmd": false } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE_FALSE(ExperimentalFeature::IsEnabled(ExperimentalFeature::Feature::ExperimentalCmd, userSettingTest));
    }
    SECTION("Invalid value")
    {
        std::string_view json = R"({ "experimentalFeatures": { "experimentalCmd": "string" } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE_FALSE(ExperimentalFeature::IsEnabled(ExperimentalFeature::Feature::ExperimentalCmd, userSettingTest));
    }
    SECTION("Disabled by group policy")
    {
        auto policiesKey = RegCreateVolatileTestRoot();
        SetRegistryValue(policiesKey.get(), ExperimentalFeaturesPolicyValueName, false);
        GroupPolicyTestOverride policies{ policiesKey.get() };

        std::string_view json = R"({ "experimentalFeatures": { "experimentalCmd": true } })";
        SetSetting(Stream::PrimaryUserSettings, json);
        UserSettingsTest userSettingTest;

        REQUIRE_FALSE(ExperimentalFeature::IsEnabled(ExperimentalFeature::Feature::ExperimentalCmd, userSettingTest));
    }
}

TEST_CASE("ExperimentalFeature InteractivePackageSelection", "[experimentalFeature]")
{
    auto again = DeleteUserSettingsFiles();
    auto [json, enabled] = GENERATE(
        std::make_pair(std::string_view{ "{}" }, false),
        std::make_pair(std::string_view{ R"({ "experimentalFeatures": { "interactivePackageSelection": true } })" }, true),
        std::make_pair(std::string_view{ R"({ "experimentalFeatures": { "interactivePackageSelection": false } })" }, false),
        std::make_pair(std::string_view{ R"({ "experimentalFeatures": { "interactivePackageSelection": "string" } })" }, false));
    bool policyEnabled = GENERATE(false, true);
    auto policiesKey = RegCreateVolatileTestRoot();
    SetRegistryValue(policiesKey.get(), ExperimentalFeaturesPolicyValueName, policyEnabled);
    GroupPolicyTestOverride policies{ policiesKey.get() };
    SetSetting(Stream::PrimaryUserSettings, json);
    UserSettingsTest userSettingTest;

    CAPTURE(json, policyEnabled);
    REQUIRE(userSettingTest.Get<Setting::EFInteractivePackageSelection>() == enabled);
    REQUIRE(ExperimentalFeature::IsEnabled(ExperimentalFeature::Feature::InteractivePackageSelection, userSettingTest) ==
        (enabled && policyEnabled));

    auto feature = ExperimentalFeature::GetFeature(ExperimentalFeature::Feature::InteractivePackageSelection);
    std::string_view jsonName = feature.JsonName();
    REQUIRE(jsonName == "interactivePackageSelection");
    auto features = ExperimentalFeature::GetAllFeatures();
    REQUIRE(std::any_of(features.begin(), features.end(), [](const auto& item)
    {
        return item.GetFeature() == ExperimentalFeature::Feature::InteractivePackageSelection;
    }));
}
