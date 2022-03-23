// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace AppInstaller
{
    namespace Manifest
    {
        struct Manifest;
    }

    namespace Repository
    {
        struct IPackage;
        struct SearchResult;
    }
}

namespace AppInstaller::Repository::Correlation
{

    // An algorithm for correlating a package with an ARP entry. 
    struct ARPCorrelationMeasure
    {
        virtual ~ARPCorrelationMeasure() = default;

        // Computes a matching score between a package manifest and a manifest entry.
        // A higher score indicates a more certain match.
        // The possible range of values is determined by the algorithm.
        virtual double GetMatchingScore(
            const AppInstaller::Manifest::Manifest& manifest,
            std::shared_ptr<AppInstaller::Repository::IPackage> arpEntry) const = 0;

        // Gets the minimum score needed by this algorithm for something to be considered a match.
        virtual double GetMatchingThreshold() const = 0;

        // Gets the package that has the best correlation score for a given manifest.
        // If no package has a good enough match, returns null.
        // This will choose a single package even if multiple are good matches.
        std::shared_ptr<IPackage> GetBestMatchForManifest(
            const AppInstaller::Manifest::Manifest& manifest, 
            const AppInstaller::Repository::SearchResult& packages) const;

        // Returns an instance of the measure we will actually use.
        static const ARPCorrelationMeasure& GetInstance();
    };

#define DEFINE_CORRELATION_ALGORITHM(_name_) \
    struct _name_ : public ARPCorrelationMeasure \
    { \
        double GetMatchingScore(const Manifest::Manifest& manifest, std::shared_ptr<IPackage> arpEntry) const override; \
        double GetMatchingThreshold() const override; \
    }

    // We define multiple algorithms to compare how good their results are,
    // but we will only use one in the product.

    // No correlation between packages and ARP entries
    DEFINE_CORRELATION_ALGORITHM(NoMatch);
}