// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>
#include <type_traits>
#include "AppInstallerStrings.h"

namespace AppInstaller::Settings
{
    struct Experiment
    {
        enum class Key : unsigned
        {
            None = 0x0,
            CDN = 0x1,
            Max,
        };

        using Key_t = std::underlying_type_t<Key>;

        Experiment(std::string_view name, std::string_view jsonName, std::string_view link, std::string key) :
            m_name(name), m_jsonName(jsonName), m_link(link), m_key(key) {}

        static bool IsEnabled(Key feature);

        static Experiment GetExperiment(Experiment::Key key);
        static std::vector<Experiment> GetAllExperiments();

        std::string_view Name() const { return m_name; }
        Utility::LocIndView JsonName() const { return m_jsonName; }
        std::string_view Link() const { return m_link; }
        std::string GetKey() const { return m_key; }

    private:
        std::string_view m_name;
        Utility::LocIndView m_jsonName;
        std::string_view m_link;
        std::string m_key;
        static std::map<Key, bool> m_isEnabledCache;
        static std::mutex m_mutex;
    };
}
