// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/RepositorySearch.h>


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
}
