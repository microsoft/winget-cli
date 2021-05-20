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

        // Get target locale from Preferred Languages settings if applicable
        std::vector<std::string> targetLocales;
        if (locale.empty())
        {
            targetLocales = Locale::GetUserPreferredLanguages();
        }
        else
        {
            targetLocales.emplace_back(locale);
        }

        for (auto const& targetLocale : targetLocales)
        {
            const ManifestLocalization* bestLocalization = nullptr;
            double bestScore = Locale::GetDistanceOfLanguage(targetLocale, DefaultLocalization.Locale);

            for (auto const& localization : Localizations)
            {
                double score = Locale::GetDistanceOfLanguage(targetLocale, localization.Locale);
                if (score > bestScore)
                {
                    bestLocalization = &localization;
                    bestScore = score;
                }
            }

            // If there's better locale than default And is compatible with target locale, merge and return;
            if (bestLocalization != nullptr && bestScore >= Locale::MinimumDistanceScoreAsCompatibleMatch)
            {
                CurrentLocalization.ReplaceOrMergeWith(*bestLocalization);
                break;
            }
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