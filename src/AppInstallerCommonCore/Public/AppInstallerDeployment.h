// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerProgress.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Management.Deployment.h>

namespace AppInstaller::Deployment
{
    // Calls winrt::Windows::Management::Deployment::PackageManager::AddPackageAsync if skipSmartScreen is true,
    // Otherwise, calls winrt::Windows::Management::Deployment::PackageManager::RequestAddPackageAsync
    void AddPackage(
        const winrt::Windows::Foundation::Uri& uri,
        winrt::Windows::Management::Deployment::DeploymentOptions options,
        bool skipSmartScreen,
        IProgressCallback& callback);

    // Calls winrt::Windows::Management::Deployment::PackageManager::AddPackageAsync if skipSmartScreen is true,
    // Otherwise, calls winrt::Windows::Management::Deployment::PackageManager::RequestAddPackageAsync.
    // If the Add function fails due to the package being in use, we fall back to stage and register, which allows
    // a deferred registration.
    // Returns true if the registration was deferred; false if not.
    bool AddPackageWithDeferredFallback(
        std::string_view uri,
        bool skipSmartScreen,
        IProgressCallback& callback);

    // Calls winrt::Windows::Management::Deployment::PackageManager::RemovePackageAsync
    void RemovePackage(
        std::string_view packageFullName,
        winrt::Windows::Management::Deployment::RemovalOptions options,
        IProgressCallback& callback);

    // Calls winrt::Windows::Management::Deployment::PackageManager::StagePackageAsync
    //       winrt::Windows::Management::Deployment::PackageManager::ProvisionPackageForAllUsersAsync
    //       winrt::Windows::Management::Deployment::PackageManager::RegisterPackageByFullNameAsync if not running as system
    bool AddPackageMachineScope(
        std::string_view uri,
        IProgressCallback& callback);

    // Calls winrt::Windows::Management::Deployment::PackageManager::DeprovisionPackageForAllUsersAsync
    //       winrt::Windows::Management::Deployment::PackageManager::RemovePackageAsync with RemoveForAllUsers
    void RemovePackageMachineScope(
        std::string_view packageFamilyName,
        std::string_view packageFullName,
        IProgressCallback& callback);

    // Calls winrt::Windows::Management::Deployment::PackageManager::FindPackagesForUser
    bool IsRegistered(std::string_view packageFamilyName);

    // Calls winrt::Windows::Management::Deployment::PackageManager::RegisterPackageByFamilyNameAsync
    void RegisterPackage(
        std::string_view packageFamilyName,
        IProgressCallback& callback);
}
