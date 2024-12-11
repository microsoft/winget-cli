
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Experiment.h"
#include "winget/UserSettings.h"
#include "Experiment/Experiment.h"
#include "AppInstallerTelemetry.h"

namespace AppInstaller::Settings
{
    namespace
    {
        ExperimentState GetExperimentStateInternal(Experiment::Key key, const UserSettings& userSettings)
        {
            if (key == Experiment::Key::None)
            {
                return { true, ExperimentToggleSource::Default };
            }

            auto experiment = Experiment::GetExperiment(key);
            if (!GroupPolicies().IsEnabled(TogglePolicy::Policy::Experiments))
            {
                AICLI_LOG(Core, Info, << "Experiment " << experiment.Name() <<
                    " is disabled due to group policy: " << TogglePolicy::GetPolicy(TogglePolicy::Policy::Experiments).RegValueName());
                return { false, ExperimentToggleSource::Policy };
            }

            auto userSettingsExperiments = userSettings.Get<Setting::Experiments>();
            auto jsonName = std::string(experiment.JsonName());
            if (userSettingsExperiments.find(jsonName) != userSettingsExperiments.end())
            {
                auto isEnabled = userSettingsExperiments[jsonName];
                AICLI_LOG(Core, Info, << "Experiment " << experiment.Name() << " is set to " << (isEnabled ? "true" : "false") << " in user settings");
                return { isEnabled, ExperimentToggleSource::UserSetting };
            }

            auto isEnabled = AppInstaller::Experiment::IsEnabled(experiment.GetKey());
            AICLI_LOG(Core, Info, << "Experiment " << experiment.Name() << " is set to " << (isEnabled ? "true" : "false"));
            return { isEnabled, ExperimentToggleSource::Default };
        }

        std::string ExperimentToggleSourceToString(ExperimentToggleSource source)
        {
            switch (source)
            {
            case ExperimentToggleSource::Default:
                return "Default";
            case ExperimentToggleSource::Policy:
                return "Policy";
            case ExperimentToggleSource::UserSetting:
                return "UserSetting";
            default:
                return "Unknown";
            }
        }
    }

    std::string ExperimentState::ToJson() const
    {
        Json::Value root;
        root["IsEnabled"] = m_isEnabled;
        root["ToggleSource"] = ExperimentToggleSourceToString(m_toggleSource);
        Json::StreamWriterBuilder builder;
        return Json::writeString(builder, root);
    }

    ExperimentState Experiment::GetStateInternal(Key key)
    {
        return GetExperimentStateInternal(key, User());
    }

    ExperimentState Experiment::GetState(Key key)
    {
        return Logging::Telemetry().GetExperimentState(key);
    }

    Experiment Experiment::GetExperiment(Key key)
    {
        switch (key)
        {
        case Key::CDN:
            return Experiment{ "CDN experiment", "CDN", "https://aka.ms/winget-settings", "CDN"};
#ifndef AICLI_DISABLE_TEST_HOOKS
        case Key::TestExperiment:
            return Experiment{ "Test experiment", "TestExperiment", "https://aka.ms/winget-settings", "TestExperiment" };
#endif
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    std::vector<Experiment> Experiment::GetAllExperiments()
    {
        std::vector<Experiment> result;

        for (Key_t i = 0x1; i < static_cast<Key_t>(Key::Max); ++i)
        {
            result.emplace_back(GetExperiment(static_cast<Key>(i)));
        }

        return result;
    }
}
