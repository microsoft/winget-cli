// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>
#include <type_traits>
#include "Experiment/Experiment.h"
#include "AppInstallerStrings.h"

namespace AppInstaller::Settings
{
    enum ExperimentToggleSource
    {
        Default = 0,
        Policy,
        UserSettingIndividualControl,
        UserSettingGlobalControl,
    };

    struct ExperimentState
    {
        ExperimentState() = default;
        ExperimentState(bool isEnabled, ExperimentToggleSource toggleSource) : m_isEnabled(isEnabled), m_toggleSource(toggleSource) {}

        // Gets a value indicating whether the experiment is enabled.
        // Note: This API expects an experiment to be disabled by default and will
        // always return false if the user opts out of the experiment from the
        // user settings or group policy.
        bool IsEnabled() const { return m_isEnabled; }
        ExperimentToggleSource ToggleSource() const { return m_toggleSource; }
        std::string ToJson() const;
    private:
        ExperimentToggleSource m_toggleSource;
        bool m_isEnabled;
    };

    struct Experiment
    {
        using Key_t = std::underlying_type_t<AppInstaller::Experiment::ExperimentKey>;

        Experiment(std::string name, std::string jsonName, std::string link, AppInstaller::Experiment::ExperimentKey key) :
            m_name(std::move(name)), m_jsonName(jsonName), m_link(std::move(link)), m_key(std::move((key))) {}

        static ExperimentState GetState(AppInstaller::Experiment::ExperimentKey feature);
        static ExperimentState GetStateInternal(AppInstaller::Experiment::ExperimentKey feature);
        static Experiment GetExperiment(AppInstaller::Experiment::ExperimentKey key);
        static std::vector<Experiment> GetAllExperiments();

        std::string Name() const { return m_name; }
        std::string JsonName() const { return m_jsonName; }
        std::string Link() const { return m_link; }
        AppInstaller::Experiment::ExperimentKey GetKey() const { return m_key; }

    private:
        std::string m_name;
        std::string m_jsonName;
        std::string m_link;
        AppInstaller::Experiment::ExperimentKey m_key;
    };
}
