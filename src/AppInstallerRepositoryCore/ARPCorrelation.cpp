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
            std::shared_ptr<IPackage> Package;
            double Score;

            PackageScore(
                const ARPCorrelationMeasure& measure,
                const Manifest::Manifest& manifest,
                std::shared_ptr<IPackage> package)
                : Package(package), Score(measure.GetMatchingScore(manifest, package)) {}

            bool operator<(const PackageScore& other)
            {
                return Score < other.Score;
            }
        };
    }

    std::shared_ptr<IPackage> ARPCorrelationMeasure::GetBestMatchForManifest(
        const Manifest::Manifest& manifest,
        const SearchResult& packages) const
    {
        AICLI_LOG(Repo, Verbose, << "Looking for best match in ARP for manifest " << manifest.Id);

        std::shared_ptr<IPackage> bestMatch;
        double bestScore = std::numeric_limits<double>::lowest();

        for (const auto& searchMatch : packages.Matches)
        {
            auto score = GetMatchingScore(manifest, searchMatch.Package);
            AICLI_LOG(Repo, Verbose, << "Match score for " << searchMatch.Package->GetProperty(PackageProperty::Id) << ": " << score);

            if (score < GetMatchingThreshold())
            {
                AICLI_LOG(Repo, Verbose, << "Score is lower than threshold");
                continue;
            }

            if (!bestMatch || bestScore < score)
            {
                bestMatch = searchMatch.Package;
                bestScore = score;
            }
        }

        if (bestMatch)
        {
            AICLI_LOG(Repo, Verbose, << "Best match is " << bestMatch->GetProperty(PackageProperty::Id));
        }
        else
        {
            AICLI_LOG(Repo, Verbose, << "No ARP entry had a correlation score surpassing the required threshold");
        }

        return bestMatch;
    }

    const ARPCorrelationMeasure& ARPCorrelationMeasure::GetInstance()
    {
        static NoMatch instance;
        return instance;
    }


    double NoMatch::GetMatchingScore(
        const Manifest::Manifest& manifest,
        std::shared_ptr<IPackage> arpEntry) const
    {
        return 0;
    }

    double NoMatch::GetMatchingThreshold() const
    {
        return 1;
    }
}