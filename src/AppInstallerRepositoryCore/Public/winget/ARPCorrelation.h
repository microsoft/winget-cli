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
        struct Source;
    }
}

namespace AppInstaller::Repository::Correlation
{
    // Contains the { Id, Version, Channel }
    // TODO: Use this in the Context. Not doing it yet to avoid having to recompile the whole CLICore project each time
    using ARPEntrySnapshot = std::tuple<Utility::LocIndString, Utility::LocIndString, Utility::LocIndString>;

    // Struct holding all the data from an ARP entry we use for the correlation
    struct ARPEntry
    {
        ARPEntry(std::shared_ptr<AppInstaller::Repository::IPackageVersion> entry, bool isNewOrUpdated) : Entry(entry), IsNewOrUpdated(isNewOrUpdated) {}

        // Data found in the ARP entry
        std::shared_ptr<AppInstaller::Repository::IPackageVersion> Entry;

        // Whether this entry changed with the current installation
        bool IsNewOrUpdated;
    };

    // An algorithm for correlating a package with an ARP entry.
    // This is the overall algorithm that considers everything: name matching, versions,
    // how to mix multiple data points, threshold for matching.
    // TODO: These may benefit from being instantiated with the single manifest
    //       we are looking for, instead of passing it as an argument each time.
    struct ARPCorrelationAlgorithm
    {
        virtual ~ARPCorrelationAlgorithm() = default;

        // Computes a matching score between a package manifest and a manifest entry.
        // A higher score indicates a more certain match.
        // The score should be in the range [0, 1].
        virtual double GetMatchingScore(
            const AppInstaller::Manifest::Manifest& manifest,
            const AppInstaller::Manifest::ManifestLocalization& manifestLocalization,
            const ARPEntry& arpEntry) const = 0;

        // Computes a matching score between a package manifest and a manifest entry.
        // This takes the highest score from all the available localizations.
        double GetMatchingScore(
            const AppInstaller::Manifest::Manifest& manifest,
            const ARPEntry& arpEntry) const;

        // Gets the ARP entry that has the best correlation score for a given manifest.
        // If no entry has a good enough match, returns null.
        // This will choose a single package even if multiple are good matches.
        std::optional<ARPEntry> GetBestMatchForManifest(
            const AppInstaller::Manifest::Manifest& manifest,
            const std::vector<ARPEntry>& arpEntries) const;

        // Returns an instance of the algorithm we will actually use.
        // We may use multiple instances/specializations for testing and experimentation.
        static const ARPCorrelationAlgorithm& GetInstance();
    };

    // An algorithm for measuring the match in name and publisher between a package
    // and an ARP entry
    struct NameAndPublisherCorrelationMeasure
    {
        virtual ~NameAndPublisherCorrelationMeasure() = default;

        // Computes a score between 0 and 1 indicating how certain we are that
        // the two pairs of name+publisher represent the same app.
        virtual double GetMatchingScore(
            std::string_view packageName,
            std::string_view packagePublisher,
            std::string_view arpName,
            std::string_view arpPublisher) const = 0;
    };

    // Empty correlation measure that always returns 0.
    // This is used only as a benchmark to compare other measures to.
    struct EmptyNameAndPublisherCorrelationMeasure : public NameAndPublisherCorrelationMeasure
    {
        double GetMatchingScore(std::string_view packageName, std::string_view packagePublisher, std::string_view arpName, std::string_view arpPublisher) const override;
    };

    // Measures the correlation with an exact match using normalized name and publisher.
    // This is used only as a benchmark to compare other measures to, as the actual correlation
    // algorithm can do this with a search of the ARP source.
    struct NormalizedNameAndPublisherCorrelationMeasure : public NameAndPublisherCorrelationMeasure
    {
        double GetMatchingScore(std::string_view packageName, std::string_view packagePublisher, std::string_view arpName, std::string_view arpPublisher) const override;
    };

    // Measures the correlation using the edit distance between the strings.
    struct EditDistanceNameAndPublisherCorrelationMeasure : public NameAndPublisherCorrelationMeasure
    {
        double GetMatchingScore(std::string_view packageName, std::string_view packagePublisher, std::string_view arpName, std::string_view arpPublisher) const override;
    };

    struct EditDistanceNormalizedNameAndPublisherCorrelationMeasure : public NameAndPublisherCorrelationMeasure
    {
        double GetMatchingScore(std::string_view packageName, std::string_view packagePublisher, std::string_view arpName, std::string_view arpPublisher) const override;
    };

    // Finds the ARP entry in the ARP source that matches a newly installed package.
    // Takes the package manifest, a snapshot of the ARP before the installation, and the current ARP source.
    // Returns the entry in the ARP source, or nullptr if there was no match.
    std::shared_ptr<AppInstaller::Repository::IPackageVersion> FindARPEntryForNewlyInstalledPackage(
        const AppInstaller::Manifest::Manifest& manifest,
        const std::vector<ARPEntrySnapshot>& arpSnapshot,
        AppInstaller::Repository::Source& arpSource,
        std::string_view sourceIdentifier);

    std::shared_ptr<AppInstaller::Repository::IPackageVersion> FindARPEntryForNewlyInstalledPackageWithHeuristics(
        const AppInstaller::Manifest::Manifest& manifest,
        const std::vector<ARPEntrySnapshot>& arpSnapshot,
        AppInstaller::Repository::Source& arpSource);
}