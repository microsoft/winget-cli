// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/ARPCorrelation.h"
#include "winget/Manifest.h"
#include "winget/NameNormalization.h"
#include "winget/RepositorySearch.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Utility;

namespace AppInstaller::Repository::Correlation
{
    namespace
    {
        struct EntryScore
        {
            ARPEntry Entry;
            double Score;

            EntryScore(
                const ARPCorrelationMeasure& measure,
                const Manifest::Manifest& manifest,
                const ARPEntry& entry)
                : Entry(entry), Score(measure.GetMatchingScore(manifest, entry)) {}

            bool operator<(const EntryScore& other)
            {
                return Score < other.Score;
            }
        };
    }

    std::optional<ARPEntry> ARPCorrelationMeasure::GetBestMatchForManifest(
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

            if (score < GetMatchingThreshold())
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
            AICLI_LOG(Repo, Verbose, << "Best match is " << bestMatch->Name);
        }
        else
        {
            AICLI_LOG(Repo, Verbose, << "No ARP entry had a correlation score surpassing the required threshold");
        }

        return bestMatch;
    }

    const ARPCorrelationMeasure& ARPCorrelationMeasure::GetInstance()
    {
        static NoCorrelation instance;
        return instance;
    }

    double NoCorrelation::GetMatchingScore(
        const Manifest::Manifest& manifest,
        const ARPEntry& arpEntry) const
    {
        UNREFERENCED_PARAMETER(manifest);
        UNREFERENCED_PARAMETER(arpEntry);
        return 0;
    }

    double NoCorrelation::GetMatchingThreshold() const
    {
        return 1;
    }

    double NormalizedNameAndPublisherCorrelation::GetMatchingScore(
        const Manifest::Manifest& manifest,
        const ARPEntry& arpEntry) const
    {
        NameNormalizer normer(NormalizationVersion::Initial);

        auto arpNormalizedName = normer.Normalize(arpEntry.Name, arpEntry.Publisher);

        // Try to match the ARP normalized name with one of the localizations
        // for the manifest
        std::string defaultName;
        std::string defaultPublisher;

        if (manifest.DefaultLocalization.Contains(Localization::PackageName))
        {
            defaultName = manifest.DefaultLocalization.Get<Localization::PackageName>();
        }

        if (manifest.DefaultLocalization.Contains(Localization::Publisher))
        {
            defaultName = manifest.DefaultLocalization.Get<Localization::Publisher>();
        }

        for (auto& localization : manifest.Localizations)
        {
            auto name = localization.Contains(Localization::PackageName) ? localization.Get<Localization::PackageName>() : defaultName;
            auto publisher = localization.Contains(Localization::Publisher) ? localization.Get<Localization::Publisher>() : defaultPublisher;

            auto manifestNormalizedName = normer.Normalize(name, publisher);

            if (Utility::CaseInsensitiveEquals(arpNormalizedName.Name(), manifestNormalizedName.Name()) &&
                Utility::CaseInsensitiveEquals(arpNormalizedName.Publisher(), manifestNormalizedName.Publisher()))
            {
                return 1;
            }
        }

        return 0;
    }

    double NormalizedNameAndPublisherCorrelation::GetMatchingThreshold() const
    {
        return 1;
    }
}