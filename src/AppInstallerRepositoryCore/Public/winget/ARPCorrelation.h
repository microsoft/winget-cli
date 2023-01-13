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

    // One of the possible options that could be chosen for correlation.
    struct CorrelationMeasure
    {
        // The value that the correlation algorithm assigned to the match with the package.
        double Measure{};

        // The package that was measured.
        std::shared_ptr<AppInstaller::Repository::IPackageVersion> Package{};
    };

    // The result of a heuristics correlation attempt.
    struct ARPHeuristicsCorrelationResult
    {
        // Correlated package from ARP
        std::shared_ptr<AppInstaller::Repository::IPackageVersion> Package{};

        // The reason for the correlation (for diagnostics).
        std::string Reason;

        // The correlation metrics and their associated ARP package information (for diagnostics).
        std::vector<CorrelationMeasure> Measures;
    };

    // The result of a correlation attempt.
    struct ARPCorrelationResult : public ARPHeuristicsCorrelationResult
    {
        // Number of ARP entries that are new or updated
        size_t ChangesToARP{};

        // Number of ARP entries that match with the installed package
        size_t MatchesInARP{};

        // Number of changed ARP entries that match the installed package
        size_t CountOfIntersectionOfChangesAndMatches{};

        ARPCorrelationResult& operator=(ARPHeuristicsCorrelationResult&& other)
        {
            *static_cast<ARPHeuristicsCorrelationResult*>(this) = std::move(other);
            return *this;
        }
    };

    // Allows callers finer control over how the correlation result will be chosen.
    // The values appear in order of their application in the correlation algorithm, meaning that a later
    // setting that is set to true can be preempted by an earlier setting, if a correlation occurs with the
    // earlier setting.
    // The default values are chosen to reflect what is used after an install on a consumer system.
    struct ARPCorrelationSettings
    {
        // This setting controls whether the name and publisher normalization algorithm will be used for correlation.
        // When true, normalization will be the first choice for correlation. This means that a normalized name+publisher
        // match will result in correlation (unless there are multiple matches).
        // When false, normalization will only be used for the statistics (MatchesInARP), but the correlation result package
        // will not be based on normalization.
        bool AllowNormalization = true;

        // This settings controls whether a single changed ARP entry is sufficient to result in correlation.
        // When true, if only a single ARP entry is detected as new or changed, it will be chosen as the correlated result.
        bool AllowSingleChange = false;
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

    ARPHeuristicsCorrelationResult FindARPEntryForNewlyInstalledPackageWithHeuristics(
        const AppInstaller::Manifest::Manifest& manifest,
        const std::vector<ARPEntry>& arpEntries);

    ARPHeuristicsCorrelationResult FindARPEntryForNewlyInstalledPackageWithHeuristics(
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
        virtual ARPCorrelationResult CorrelateForNewlyInstalled(const Manifest::Manifest& manifest, const ARPCorrelationSettings& settings = {});

        const std::vector<ARPEntrySnapshot>& GetPreInstallSnapshot() const { return m_preInstallSnapshot; }

    private:
        std::vector<ARPEntrySnapshot> m_preInstallSnapshot;

        Source m_postInstallSnapshotSource;
        std::vector<Correlation::ARPEntry> m_postInstallSnapshot;
    };
}
