// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerProgress.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Management.Deployment.h>

namespace AppInstaller::Deployment
{
    // Calls winrt::Windows::Management::Deployment::PackageManager::RequestAddPackageAsync
    void RequestAddPackage(
        const winrt::Windows::Foundation::Uri& uri, 
        winrt::Windows::Management::Deployment::DeploymentOptions options,
        IProgressCallback& callback);

    // Calls winrt::Windows::Management::Deployment::PackageManager::RemovePackageAsync
    void RemovePackage(
        std::string_view packageFullName,
        IProgressCallback& callback);
}
