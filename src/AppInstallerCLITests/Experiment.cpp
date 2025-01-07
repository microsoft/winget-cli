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

TEST_CASE("Experiment_GroupPolicyControl", "[experiment]")
{
    SECTION("Not configured")
    {
        ExperimentsTest experiments;
        GroupPolicyTestOverride policies;
        policies.SetState(TogglePolicy::Policy::Experiments, PolicyState::NotConfigured);
        ASSERT_EXPERIMENT(true, ExperimentToggleSource::Default);
    }

    SECTION("Enabled")
    {
        ExperimentsTest experiments;
        GroupPolicyTestOverride policies;
        policies.SetState(TogglePolicy::Policy::Experiments, PolicyState::Enabled);
        ASSERT_EXPERIMENT(true, ExperimentToggleSource::Default);
    }

    SECTION("Disabled")
    {
        ExperimentsTest experiments;
        GroupPolicyTestOverride policies;
        policies.SetState(TogglePolicy::Policy::Experiments, PolicyState::Disabled);
        ASSERT_EXPERIMENT(false, ExperimentToggleSource::Policy);
    }
}

TEST_CASE("Experiment_GroupPolicyDisabled_ReturnFalse", "[experiment]")
{
    ExperimentsTest experiments;
    TestUserSettings settings;
    settings.Set<Setting::Experiments>({{"TestExperiment", true}});

    // If the policy is disabled, then also the user settings should be ignored.
    GroupPolicyTestOverride policies;
    policies.SetState(TogglePolicy::Policy::Experiments, PolicyState::Disabled);
    ASSERT_EXPERIMENT(false, ExperimentToggleSource::Policy);
}

TEST_CASE("Experiment_UserSettingsIndividualControl", "[experiment]")
{
    SECTION("Experiments not configured in user settings")
    {
        // Default value are used
        ExperimentsTest experiments;
        ASSERT_EXPERIMENT(true, ExperimentToggleSource::Default);
    }

    SECTION("Experiments enabled in user settings")
    {
        ExperimentsTest experiments;
        TestUserSettings settings;
        settings.Set<Setting::Experiments>({{"TestExperiment", true}});
        ASSERT_EXPERIMENT(true, ExperimentToggleSource::UserSettingIndividualControl);
    }

    SECTION("Experiments disabled in user settings")
    {
        ExperimentsTest experiments;
        TestUserSettings settings;
        settings.Set<Setting::Experiments>({{"TestExperiment", false}});
        ASSERT_EXPERIMENT(false, ExperimentToggleSource::UserSettingIndividualControl);
    }
}

TEST_CASE("Experiment_UserSettingsGlobalControl", "[experiment]")
{
    SECTION("'Allow experiments' not configured in user settings")
    {
        ExperimentsTest experiments;
        TestUserSettings settings;
        settings.Set<Setting::Experiments>({{"TestExperiment", true}});
        ASSERT_EXPERIMENT(true, ExperimentToggleSource::UserSettingIndividualControl);
    }

    SECTION("'Allow experiments' enabled in user settings")
    {
        ExperimentsTest experiments;
        TestUserSettings settings;
        settings.Set<Setting::AllowExperiments>(true);
        settings.Set<Setting::Experiments>({{"TestExperiment", true}});
        ASSERT_EXPERIMENT(true, ExperimentToggleSource::UserSettingIndividualControl);
    }

    SECTION("'Allow experiments' disabled in user settings")
    {
        ExperimentsTest experiments;
        TestUserSettings settings;
        settings.Set<Setting::AllowExperiments>(false);
        settings.Set<Setting::Experiments>({{"TestExperiment", true}});
        ASSERT_EXPERIMENT(false, ExperimentToggleSource::UserSettingGlobalControl);
    }
}
