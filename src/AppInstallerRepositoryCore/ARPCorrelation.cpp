// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/ARPCorrelation.h"
#include "winget/ARPCorrelationAlgorithms.h"
#include "winget/Manifest.h"
#include "winget/NameNormalization.h"
#include "winget/RepositorySearch.h"
#include "winget/RepositorySource.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Utility;

namespace AppInstaller::Repository::Correlation
{
    namespace
    {
        constexpr double MatchingThreshold = 0.5;

        IARPMatchConfidenceAlgorithm& InstanceInternal(std::optional<IARPMatchConfidenceAlgorithm*> algorithmOverride = {})
        {
            static WordsEditDistanceMatchConfidenceAlgorithm s_algorithm;
            static IARPMatchConfidenceAlgorithm* s_override = nullptr;

            if (algorithmOverride.has_value())
            {
                s_override = algorithmOverride.value();
            }

            if (s_override)
            {
                return *s_override;
            }
            else
            {
                return s_algorithm;
            }
        }
    }

    IARPMatchConfidenceAlgorithm& IARPMatchConfidenceAlgorithm::Instance()
    {
        return InstanceInternal();
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    void IARPMatchConfidenceAlgorithm::OverrideInstance(IARPMatchConfidenceAlgorithm* algorithmOverride)
    {
        InstanceInternal(algorithmOverride);
    }

    void IARPMatchConfidenceAlgorithm::ResetInstance()
    {
        InstanceInternal(nullptr);
    }
#endif

    ARPCorrelationResult FindARPEntryForNewlyInstalledPackage(
        const Manifest::Manifest& manifest,
        const std::vector<ARPEntrySnapshot>& arpSnapshot,
        Source& arpSource)
    {
        AICLI_LOG(Repo, Verbose, << "Finding ARP entry matching newly installed package");

        std::vector<Correlation::ARPEntry> changedArpEntries;
        std::vector<Correlation::ARPEntry> existingArpEntries;

        for (auto& entry : arpSource.Search({}).Matches)
        {
            auto installed = entry.Package->GetInstalledVersion();

            if (installed)
            {
                auto entryKey = std::make_tuple(
                    entry.Package->GetProperty(PackageProperty::Id),
                    installed->GetProperty(PackageVersionProperty::Version),
                    installed->GetProperty(PackageVersionProperty::Channel));

                auto itr = std::lower_bound(arpSnapshot.begin(), arpSnapshot.end(), entryKey);
                if (itr == arpSnapshot.end() || *itr != entryKey)
                {
                    changedArpEntries.emplace_back(entry.Package, true);
                }
                else
                {
                    existingArpEntries.emplace_back(entry.Package, false);
                }
            }
        }

        // Also attempt to find the entry based on the manifest data

        SearchRequest manifestSearchRequest;
        AppInstaller::Manifest::Manifest::string_t defaultPublisher;
        if (manifest.DefaultLocalization.Contains(Localization::Publisher))
        {
            defaultPublisher = manifest.DefaultLocalization.Get<Localization::Publisher>();
        }

        // The default localization must contain the name or we cannot do this lookup
        if (manifest.DefaultLocalization.Contains(Localization::PackageName))
        {
            AppInstaller::Manifest::Manifest::string_t defaultName = manifest.DefaultLocalization.Get<Localization::PackageName>();
            manifestSearchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::NormalizedNameAndPublisher, MatchType::Exact, defaultName, defaultPublisher));

            for (const auto& loc : manifest.Localizations)
            {
                if (loc.Contains(Localization::PackageName) || loc.Contains(Localization::Publisher))
                {
                    manifestSearchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::NormalizedNameAndPublisher, MatchType::Exact,
                        loc.Contains(Localization::PackageName) ? loc.Get<Localization::PackageName>() : defaultName,
                        loc.Contains(Localization::Publisher) ? loc.Get<Localization::Publisher>() : defaultPublisher));
                }
            }
        }

        std::vector<std::string> productCodes;
        for (const auto& installer : manifest.Installers)
        {
            if (!installer.ProductCode.empty())
            {
                if (std::find(productCodes.begin(), productCodes.end(), installer.ProductCode) == productCodes.end())
                {
                    manifestSearchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::ProductCode, MatchType::Exact, installer.ProductCode));
                    productCodes.emplace_back(installer.ProductCode);
                }
            }

            for (const auto& appsAndFeaturesEntry : installer.AppsAndFeaturesEntries)
            {
                if (!appsAndFeaturesEntry.DisplayName.empty())
                {
                    manifestSearchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::NormalizedNameAndPublisher, MatchType::Exact,
                        appsAndFeaturesEntry.DisplayName,
                        appsAndFeaturesEntry.Publisher.empty() ? defaultPublisher : appsAndFeaturesEntry.Publisher));
                }
            }
        }

        SearchResult findByManifest;

        // Don't execute this search if it would just find everything
        if (!manifestSearchRequest.IsForEverything())
        {
            findByManifest = arpSource.Search(manifestSearchRequest);
        }

        // Cross reference the changes with the search results
        std::vector<std::shared_ptr<IPackage>> packagesInBoth;

        for (const auto& change : changedArpEntries)
        {
            for (const auto& byManifest : findByManifest.Matches)
            {
                if (change.Entry->IsSame(byManifest.Package.get()))
                {
                    packagesInBoth.emplace_back(change.Entry);
                    break;
                }
            }
        }

        // We now have all of the package changes; time to report them.
        //
        // The set of cases we could have for finding packages based on the manifest:
        //  0 packages  ::  The manifest data does not match the ARP information.
        //  1 package   ::  Golden path; this should be what we installed.
        //  2+ packages ::  The data in the manifest is either too broad or we have
        //                  a problem with our name normalization.

        // Find the package that we are going to log
        ARPCorrelationResult result;
        // TODO: Find a good way to consider the other heuristics in these stats.
        result.ChangesToARP = changedArpEntries.size();
        result.MatchesInARP = findByManifest.Matches.size();
        result.CountOfIntersectionOfChangesAndMatches = packagesInBoth.size();

        // If there is only a single common package (changed and matches), it is almost certainly the correct one.
        if (packagesInBoth.size() == 1)
        {
            result.Package = packagesInBoth[0]->GetInstalledVersion();
        }
        // If it wasn't changed but we still find a match, that is the best thing to report.
        else if (findByManifest.Matches.size() == 1)
        {
            result.Package = findByManifest.Matches[0].Package->GetInstalledVersion();
        }
        else
        {
            // We were not able to find an exact match, so we now run some heuristics
            // to try and match the package with some ARP entry by assigning them scores.
            AICLI_LOG(Repo, Verbose, << "No exact ARP match found. Trying to find one with heuristics");

            std::vector<ARPEntry> arpEntries;
            for (auto&& entry : changedArpEntries)
            {
                arpEntries.push_back(std::move(entry));
            }
            for (auto&& entry : existingArpEntries)
            {
                arpEntries.push_back(std::move(entry));
            }

            result.Package = FindARPEntryForNewlyInstalledPackageWithHeuristics(manifest, arpEntries);
        }

        return result;
    }

    // Find the best match using heuristics
    std::shared_ptr<IPackageVersion> FindARPEntryForNewlyInstalledPackageWithHeuristics(
        const Manifest::Manifest& manifest,
        const std::vector<ARPEntry>& arpEntries)
    {
        // TODO: In the future we can make different passes with different algorithms until we find a match
        return FindARPEntryForNewlyInstalledPackageWithHeuristics(manifest, arpEntries, IARPMatchConfidenceAlgorithm::Instance());
    }

    std::shared_ptr<IPackageVersion> FindARPEntryForNewlyInstalledPackageWithHeuristics(
        const AppInstaller::Manifest::Manifest& manifest,
        const std::vector<ARPEntry>& arpEntries,
        IARPMatchConfidenceAlgorithm& algorithm)
    {
        AICLI_LOG(Repo, Verbose, << "Looking for best match in ARP for manifest " << manifest.Id);

        algorithm.Init(manifest);

        std::optional<ARPEntry> bestMatch;
        double bestScore = 0;

        for (const auto& arpEntry : arpEntries)
        {
            auto score = algorithm.ComputeConfidence(arpEntry);
            AICLI_LOG(Repo, Verbose, << "Match confidence for " << arpEntry.Entry->GetProperty(PackageProperty::Id) << ": " << score);

            if (score < MatchingThreshold)
            {
                AICLI_LOG(Repo, Verbose, << "Score is lower than threshold");
                continue;
            }

            if (!bestMatch || bestScore < score)
            {
                bestMatch = arpEntry;
                bestScore = score;
            }
        }

        if (bestMatch)
        {
            AICLI_LOG(Repo, Verbose, << "Best match is " << bestMatch->Entry->GetProperty(PackageProperty::Id));
        }
        else
        {
            AICLI_LOG(Repo, Verbose, << "No ARP entry had a correlation score surpassing the required threshold");
        }

        return bestMatch ? bestMatch->Entry->GetInstalledVersion() : nullptr;
    }
}