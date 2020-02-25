// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerFuture.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Management.Deployment.h>

namespace AppInstaller::Deployment
{
    // Calls winrt::Windows::Management::Deployment::PackageManager::RequestAddPackageAsync as a Future.
    Future<void> RequestAddPackageAsync(
        const winrt::Windows::Foundation::Uri& uri, 
        winrt::Windows::Management::Deployment::DeploymentOptions options);
}
