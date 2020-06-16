// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerStrings.h"
#include "SettingValidation.h"
#include "winget\JsonUtil.h"
#include "winget\settings\Source.h"

#include <string>

#include <iostream>

namespace AppInstaller::Settings
{
    using namespace std::string_view_literals;

    // Visual root
    static constexpr std::string_view s_SourceKey = "source"sv;

    // Default values;
    static constexpr uint32_t Five = 5;

    Source::Source(const Json::Value& node) : m_autoUpdateIntervalInMinutes(Five)
    {
        std::vector<SettingFieldInfo> sourceSettings =
        {
            {
                "autoUpdateIntervalInMinutes",
                [this](const std::string& key, const Json::Value& node)
                {
                    if (!node.isNull())
                    {
                        auto autoUpdateInterval = Utility::GetValueUInt(key, node);
                        if (autoUpdateInterval.has_value())
                        {
                            m_autoUpdateIntervalInMinutes = autoUpdateInterval.value();
                        }
                        else
                        {
                            // TODO: Localize
                            std::string warning = "Invalid format for: " + key;
                            m_warnings.emplace_back(warning);
                        }
                    }
                }
            },
        };

        auto warnings = ValidateAndProcessFields(node, sourceSettings);
        std::move(warnings.begin(), warnings.end(), std::inserter(m_warnings, m_warnings.end()));
    }

    std::string_view Source::GetPropertyName()
    {
        return s_SourceKey;
    }
}
