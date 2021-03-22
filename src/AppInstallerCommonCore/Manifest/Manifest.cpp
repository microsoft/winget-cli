// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Manifest.h"

namespace AppInstaller::Manifest
{
    void Manifest::ApplyLocale(const std::string&)
    {
        // TODO: need more work in locale processing
        CurrentLocalization = DefaultLocalization;
    }

    std::vector<string_t> Manifest::GetAggregatedTags() const
    {
        std::vector<string_t> resultTags = DefaultLocalization.Get<Localization::Tags>();

        for (const auto& locale : Localizations)
        {
            auto tags = locale.Get<Localization::Tags>();
            for (const auto& tag : tags)
            {
                if (std::find(resultTags.begin(), resultTags.end(), tag) == resultTags.end())
                {
                    resultTags.emplace_back(tag);
                }
            }
        }

        return resultTags;
    }

    std::vector<string_t> Manifest::GetAggregatedCommands() const
    {
        std::vector<string_t> resultCommands;

        for (const auto& installer : Installers)
        {
            for (const auto& command : installer.Commands)
            {
                if (std::find(resultCommands.begin(), resultCommands.end(), command) == resultCommands.end())
                {
                    resultCommands.emplace_back(command);
                }
            }
        }

        return resultCommands;
    }
}