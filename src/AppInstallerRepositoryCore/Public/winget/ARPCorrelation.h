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
        struct IPackage;
        struct IPackageVersion;
        struct Source;
    }
}

namespace AppInstaller::Repository::Correlation
{
    // Contains the { Id, Version, Channel }
    using ARPEntrySnapshot = std::tuple<Utility::LocIndString, Utility::LocIndString, Utility::LocIndString>;

    // Struct holding all the data from an ARP entry we use for the correlation
    struct ARPEntry
    {
        ARPEntry(std::shared_ptr<AppInstaller::Repository::IPackage> entry, bool isNewOrUpdated) : Entry(entry), IsNewOrUpdated(isNewOrUpdated) {}

        // Data found in the ARP entry
        std::shared_ptr<AppInstaller::Repository::IPackage> Entry;

        // Whether this entry changed with the current installation
        bool IsNewOrUpdated;
    };

    struct ARPCorrelationResult
    {
        // Correlated package from ARP
        std::shared_ptr<AppInstaller::Repository::IPackageVersion> Package{};
        // Number of ARP entries that are new or updated
        size_t ChangesToARP{};
        // Number of ARP entries that match with the installed package
        size_t MatchesInARP{};
        // Number of changed ARP entries that match the installed package
        size_t CountOfIntersectionOfChangesAndMatches{};
    };

    struct IARPMatchConfidenceAlgorithm
    {
        virtual ~IARPMatchConfidenceAlgorithm() = default;
        virtual void Init(const AppInstaller::Manifest::Manifest& manifest) = 0;
        virtual double ComputeConfidence(const ARPEntry& arpEntry) const = 0;

        // Returns an instance of the algorithm we will actually use.
        // We may use multiple instances/specializations for testing and experimentation.
        static IARPMatchConfidenceAlgorithm& Instance();

#ifndef AICLI_DISABLE_TEST_HOOKS
        static void OverrideInstance(IARPMatchConfidenceAlgorithm* algorithmOverride);
        static void ResetInstance();
#endif
    };

    // Finds the ARP entry in the ARP source that matches a newly installed package.
    // Takes the package manifest, a snapshot of the ARP before the installation, and the current ARP source.
    // Returns the entry in the ARP source, or nullptr if there was no match, plus some stats about the correlation.
    ARPCorrelationResult FindARPEntryForNewlyInstalledPackage(
        const AppInstaller::Manifest::Manifest& manifest,
        const std::vector<ARPEntrySnapshot>& arpSnapshot,
        AppInstaller::Repository::Source& arpSource);

    std::shared_ptr<AppInstaller::Repository::IPackageVersion> FindARPEntryForNewlyInstalledPackageWithHeuristics(
        const AppInstaller::Manifest::Manifest& manifest,
        const std::vector<ARPEntry>& arpEntries);

    std::shared_ptr<AppInstaller::Repository::IPackageVersion> FindARPEntryForNewlyInstalledPackageWithHeuristics(
        const AppInstaller::Manifest::Manifest& manifest,
        const std::vector<ARPEntry>& arpEntries,
        IARPMatchConfidenceAlgorithm& algorithm);
}