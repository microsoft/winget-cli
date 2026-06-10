// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Manifest.h"
#include "winget/Locale.h"
#include "winget/UserSettings.h"

namespace AppInstaller::Manifest
{
    namespace
    {
        void AddFoldedStringToSetIfNotEmpty(std::set<string_t>& set, const string_t& value)
        {
            if (!value.empty())
            {
                set.emplace(Utility::FoldCase(value));
            }
        }
    }

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
            if (bestScore >= Locale::MinimumDistanceScoreAsCompatibleMatch)
            {
                if (bestLocalization != nullptr)
                {
                    CurrentLocalization.ReplaceOrMergeWith(*bestLocalization);
                }
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

    Utility::VersionRange Manifest::GetArpVersionRange() const
    {
        bool arpVersionFound = false;
        Utility::Version minVersion;
        Utility::Version maxVersion;

        for (auto const& installer : Installers)
        {
            if (DoesInstallerTypeSupportArpVersionRange(installer.EffectiveInstallerType()))
            {
                for (auto const& entry : installer.AppsAndFeaturesEntries)
                {
                    if (!entry.DisplayVersion.empty())
                    {
                        Utility::Version arpVersion{ entry.DisplayVersion };

                        if (!arpVersionFound)
                        {
                            // This is the first arp version found, populate both min and max version
                            minVersion = arpVersion;
                            maxVersion = arpVersion;
                            arpVersionFound = true;
                            continue;
                        }

                        if (arpVersion < minVersion)
                        {
                            minVersion = arpVersion;
                        }
                        else if (arpVersion > maxVersion)
                        {
                            maxVersion = arpVersion;
                        }
                    }
                }
            }
        }

        return arpVersionFound ? Utility::VersionRange{ minVersion, maxVersion } : Utility::VersionRange{};
    }

    std::vector<string_t> Manifest::GetPackageFamilyNames() const
    {
        return GetSystemReferenceStrings(
            [](const ManifestInstaller& i) -> const Utility::NormalizedString& { return i.PackageFamilyName; });
    }

    std::vector<string_t> Manifest::GetProductCodes() const
    {
        return GetSystemReferenceStrings(
            [](const ManifestInstaller& i) -> const Utility::NormalizedString& { return i.ProductCode; },
            [](const AppsAndFeaturesEntry& e) -> const Utility::NormalizedString& { return e.ProductCode; });
    }

    std::vector<string_t> Manifest::GetUpgradeCodes() const
    {
        return GetSystemReferenceStrings(
            {},
            [](const AppsAndFeaturesEntry& e) -> const Utility::NormalizedString& { return e.UpgradeCode; });
    }

    std::vector<string_t> Manifest::GetPackageNames() const
    {
        std::set<string_t> set;

        AddFoldedStringToSetIfNotEmpty(set, DefaultLocalization.Get<Localization::PackageName>());
        for (const auto& loc : Localizations)
        {
            AddFoldedStringToSetIfNotEmpty(set, loc.Get<Localization::PackageName>());
        }

        // In addition to the names used for our display, add the display names from the ARP entries
        for (const auto& installer : Installers)
        {
            for (const auto& appsAndFeaturesEntry : installer.AppsAndFeaturesEntries)
            {
                AddFoldedStringToSetIfNotEmpty(set, appsAndFeaturesEntry.DisplayName);
            }
        }

        std::vector<Utility::NormalizedString> result(
            std::make_move_iterator(set.begin()),
            std::make_move_iterator(set.end()));

        return result;
    }

    std::vector<string_t> Manifest::GetPublishers() const
    {
        std::set<string_t> set;

        AddFoldedStringToSetIfNotEmpty(set, DefaultLocalization.Get<Localization::Publisher>());
        for (const auto& loc : Localizations)
        {
            AddFoldedStringToSetIfNotEmpty(set, loc.Get<Localization::Publisher>());
        }

        // In addition to the publishers used for our display, add the publisher from the ARP entries
        for (const auto& installer : Installers)
        {
            for (const auto& appsAndFeaturesEntry : installer.AppsAndFeaturesEntries)
            {
                AddFoldedStringToSetIfNotEmpty(set, appsAndFeaturesEntry.Publisher);
            }
        }

        std::vector<Utility::NormalizedString> result(
            std::make_move_iterator(set.begin()),
            std::make_move_iterator(set.end()));

        return result;
    }

    std::vector<string_t> Manifest::GetSystemReferenceStrings(
        std::function<const string_t& (const ManifestInstaller&)> extractStringFromInstaller,
        std::function<const string_t& (const AppsAndFeaturesEntry&)> extractStringFromAppsAndFeaturesEntry) const
    {
        std::set<string_t> set;

        for (const auto& installer : Installers)
        {
            if (extractStringFromInstaller)
            {
                const auto& installerString = extractStringFromInstaller(installer);
                AddFoldedStringToSetIfNotEmpty(set, installerString);
            }

            if (extractStringFromAppsAndFeaturesEntry)
            {
                for (const auto& entry : installer.AppsAndFeaturesEntries)
                {
                    const auto& entryString = extractStringFromAppsAndFeaturesEntry(entry);
                    AddFoldedStringToSetIfNotEmpty(set, entryString);
                }
            }
        }

        std::vector<Utility::NormalizedString> result(
            std::make_move_iterator(set.begin()),
            std::make_move_iterator(set.end()));

        return result;
    }
}