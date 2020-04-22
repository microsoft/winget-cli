// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerProgress.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Management.Deployment.h>

namespace AppInstaller::Deployment
{
    // Calls winrt::Windows::Management::Deployment::PackageManager::RequestAddPackageAsync
    void RequestAddPackageAsync(
        const winrt::Windows::Foundation::Uri& uri, 
        winrt::Windows::Management::Deployment::DeploymentOptions options,
        IProgressCallback& callback);

    // Calls winrt::Windows::Management::Deployment::PackageManager::RemovePackageAsync,
    // but *DOES NOT WAIT FOR A RESULT*.  As this is used for removing an optional package
    // we will simply complete our actions and exit the process.
    void RemovePackageFireAndForget(std::string_view packageFullName);
}
