
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
                    "Experiment " << Experiment::GetExperiment(key).Name() <<
                    " is disabled due to group policy: " << TogglePolicy::GetPolicy(TogglePolicy::Policy::Experiments).RegValueName());
                return false;
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
                return isEnabled;
            }

            auto isEnabled = AppInstaller::Experiment::IsEnabled(experiment.GetKey());
            AICLI_LOG(Core, Info, <<
                "Experiment " << Experiment::GetExperiment(key).Name() <<
                " is set to " << isEnabled);
            return isEnabled;
        }
    }

    // Define static members
    std::map<Experiment::Key, bool> Experiment::m_isEnabledCache;
    std::mutex Experiment::m_mutex;

    bool Experiment::IsEnabled(Key key)
    {
        std::lock_guard lock(m_mutex);

#ifndef AICLI_DISABLE_TEST_HOOKS
        m_isEnabledCache.clear();
#endif

        if (m_isEnabledCache.find(key) == m_isEnabledCache.end())
        {
            m_isEnabledCache[key] = IsEnabledInternal(key, User());
        }

        return m_isEnabledCache[key];
    }

    Experiment Experiment::GetExperiment(Key key)
    {
        switch (key)
        {
        case Key::CDN:
            return Experiment{ "CDN experiment", "CDN", "https://aka.ms/winget-settings", "CDN"};
#ifndef AICLI_DISABLE_TEST_HOOKS
        case Key::TestExperimentDisabledByDefault:
            return Experiment{ "Test experiment disabled by default", "TestExperimentDisabledByDefault", "https://aka.ms/winget-settings", "TestExperimentDisabledByDefault" };
        case Key::TestExperimentEnabledByDefault:
            return Experiment{ "Test experiment enabled by default", "TestExperimentEnabledByDefault", "https://aka.ms/winget-settings", "TestExperimentEnabledByDefault" };
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
