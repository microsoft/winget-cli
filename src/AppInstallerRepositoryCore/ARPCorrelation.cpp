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
        constexpr double MinimumDifferentiationThreshold = 0.05;

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

    // Find the best match using heuristics
    ARPHeuristicsCorrelationResult FindARPEntryForNewlyInstalledPackageWithHeuristics(
        const Manifest::Manifest& manifest,
        const std::vector<ARPEntry>& arpEntries)
    {
        // TODO: In the future we can make different passes with different algorithms until we find a match
        return FindARPEntryForNewlyInstalledPackageWithHeuristics(manifest, arpEntries, IARPMatchConfidenceAlgorithm::Instance());
    }

    ARPHeuristicsCorrelationResult FindARPEntryForNewlyInstalledPackageWithHeuristics(
        const AppInstaller::Manifest::Manifest& manifest,
        const std::vector<ARPEntry>& arpEntries,
        IARPMatchConfidenceAlgorithm& algorithm)
    {
        if (arpEntries.empty())
        {
            AICLI_LOG(Repo, Warning, << "Empty ARP entries given");
            return {};
        }

        AICLI_LOG(Repo, Verbose, << "Looking for best match in ARP for manifest " << manifest.Id);

        algorithm.Init(manifest);

        ARPHeuristicsCorrelationResult result;
        result.Measures.reserve(arpEntries.size());

        for (const auto& arpEntry : arpEntries)
        {
            auto score = algorithm.ComputeConfidence(arpEntry);
            AICLI_LOG(Repo, Verbose, << "Match confidence for " << arpEntry.Entry->GetProperty(PackageProperty::Id) << ": " << score);

            result.Measures.emplace_back(CorrelationMeasure{ score, arpEntry.Entry->GetLatestVersion() });
        }

        std::sort(result.Measures.begin(), result.Measures.end(), [](const CorrelationMeasure& a, const CorrelationMeasure& b) { return a.Measure > b.Measure; });

        if (result.Measures[0].Measure < MatchingThreshold)
        {
            AICLI_LOG(Repo, Verbose, << "Maximum score [" << result.Measures[0].Measure << "] is lower than threshold [" << MatchingThreshold << "]");
            result.Reason = "maximum score below threshold";
        }
        else if (result.Measures.size() >= 2 && (result.Measures[0].Measure - result.Measures[1].Measure) < MinimumDifferentiationThreshold)
        {
            AICLI_LOG(Repo, Verbose, << "Top two scores, [" << result.Measures[0].Measure << "] and [" << result.Measures[1].Measure << "] are not significantly different [" << MinimumDifferentiationThreshold << "]");
            result.Reason = "top two scores are not significantly different";
        }
        else
        {
            AICLI_LOG(Repo, Verbose, << "Best match is " << result.Measures[0].Package->GetProperty(PackageVersionProperty::Id));
            result.Package = result.Measures[0].Package;
            result.Reason = "heuristics match";
        }

        return result;
    }

    void ARPCorrelationData::CapturePreInstallSnapshot()
    {
        ProgressCallback empty;
        Repository::Source preInstallARP = Repository::Source(PredefinedSource::ARP);
        preInstallARP.Open(empty);

        for (const auto& entry : preInstallARP.Search({}).Matches)
        {
            auto installed = entry.Package->GetInstalled()->GetLatestVersion();
            if (installed)
            {
                m_preInstallSnapshot.emplace_back(std::make_tuple(
                    installed->GetProperty(PackageVersionProperty::Id),
                    installed->GetProperty(PackageVersionProperty::Version),
                    installed->GetProperty(PackageVersionProperty::Channel)));
            }
        }

        std::sort(m_preInstallSnapshot.begin(), m_preInstallSnapshot.end());
    }

    void ARPCorrelationData::CapturePostInstallSnapshot()
    {
        ProgressCallback empty;
        m_postInstallSnapshotSource = Repository::Source(PredefinedSource::ARP);
        m_postInstallSnapshotSource.Open(empty);

        for (auto& entry : m_postInstallSnapshotSource.Search({}).Matches)
        {
            auto installed = entry.Package->GetInstalled()->GetLatestVersion();

            if (installed)
            {
                auto entryKey = std::make_tuple(
                    installed->GetProperty(PackageVersionProperty::Id),
                    installed->GetProperty(PackageVersionProperty::Version),
                    installed->GetProperty(PackageVersionProperty::Channel));

                auto itr = std::lower_bound(m_preInstallSnapshot.begin(), m_preInstallSnapshot.end(), entryKey);
                m_postInstallSnapshot.emplace_back(entry.Package->GetInstalled(), itr == m_preInstallSnapshot.end() || *itr != entryKey);
            }
        }
    }

    ARPCorrelationResult ARPCorrelationData::CorrelateForNewlyInstalled(const Manifest::Manifest& manifest, const ARPCorrelationSettings& settings)
    {
        AICLI_LOG(Repo, Verbose, << "Finding ARP entry matching newly installed package");

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

        std::set<std::string> productCodes;
        std::set<std::string> upgradeCodes;
        for (const auto& installer : manifest.Installers)
        {
            if (!installer.ProductCode.empty())
            {
                // Add each ProductCode only once
                if (productCodes.insert(installer.ProductCode).second)
                {
                    manifestSearchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::ProductCode, MatchType::Exact, installer.ProductCode));
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

                // Add each ProductCode and UpgradeCode only once;
                if (!appsAndFeaturesEntry.ProductCode.empty() && productCodes.insert(appsAndFeaturesEntry.ProductCode).second)
                {
                    manifestSearchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::ProductCode, MatchType::Exact, appsAndFeaturesEntry.ProductCode));
                }
                if (!appsAndFeaturesEntry.UpgradeCode.empty() && upgradeCodes.insert(appsAndFeaturesEntry.UpgradeCode).second)
                {
                    manifestSearchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::UpgradeCode, MatchType::Exact, appsAndFeaturesEntry.UpgradeCode));
                }
            }
        }

        SearchResult findByManifest;

        // Don't execute this search if it would just find everything
        if (!manifestSearchRequest.IsForEverything())
        {
            findByManifest = m_postInstallSnapshotSource.Search(manifestSearchRequest);
        }

        // Cross reference the changes with the search results
        std::vector<std::shared_ptr<IPackage>> packagesInBoth;

        for (const auto& change : m_postInstallSnapshot)
        {
            if (change.IsNewOrUpdated)
            {
                for (const auto& byManifest : findByManifest.Matches)
                {
                    if (change.Entry->IsSame(byManifest.Package->GetInstalled().get()))
                    {
                        packagesInBoth.emplace_back(change.Entry);
                        break;
                    }
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
        result.ChangesToARP = std::count_if(m_postInstallSnapshot.begin(), m_postInstallSnapshot.end(), [](const ARPEntry& e) { return e.IsNewOrUpdated; });
        result.MatchesInARP = findByManifest.Matches.size();
        result.CountOfIntersectionOfChangesAndMatches = packagesInBoth.size();

        // If there is only a single common package (changed and matches), it is almost certainly the correct one.
        if (settings.AllowNormalization && packagesInBoth.size() == 1)
        {
            result.Package = packagesInBoth[0]->GetLatestVersion();
            result.Reason = "normalization match and new/changed";
        }
        // If it wasn't changed but we still find a match, that is the best thing to report.
        else if (settings.AllowNormalization && findByManifest.Matches.size() == 1)
        {
            result.Package = findByManifest.Matches[0].Package->GetInstalled()->GetLatestVersion();
            result.Reason = "normalization match (not new/changed)";
        }
        else if (settings.AllowSingleChange && result.ChangesToARP == 1)
        {
            result.Package = std::find_if(m_postInstallSnapshot.begin(), m_postInstallSnapshot.end(), [](const ARPEntry& e) { return e.IsNewOrUpdated; })->Entry->GetLatestVersion();
            result.Reason = "only new/changed value";
        }
        else
        {
            // We were not able to find an exact match, so we now run some heuristics
            // to try and match the package with some ARP entry by assigning them scores.
            AICLI_LOG(Repo, Verbose, << "No exact ARP match found. Trying to find one with heuristics");

            result = FindARPEntryForNewlyInstalledPackageWithHeuristics(manifest, m_postInstallSnapshot);
        }

        return result;
    }
}
