// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace AppInstaller
{
    namespace Manifest
    {
        struct Manifest;
        struct ManifestLocalization;
    }

    namespace Repository
    {
        struct IPackageVersion;
        struct SearchResult;
    }
}

namespace AppInstaller::Repository::Correlation
{
    struct ARPEntry
    {
        ARPEntry(std::shared_ptr<AppInstaller::Repository::IPackageVersion> entry, bool isNewOrUpdated) : Entry(entry), IsNewOrUpdated(isNewOrUpdated) {}

        // Data found in the ARP entry
        std::shared_ptr<AppInstaller::Repository::IPackageVersion> Entry;

        // Whether this entry changed with the current installation
        bool IsNewOrUpdated;

        // TODO: Use data from package; this is a hack for testing...
        std::string Name, Publisher;
    };

    // An algorithm for correlating a package with an ARP entry. 
    struct ARPCorrelationMeasure
    {
        virtual ~ARPCorrelationMeasure() = default;

        // Computes a matching score between a package manifest and a manifest entry.
        // A higher score indicates a more certain match.
        // The possible range of values is determined by the algorithm.
        // Note: This should ideally use all manifest localizations
        virtual double GetMatchingScore(
            const AppInstaller::Manifest::Manifest& manifest,
            const AppInstaller::Manifest::ManifestLocalization& manifestLocalization,
            const ARPEntry& arpEntry) const = 0;

        double GetMatchingScore(
            const AppInstaller::Manifest::Manifest& manifest,
            const ARPEntry& arpEntry) const;

        // Gets the minimum score needed by this algorithm for something to be considered a match.
        virtual double GetMatchingThreshold() const = 0;

        // Gets the package that has the best correlation score for a given manifest.
        // If no package has a good enough match, returns null.
        // This will choose a single package even if multiple are good matches.
        std::optional<ARPEntry> GetBestMatchForManifest(
            const AppInstaller::Manifest::Manifest& manifest, 
            const std::vector<ARPEntry>& arpEntries) const;

        // Returns an instance of the measure we will actually use.
        static const ARPCorrelationMeasure& GetInstance();
    };

#define DEFINE_CORRELATION_ALGORITHM(_name_) \
    struct _name_ : public ARPCorrelationMeasure \
    { \
        double GetMatchingScore( \
            const AppInstaller::Manifest::Manifest& manifest, \
            const AppInstaller::Manifest::ManifestLocalization& manifestLocalization, \
            const ARPEntry& arpEntry) const; \
        double GetMatchingThreshold() const override; \
    }

    // We define multiple algorithms to compare how good their results are,
    // but we will only use one in the product.

    DEFINE_CORRELATION_ALGORITHM(NoCorrelation);
    DEFINE_CORRELATION_ALGORITHM(NormalizedNameAndPublisherCorrelation);
    DEFINE_CORRELATION_ALGORITHM(NormalizedEditDistanceCorrelation);
}