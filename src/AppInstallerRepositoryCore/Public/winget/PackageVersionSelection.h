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
}
