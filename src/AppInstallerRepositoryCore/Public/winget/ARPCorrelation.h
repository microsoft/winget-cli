// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <winget/LocIndependent.h>
#include <winget/RepositorySource.h>

#include <memory>
#include <utility>
#include <vector>

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
        ARPEntry(std::shared_ptr<AppInstaller::Repository::IPackage> entry, bool isNewOrUpdated) : Entry(std::move(entry)), IsNewOrUpdated(isNewOrUpdated) {}

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

    std::shared_ptr<AppInstaller::Repository::IPackageVersion> FindARPEntryForNewlyInstalledPackageWithHeuristics(
        const AppInstaller::Manifest::Manifest& manifest,
        const std::vector<ARPEntry>& arpEntries);

    std::shared_ptr<AppInstaller::Repository::IPackageVersion> FindARPEntryForNewlyInstalledPackageWithHeuristics(
        const AppInstaller::Manifest::Manifest& manifest,
        const std::vector<ARPEntry>& arpEntries,
        IARPMatchConfidenceAlgorithm& algorithm);

    // Holds data needed for ARP correlation, as well as functions to run correlation on the collected data.
    struct ARPCorrelationData
    {
        ARPCorrelationData() = default;
        virtual ~ARPCorrelationData() = default;

        // Captures the ARP state before the package installation.
        void CapturePreInstallSnapshot();

        // Captures the ARP state differences after the package installation.
        void CapturePostInstallSnapshot();

        // Correlates the given manifest against the data previously collected with capture calls.
        virtual ARPCorrelationResult CorrelateForNewlyInstalled(const Manifest::Manifest& manifest);

        const std::vector<ARPEntrySnapshot>& GetPreInstallSnapshot() const { return m_preInstallSnapshot; }

    private:
        std::vector<ARPEntrySnapshot> m_preInstallSnapshot;

        Source m_postInstallSnapshotSource;
        std::vector<Correlation::ARPEntry> m_postInstallSnapshot;
    };
}
