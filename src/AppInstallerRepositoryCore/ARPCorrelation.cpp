// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/ARPCorrelation.h"
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

        struct EntryScore
        {
            ARPEntry Entry;
            double Score;

            EntryScore(
                const ARPCorrelationAlgorithm& matchingAlgorithm,
                const Manifest::Manifest& manifest,
                const ARPEntry& entry)
                : Entry(entry), Score(matchingAlgorithm.GetMatchingScore(manifest, entry)) {}

            bool operator<(const EntryScore& other)
            {
                return Score < other.Score;
            }
        };

        template<typename T>
        struct BasicARPCorrelationAlgorithm : public ARPCorrelationAlgorithm
        {
            double GetMatchingScore(
                const Manifest::Manifest&,
                const ManifestLocalization& manifestLocalization,
                const ARPEntry& arpEntry) const override
            {
                // Overall algorithm:
                // This considers only the matching between name/publisher.
                // It ignores versions and whether the ARP entry is new.
                const auto packageName = manifestLocalization.Get<Localization::PackageName>();
                const auto packagePublisher = manifestLocalization.Get<Localization::Publisher>();

                const auto arpNames = arpEntry.Entry->GetMultiProperty(PackageVersionMultiProperty::Name);
                const auto arpPublishers = arpEntry.Entry->GetMultiProperty(PackageVersionMultiProperty::Publisher);

                // TODO: Comments say that these should match, but it seems they don't always do
                if (arpNames.size() != arpPublishers.size())
                {
                    return 0;
                }

                T nameAndPublisherCorrelationMeasure;
                double bestMatch = 0;
                for (size_t i = 0; i < arpNames.size(); ++i)
                {
                    bestMatch = std::max(bestMatch, nameAndPublisherCorrelationMeasure.GetMatchingScore(packageName, packagePublisher, arpNames[i], arpPublishers[i]));
                }

                return bestMatch;
            }
        };

        double EditDistanceScore(std::string_view sv1, std::string_view sv2)
        {
            // Naive implementation of edit distance (scaled over the string size)
            // TODO: This implementation does not consider multi-byte symbols.

            // We may have empty values coming from the ARP
            if (sv1.empty() || sv2.empty())
            {
                return 0;
            }

            // Do it ignoring case
            auto s1 = Utility::FoldCase(sv1);
            auto s2 = Utility::FoldCase(sv2);

            // distance[i][j] = distance between s1[0:i] and s2[0:j]
            std::vector<std::vector<double>> distance{ s1.size(), std::vector<double>(s2.size(), 0.0) };

            for (size_t i = 0; i < s1.size(); ++i)
            {
                for (size_t j = 0; j < s2.size(); ++j)
                {
                    double& d = distance[i][j];
                    if (i == 0)
                    {
                        d = static_cast<double>(j);
                    }
                    else if (j == 0)
                    {
                        d = static_cast<double>(i);
                    }
                    else if (s1[i] == s2[j])
                    {
                        d = distance[i - 1][j - 1];
                    }
                    else
                    {
                        d = std::min(
                            1 + distance[i - 1][j - 1],
                            1 + std::min(distance[i][j - 1], distance[i - 1][j]));
                    }
                }
            }

            // Maximum distance is equal to the length of the longest string.
            // We use that to scale to [0,1].
            // A smaller distance represents a higher match, so we subtract from 1 for the final score
            double editDistance = distance.back().back();
            return 1 - editDistance / std::max(s1.size(), s2.size());
        }

    }

    double ARPCorrelationAlgorithm::GetMatchingScore(
        const Manifest::Manifest& manifest,
        const ARPEntry& arpEntry) const
    {
        // Get the best score across all localizations
        double bestMatchingScore = GetMatchingScore(manifest, manifest.DefaultLocalization, arpEntry);
        for (const auto& localization : manifest.Localizations)
        {
            double matchingScore = GetMatchingScore(manifest, localization, arpEntry);
            bestMatchingScore = std::max(bestMatchingScore, matchingScore);
        }

        return bestMatchingScore;
    }

    std::optional<ARPEntry> ARPCorrelationAlgorithm::GetBestMatchForManifest(
        const Manifest::Manifest& manifest,
        const std::vector<ARPEntry>& arpEntries) const
    {
        AICLI_LOG(Repo, Verbose, << "Looking for best match in ARP for manifest " << manifest.Id);

        std::optional<ARPEntry> bestMatch;
        double bestScore = std::numeric_limits<double>::lowest();

        for (const auto& arpEntry : arpEntries)
        {
            auto score = GetMatchingScore(manifest, arpEntry);
            AICLI_LOG(Repo, Verbose, << "Match score for " << arpEntry.Entry->GetProperty(PackageVersionProperty::Id) << ": " << score);

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
            AICLI_LOG(Repo, Verbose, << "Best match is " << bestMatch->Entry->GetProperty(PackageVersionProperty::Id));
        }
        else
        {
            AICLI_LOG(Repo, Verbose, << "No ARP entry had a correlation score surpassing the required threshold");
        }

        return bestMatch;
    }

    const ARPCorrelationAlgorithm& ARPCorrelationAlgorithm::GetInstance()
    {
        static BasicARPCorrelationAlgorithm<EditDistanceNormalizedNameAndPublisherCorrelationMeasure> instance;
        return instance;
    }

    double EmptyNameAndPublisherCorrelationMeasure::GetMatchingScore(std::string_view, std::string_view, std::string_view, std::string_view) const
    {
        return 0;
    }

    double NormalizedNameAndPublisherCorrelationMeasure::GetMatchingScore(std::string_view packageName, std::string_view packagePublisher, std::string_view arpName, std::string_view arpPublisher) const
    {
        NameNormalizer normer(NormalizationVersion::Initial);

        auto packageNormalizedName = normer.Normalize(packageName, packagePublisher);
        auto arpNormalizedName = normer.Normalize(arpName, arpPublisher);

        if (Utility::CaseInsensitiveEquals(arpNormalizedName.Name(), packageNormalizedName.Name()) &&
            Utility::CaseInsensitiveEquals(arpNormalizedName.Publisher(), packageNormalizedName.Publisher()))
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }

    double EditDistanceNormalizedNameAndPublisherCorrelationMeasure::GetMatchingScore(std::string_view packageName, std::string_view packagePublisher, std::string_view arpName, std::string_view arpPublisher) const
    {
        NameNormalizer normer(NormalizationVersion::Initial);

        auto packageNormalizedName = normer.Normalize(packageName, packagePublisher);
        auto arpNormalizedName = normer.Normalize(arpName, arpPublisher);

        auto nameDistance = EditDistanceScore(arpNormalizedName.Name(), packageNormalizedName.Name());
        auto publisherDistance = EditDistanceScore(arpNormalizedName.Publisher(), packageNormalizedName.Publisher());

        // TODO: Consider other ways of merging the two values
        return (2 * nameDistance + publisherDistance) / 3;
    }

    std::shared_ptr<AppInstaller::Repository::IPackageVersion> FindARPEntryForNewlyInstalledPackage(
        const Manifest::Manifest& manifest,
        const std::vector<ARPEntrySnapshot>& arpSnapshot,
        Source& arpSource,
        std::string_view sourceIdentifier)
    {
        AICLI_LOG(Repo, Verbose, << "Finding ARP entry matching newly installed package");
        std::vector<ResultMatch> changes;

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
                    changes.emplace_back(std::move(entry));
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

        for (const auto& change : changes)
        {
            for (const auto& byManifest : findByManifest.Matches)
            {
                if (change.Package->IsSame(byManifest.Package.get()))
                {
                    packagesInBoth.emplace_back(change.Package);
                    break;
                }
            }
        }

        // We now have all of the package changes; time to report them.
        // The set of cases we could have for changes to ARP:
        //  0 packages  ::  No changes were detected to ARP, which could mean that the installer
        //                  did not write an entry. It could also be a forced reinstall.
        //  1 package   ::  Golden path; this should be what we installed.
        //  2+ packages ::  We need to determine which package actually matches the one that we
        //                  were installing.
        //
        // The set of cases we could have for finding packages based on the manifest:
        //  0 packages  ::  The manifest data does not match the ARP information.
        //  1 package   ::  Golden path; this should be what we installed.
        //  2+ packages ::  The data in the manifest is either too broad or we have
        //                  a problem with our name normalization.

        // Find the package that we are going to log
        std::shared_ptr<IPackageVersion> toLog;

        // If there is only a single common package (changed and matches), it is almost certainly the correct one.
        if (packagesInBoth.size() == 1)
        {
            toLog = packagesInBoth[0]->GetInstalledVersion();
        }
        // If it wasn't changed but we still find a match, that is the best thing to report.
        else if (findByManifest.Matches.size() == 1)
        {
            toLog = findByManifest.Matches[0].Package->GetInstalledVersion();
        }
        // If only a single ARP entry was changed and we found no matches, report that.
        else if (findByManifest.Matches.empty() && changes.size() == 1)
        {
            toLog = changes[0].Package->GetInstalledVersion();
        }

        if (!toLog)
        {
            // We were not able to find an exact match, so we now run some heuristics
            // to try and match the package with some ARP entry by assigning them scores.
            AICLI_LOG(Repo, Verbose, << "No exact ARP match found. Trying to find one with heuristics");
            toLog = FindARPEntryForNewlyInstalledPackageWithHeuristics(manifest, arpSnapshot, arpSource);
        }

        IPackageVersion::Metadata toLogMetadata;
        if (toLog)
        {
            toLogMetadata = toLog->GetMetadata();
        }

        Logging::Telemetry().LogSuccessfulInstallARPChange(
            sourceIdentifier,
            manifest.Id,
            manifest.Version,
            manifest.Channel,
            changes.size(),
            findByManifest.Matches.size(),
            packagesInBoth.size(),
            toLog ? static_cast<std::string>(toLog->GetProperty(PackageVersionProperty::Name)) : "",
            toLog ? static_cast<std::string>(toLog->GetProperty(PackageVersionProperty::Version)) : "",
            toLog ? static_cast<std::string_view>(toLogMetadata[PackageVersionMetadata::Publisher]) : "",
            toLog ? static_cast<std::string_view>(toLogMetadata[PackageVersionMetadata::InstalledLocale]) : ""
        );

        return toLog;
    }

    // Find the best match using heuristics
    std::shared_ptr<AppInstaller::Repository::IPackageVersion> FindARPEntryForNewlyInstalledPackageWithHeuristics(
        const Manifest::Manifest& manifest,
        const std::vector<ARPEntrySnapshot>& arpSnapshot,
        Source& arpSource)
    {
        // First format the ARP data appropriately for the heuristic search
        std::vector<Correlation::ARPEntry> arpEntriesForCorrelation;
        for (auto& entry : arpSource.Search({}).Matches)
        {
            // TODO: Remove duplication with the other function
            auto installed = entry.Package->GetInstalledVersion();

            if (installed)
            {
                // Compare with the previous snapshot to see if it changed.
                auto entryKey = std::make_tuple(
                    entry.Package->GetProperty(PackageProperty::Id),
                    installed->GetProperty(PackageVersionProperty::Version),
                    installed->GetProperty(PackageVersionProperty::Channel));

                auto itr = std::lower_bound(arpSnapshot.begin(), arpSnapshot.end(), entryKey);
                bool isNewOrUpdated = (itr == arpSnapshot.end() || *itr != entryKey);
                arpEntriesForCorrelation.emplace_back(installed, isNewOrUpdated);
            }
        }

        // Find the best match
        const auto& correlationMeasure = Correlation::ARPCorrelationAlgorithm::GetInstance();
        auto bestMatch = correlationMeasure.GetBestMatchForManifest(manifest, arpEntriesForCorrelation);
        return bestMatch ? bestMatch->Entry : nullptr;
    }
}