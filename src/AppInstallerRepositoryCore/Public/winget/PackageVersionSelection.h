// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/RepositorySearch.h>
#include <winget/ManifestComparator.h>


namespace AppInstaller::Repository
{
    // Gets an IPackageVersionCollection that represents the available package versions for the installed version.
    // If we have tracking data, will remove packages not from the tracked source. Will also remove versions that do not correspond to the tracked channel.
    // This function uses the latest installed version as a temporary convenience until side-by-side is implemented.
    std::shared_ptr<IPackageVersionCollection> GetAvailableVersionsForInstalledVersion(const std::shared_ptr<ICompositePackage>& composite);

    // Gets an IPackageVersionCollection that represents the available package versions for the given installed version.
    std::shared_ptr<IPackageVersionCollection> GetAvailableVersionsForInstalledVersion(
        const std::shared_ptr<ICompositePackage>& composite,
        const std::shared_ptr<IPackageVersion>& installedVersion);

    // Equivalent to `GetAvailableVersionsForInstalledVersion(composite, nullptr)` to make the intent more clear that the caller wants to ignore any installed
    // package information.
    std::shared_ptr<IPackageVersionCollection> GetAllAvailableVersions(const std::shared_ptr<ICompositePackage>& composite);

    // Gets the installed version, or a null if there isn't one.
    std::shared_ptr<IPackageVersion> GetInstalledVersion(const std::shared_ptr<ICompositePackage>& composite);

    // Gets the available IPackage corresponding to the given source identifier.
    std::shared_ptr<IPackage> GetAvailablePackageFromSource(const std::shared_ptr<ICompositePackage>& composite, const std::string_view sourceIdentifier);

    struct LatestApplicableVersionData
    {
        std::shared_ptr<AppInstaller::Repository::IPackageVersion> LatestApplicableVersion;
        bool UpdateAvailable = false;
    };

    // Determines the default install version and whether an update is available.
    LatestApplicableVersionData GetLatestApplicableVersion(const std::shared_ptr<ICompositePackage>& composite);

    // Fills the options from the given metadata, optionally including the allowed architectures.
    void GetManifestComparatorOptionsFromMetadata(AppInstaller::Manifest::ManifestComparator::Options& options, const IPackageVersion::Metadata& metadata, bool includeAllowedArchitectures = true);

    // Gets the source priority for a given composite package, taking into account installed relationships.
    std::optional<int32_t> GetSourcePriority(const std::shared_ptr<ICompositePackage>& composite);

    // Determines whether repairing the installed version uses an available version of the package.
    // MSI and MSIX packages not from the Microsoft Store are repaired by the platform without one.
    // The composite may be null, such as when repairing from a manifest.
    bool IsAvailableVersionRequiredForRepair(const std::shared_ptr<ICompositePackage>& composite, const std::shared_ptr<IPackageVersion>& installedVersion);

    // Gets the key of the available version used to repair the installed version.
    // The requested version, if not empty, is used in place of the installed version.
    // Returns nullopt if the installed version is unknown, as no available version can be mapped to it.
    std::optional<PackageVersionKey> GetRepairVersionKey(
        const std::shared_ptr<ICompositePackage>& composite,
        const std::shared_ptr<IPackageVersion>& installedVersion,
        std::string_view requestedVersion = {},
        std::string_view requestedChannel = {});

    // Gets the available version used to repair the installed version of the package, or null if there isn't one.
    std::shared_ptr<IPackageVersion> GetRepairVersion(const std::shared_ptr<ICompositePackage>& composite);
}
