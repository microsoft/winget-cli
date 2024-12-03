
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

#define SET_USER_SETTINGS(_enabled_, _disabled_) \
    TestUserSettings settings; \
    settings.Set<Setting::Experiments>({ \
        {"TestExperimentEnabledByDefault", _enabled_}, \
        {"TestExperimentDisabledByDefault", _disabled_} \
    });

#define ASSERT_EXPERIMENTS(_enabled_, _disabled_) \
    REQUIRE(_enabled_ == Experiment::IsEnabled(Experiment::Key::TestExperimentEnabledByDefault)); \
    REQUIRE(_disabled_ == Experiment::IsEnabled(Experiment::Key::TestExperimentDisabledByDefault));

TEST_CASE("Experiment_GroupPolicyControl", "[experiment]")
{
    SECTION("Not configured")
    {
        SET_POLICY_STATE(TogglePolicy::Policy::Experiments, PolicyState::NotConfigured);
        ASSERT_EXPERIMENTS(true, false);
    }

    SECTION("Enabled")
    {
        SET_POLICY_STATE(TogglePolicy::Policy::Experiments, PolicyState::Enabled);
        ASSERT_EXPERIMENTS(true, false);
    }

    SECTION("Disabled")
    {
        SET_POLICY_STATE(TogglePolicy::Policy::Experiments, PolicyState::Disabled);
        ASSERT_EXPERIMENTS(false, false);
    }
}

TEST_CASE("Experiment_GroupPolicyDisabled_ReturnFalse", "[experiment]")
{
    // If the policy is disabled, then also the user settings should be ignored.
    SET_POLICY_STATE(TogglePolicy::Policy::Experiments, PolicyState::Disabled);
    SET_USER_SETTINGS(true, true);
    ASSERT_EXPERIMENTS(false, false);
}

TEST_CASE("Experiment_UserSettingsControl", "[experiment]")
{
    SECTION("Experiments not configured in user settings")
    {
        // Default values are used
        ASSERT_EXPERIMENTS(true, false);
    }

    SECTION("Experiments enabled in user settings")
    {
        SET_USER_SETTINGS(true, true);
        ASSERT_EXPERIMENTS(true, true);
    }

    SECTION("Experiments disabled in user settings")
    {
        SET_USER_SETTINGS(false, false);
        ASSERT_EXPERIMENTS(false, false);
    }
}
