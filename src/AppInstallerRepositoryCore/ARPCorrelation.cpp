// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/ARPCorrelation.h"
#include "winget/Manifest.h"
#include "winget/RepositorySearch.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;

namespace AppInstaller::Repository::Correlation
{
    namespace
    {
        struct PackageScore
        {
            std::shared_ptr<IPackageVersion> Package;
            double Score;

            PackageScore(
                const ARPCorrelationMeasure& measure,
                const Manifest::Manifest& manifest,
                std::shared_ptr<IPackageVersion> package)
                : Package(package), Score(measure.GetMatchingScore(manifest, package)) {}

            bool operator<(const PackageScore& other)
            {
                return Score < other.Score;
            }
        };
    }

    std::shared_ptr<IPackageVersion> ARPCorrelationMeasure::GetBestMatchForManifest(
        const Manifest::Manifest& manifest,
        const std::vector<ARPEntry>& arpEntries) const
    {
        AICLI_LOG(Repo, Verbose, << "Looking for best match in ARP for manifest " << manifest.Id);

        std::shared_ptr<IPackageVersion> bestMatch;
        double bestScore = std::numeric_limits<double>::lowest();

        for (const auto& arpEntry : arpEntries)
        {
            auto score = GetMatchingScore(manifest, arpEntry.Entry);
            AICLI_LOG(Repo, Verbose, << "Match score for " << arpEntry.Entry->GetProperty(PackageVersionProperty::Id) << ": " << score);

            if (score < GetMatchingThreshold())
            {
                AICLI_LOG(Repo, Verbose, << "Score is lower than threshold");
                continue;
            }

            if (!bestMatch || bestScore < score)
            {
                bestMatch = arpEntry.Entry;
                bestScore = score;
            }
        }

        if (bestMatch)
        {
            AICLI_LOG(Repo, Verbose, << "Best match is " << bestMatch->GetProperty(PackageVersionProperty::Id));
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
        std::shared_ptr<IPackageVersion> arpEntry) const
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
        std::shared_ptr<IPackageVersion> arpEntry) const
    {
        UNREFERENCED_PARAMETER(manifest);
        UNREFERENCED_PARAMETER(arpEntry);
        return 0;
    }

    double NormalizedNameAndPublisherCorrelation::GetMatchingThreshold() const
    {
        return 1;
    }
}