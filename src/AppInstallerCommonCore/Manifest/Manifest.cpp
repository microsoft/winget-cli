// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Manifest.h"
#include "winget/Locale.h"
#include "winget/UserSettings.h"

namespace AppInstaller::Manifest
{
    void Manifest::ApplyLocale(const std::string& locale)
    {
        CurrentLocalization = DefaultLocalization;

        // Get target locale from winget settings or Preferred Languages settings if applicable
        std::string targetLocale = locale;
        if (targetLocale.empty())
        {
            targetLocale = Settings::User().Get<Settings::Setting::InstallLocalePreference>();
            if (targetLocale.empty())
            {
                auto preferredList = Utility::GetUserPreferredLanguages();
                if (!preferredList.empty())
                {
                    // TODO: we only take the first one for now
                    targetLocale = preferredList.at(0);
                }
            }
        }

        if (targetLocale.empty())
        {
            return;
        }

        const ManifestLocalization* bestLocalization = nullptr;
        double bestScore = Utility::GetDistanceOfLanguage(targetLocale, DefaultLocalization.Locale);

        for (auto const& localization : Localizations)
        {
            double score = Utility::GetDistanceOfLanguage(targetLocale, localization.Locale);
            if (score > bestScore)
            {
                bestLocalization = &localization;
                bestScore = score;
            }
        }

        if (bestLocalization != nullptr)
        {
            CurrentLocalization.ReplaceOrMergeWith(*bestLocalization);
        }
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