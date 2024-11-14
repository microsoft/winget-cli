
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Experiment.h"
#include "winget/UserSettings.h"
#include "Experiment/Experiment.h"

namespace AppInstaller::Settings
{
    namespace
    {
        bool IsEnabledInternal(Experiment::Key key, const UserSettings& userSettings)
        {
            if (key == Experiment::Key::None)
            {
                return true;
            }

            if (!GroupPolicies().IsEnabled(TogglePolicy::Policy::Experiments))
            {
                AICLI_LOG(Core, Info, <<
                    "Experiments '" << Experiment::GetExperiment(key).Name() <<
                    "' is disabled due to group policy: " << TogglePolicy::GetPolicy(TogglePolicy::Policy::Experiments).RegValueName());
                return false;
            }

            auto experiments = userSettings.Get<Setting::Experiments>();
            auto experiment = Experiment::GetExperiment(key);
            auto jsonName = std::string(experiment.JsonName());
            if (experiments.find(jsonName) != experiments.end())
            {
                return experiments[jsonName];
            }

            return AppInstaller::Experiment::IsEnabled(experiment.GetKey());
        }
    }

    bool Experiment::IsEnabled(Key key)
    {
        return IsEnabledInternal(key, User());
    }

    Experiment Experiment::GetExperiment(Key key)
    {
        switch (key)
        {
        case Key::CDN:
            return Experiment{ "CDN experiment", "CDN", "https://aka.ms/winget-settings", "CDN"};
            
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    std::vector<Experiment> Experiment::GetAllExperiments()
    {
        std::vector<Experiment> result;

        for (Key_t i = 0x1; i < static_cast<Key_t>(Key::Max); i = i << 1)
        {
            result.emplace_back(GetExperiment(static_cast<Key>(i)));
        }

        return result;
    }
}
