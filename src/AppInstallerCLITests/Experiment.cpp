
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSettings.h"
#include <winget/Experiment.h>
#include <AppInstallerStrings.h>

using namespace TestCommon;
using namespace AppInstaller::Settings;

#define SET_POLICY_STATE(_policy_, _state_) \
    GroupPolicyTestOverride policies; \
    policies.SetState(_policy_, _state_);

#define SET_USER_SETTINGS(_value_) \
    TestUserSettings settings; \
    settings.Set<Setting::Experiments>({ \
        {"TestExperiment", _value_} \
    });

#define ASSERT_EXPERIMENT(_isEnabled_, _toggleSource_) \
    auto testExperimentState = Experiment::GetState(Experiment::Key::TestExperiment); \
    REQUIRE(_isEnabled_ == testExperimentState.IsEnabled()); \
    REQUIRE(_toggleSource_ == testExperimentState.ToggleSource());

TEST_CASE("Experiment_GroupPolicyControl", "[experiment]")
{
    SECTION("Not configured")
    {
        SET_POLICY_STATE(TogglePolicy::Policy::Experiments, PolicyState::NotConfigured);
        ASSERT_EXPERIMENT(true, ExperimentToggleSource::Default);
    }

    SECTION("Enabled")
    {
        SET_POLICY_STATE(TogglePolicy::Policy::Experiments, PolicyState::Enabled);
        ASSERT_EXPERIMENT(true, ExperimentToggleSource::Default);
    }

    SECTION("Disabled")
    {
        SET_POLICY_STATE(TogglePolicy::Policy::Experiments, PolicyState::Disabled);
        ASSERT_EXPERIMENT(false, ExperimentToggleSource::Policy);
    }
}

TEST_CASE("Experiment_GroupPolicyDisabled_ReturnFalse", "[experiment]")
{
    // If the policy is disabled, then also the user settings should be ignored.
    SET_POLICY_STATE(TogglePolicy::Policy::Experiments, PolicyState::Disabled);
    SET_USER_SETTINGS(true);
    ASSERT_EXPERIMENT(false, ExperimentToggleSource::Policy);
}

TEST_CASE("Experiment_UserSettingsControl", "[experiment]")
{
    SECTION("Experiments not configured in user settings")
    {
        // Default value are used
        ASSERT_EXPERIMENT(true, ExperimentToggleSource::Default);
    }

    SECTION("Experiments enabled in user settings")
    {
        SET_USER_SETTINGS(true);
        ASSERT_EXPERIMENT(true, ExperimentToggleSource::UserSetting);
    }

    SECTION("Experiments disabled in user settings")
    {
        SET_USER_SETTINGS(false);
        ASSERT_EXPERIMENT(false, ExperimentToggleSource::UserSetting);
    }
}
