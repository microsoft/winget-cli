// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerStrings.h"
#include "SettingValidation.h"
#include "winget\JsonUtil.h"
#include "winget\settings\Visual.h"

#include <string>

namespace AppInstaller::Settings
{
    using namespace std::string_view_literals;

    // Visual root
    static constexpr std::string_view s_VisualKey = "visual"sv;

    // progressBar property possible values
    static constexpr std::string_view s_progressBar_Accent = "accent";
    static constexpr std::string_view s_progressBar_Rainbow = "rainbow";
    static constexpr std::string_view s_progressBar_Retro = "retro";

    Visual::Visual(const Json::Value& node) : m_progressBar(VisualStyle::Accent)
    {
        std::vector<SettingFieldInfo> visualSettings =
        {
            {
                "progressBar",
                [this](const std::string& key, const Json::Value& node)
                {
                    if (!node.isNull())
                    {
                        auto value = Utility::GetValueString(key, node);
                        if (value.has_value())
                        {
                            if (Utility::CaseInsensitiveEquals(value.value(), s_progressBar_Accent))
                            {
                                m_progressBar = VisualStyle::Accent;
                            }
                            else if (Utility::CaseInsensitiveEquals(value.value(), s_progressBar_Rainbow))
                            {
                                m_progressBar = VisualStyle::Rainbow;
                            }
                            else if (Utility::CaseInsensitiveEquals(value.value(), s_progressBar_Retro))
                            {
                                m_progressBar = VisualStyle::Retro;
                            }
                            else
                            {
                                // TODO: Localize
                                std::string warning = "Invalid value for " + key + ": " + value.value();
                                m_warnings.emplace_back(warning);
                            }
                        }
                        else
                        {
                            // TODO: Localize.
                            std::string warning = "Invalid format for " + key;
                            m_warnings.emplace_back(warning);
                        }
                    }
                }
            },
        };

        auto warnings = ValidateAndProcessFields(node, visualSettings);
        std::move(warnings.begin(), warnings.end(), std::inserter(m_warnings, m_warnings.end()));
    }

    std::string_view Visual::GetPropertyName()
    {
        return s_VisualKey;
    }
}