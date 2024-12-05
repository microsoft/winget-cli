
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
        ExperimentState GetExperimentStateInternal(Experiment::Key key, const UserSettings& userSettings)
        {
            if (key == Experiment::Key::None)
            {
                return { true, ExperimentToggleSource::Default };
            }

            if (!GroupPolicies().IsEnabled(TogglePolicy::Policy::Experiments))
            {
                AICLI_LOG(Core, Info, <<
                    "Experiment " << Experiment::GetExperiment(key).Name() <<
                    " is disabled due to group policy: " << TogglePolicy::GetPolicy(TogglePolicy::Policy::Experiments).RegValueName());
                return { false, ExperimentToggleSource::Policy };
            }

            auto experiments = userSettings.Get<Setting::Experiments>();
            auto experiment = Experiment::GetExperiment(key);
            auto jsonName = std::string(experiment.JsonName());
            if (experiments.find(jsonName) != experiments.end())
            {
                auto isEnabled = experiments[jsonName];
                AICLI_LOG(Core, Info, <<
                    "Experiment " << Experiment::GetExperiment(key).Name() <<
                    " is set to " << isEnabled << " in user settings");
                return { isEnabled, ExperimentToggleSource::UserSetting };
            }

            auto isEnabled = AppInstaller::Experiment::IsEnabled(experiment.GetKey());
            AICLI_LOG(Core, Info, <<
                "Experiment " << Experiment::GetExperiment(key).Name() <<
                " is set to " << isEnabled);
            return { isEnabled, ExperimentToggleSource::Default };
        }
    }

    // Define static members
    std::map<Experiment::Key, ExperimentState> Experiment::m_experimentStateCache;
    std::mutex Experiment::m_mutex;

    ExperimentState Experiment::GetState(Key key)
    {
        std::lock_guard lock(m_mutex);

#ifndef AICLI_DISABLE_TEST_HOOKS
        m_experimentStateCache.clear();
#endif

        if (m_experimentStateCache.find(key) == m_experimentStateCache.end())
        {
            m_experimentStateCache[key] = GetExperimentStateInternal(key, User());
        }

        return m_experimentStateCache[key];
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

        for (Key_t i = 0x1; i < static_cast<Key_t>(Key::Max); i = i << 1)
        {
            result.emplace_back(GetExperiment(static_cast<Key>(i)));
        }

        return result;
    }
}
