// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSettings.h"
#include <winget/Experiment.h>
#include <AppInstallerStrings.h>

using namespace TestCommon;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Experiment;

#define ASSERT_EXPERIMENT(_isEnabled_, _toggleSource_) \
    auto testExperimentState = Experiment::GetState(ExperimentKey::TestExperiment); \
    REQUIRE(_isEnabled_ == testExperimentState.IsEnabled()); \
    REQUIRE(_toggleSource_ == testExperimentState.ToggleSource());

const std::string s_TestExperimentName = "TestExperiment";

TEST_CASE("Experiment_GroupPolicyControl", "[experiment]")
{
    SECTION("Not configured")
    {
        ExperimentationTest experimentation;
        GroupPolicyTestOverride policies;
        policies.SetState(TogglePolicy::Policy::Experimentation, PolicyState::NotConfigured);
        ASSERT_EXPERIMENT(true, ExperimentToggleSource::Default);
    }

    SECTION("Enabled")
    {
        ExperimentationTest experimentation;
        GroupPolicyTestOverride policies;
        policies.SetState(TogglePolicy::Policy::Experimentation, PolicyState::Enabled);
        ASSERT_EXPERIMENT(true, ExperimentToggleSource::Default);
    }

    SECTION("Disabled")
    {
        ExperimentationTest experimentation;
        GroupPolicyTestOverride policies;
        policies.SetState(TogglePolicy::Policy::Experimentation, PolicyState::Disabled);
        ASSERT_EXPERIMENT(false, ExperimentToggleSource::Policy);
    }
}

TEST_CASE("Experiment_GroupPolicyDisabled_ReturnFalse", "[experiment]")
{
    ExperimentationTest experimentation;
    TestUserSettings settings;
    settings.Set<Setting::Experimentation>({{s_TestExperimentName, true}});

    // If the policy is disabled, then also the user settings should be ignored.
    GroupPolicyTestOverride policies;
    policies.SetState(TogglePolicy::Policy::Experimentation, PolicyState::Disabled);
    ASSERT_EXPERIMENT(false, ExperimentToggleSource::Policy);
}

TEST_CASE("Experiment_GroupPolicyEnabled", "[experiment]")
{
    SECTION("Global experimentation disabled in user settings")
    {
        GroupPolicyTestOverride policies;
        policies.SetState(TogglePolicy::Policy::Experimentation, PolicyState::Enabled);
        ExperimentationTest experimentation;
        TestUserSettings settings;
        settings.Set<Setting::AllowExperimentation>(false);
        settings.Set<Setting::Experimentation>({{s_TestExperimentName, true}});
        ASSERT_EXPERIMENT(false, ExperimentToggleSource::UserSettingGlobalControl);
    }

    SECTION("Individual experiment disabled in user settings")
    {
        GroupPolicyTestOverride policies;
        policies.SetState(TogglePolicy::Policy::Experimentation, PolicyState::Enabled);
        ExperimentationTest experimentation;
        TestUserSettings settings;
        settings.Set<Setting::AllowExperimentation>(true);
        settings.Set<Setting::Experimentation>({{s_TestExperimentName, false}});
        ASSERT_EXPERIMENT(false, ExperimentToggleSource::UserSettingIndividualControl);
    }
}

TEST_CASE("Experiment_UserSettingsIndividualControl", "[experiment]")
{
    SECTION("Individual experiment not configured in user settings")
    {
        // Default value are used
        ExperimentationTest experimentation;
        ASSERT_EXPERIMENT(true, ExperimentToggleSource::Default);
    }

    SECTION("Individual experiment enabled in user settings")
    {
        ExperimentationTest experimentation;
        TestUserSettings settings;
        settings.Set<Setting::Experimentation>({{s_TestExperimentName, true}});
        ASSERT_EXPERIMENT(true, ExperimentToggleSource::UserSettingIndividualControl);
    }

    SECTION("Individual experiment disabled in user settings")
    {
        ExperimentationTest experimentation;
        TestUserSettings settings;
        settings.Set<Setting::Experimentation>({{s_TestExperimentName, false}});
        ASSERT_EXPERIMENT(false, ExperimentToggleSource::UserSettingIndividualControl);
    }
}

TEST_CASE("Experiment_UserSettingsGlobalControl", "[experiment]")
{
    SECTION("Global experimentation not configured in user settings")
    {
        ExperimentationTest experimentation;
        TestUserSettings settings;
        settings.Set<Setting::Experimentation>({{s_TestExperimentName, true}});
        ASSERT_EXPERIMENT(true, ExperimentToggleSource::UserSettingIndividualControl);
    }

    SECTION("Global experimentation enabled in user settings")
    {
        ExperimentationTest experimentation;
        TestUserSettings settings;
        settings.Set<Setting::AllowExperimentation>(true);
        settings.Set<Setting::Experimentation>({{s_TestExperimentName, true}});
        ASSERT_EXPERIMENT(true, ExperimentToggleSource::UserSettingIndividualControl);
    }

    SECTION("Global experimentation disabled in user settings")
    {
        ExperimentationTest experimentation;
        TestUserSettings settings;
        settings.Set<Setting::AllowExperimentation>(false);
        settings.Set<Setting::Experimentation>({{s_TestExperimentName, true}});
        ASSERT_EXPERIMENT(false, ExperimentToggleSource::UserSettingGlobalControl);
    }
}
