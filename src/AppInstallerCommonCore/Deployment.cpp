// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Public/AppInstallerDeployment.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::Deployment
{
    namespace
    {
        size_t GetDeploymentOperationId()
        {
            static std::atomic_size_t s_deploymentId = 0;
            return s_deploymentId.fetch_add(1);
        }
    }

    void RequestAddPackageAsync(
        const winrt::Windows::Foundation::Uri& uri,
        winrt::Windows::Management::Deployment::DeploymentOptions options,
        IProgressCallback& callback)
    {
        using namespace winrt::Windows::Foundation;
        using namespace winrt::Windows::Management::Deployment;

        size_t id = GetDeploymentOperationId();
        AICLI_LOG(Core, Info, << "Starting RequestAddPackage operation #" << id << ": " << Utility::ConvertToUTF8(uri.AbsoluteUri().c_str()));

        PackageManager packageManager;

        // RequestAddPackageAsync will invoke smart screen.
        auto deployOperation = packageManager.RequestAddPackageAsync(
            uri,
            nullptr, /*dependencyPackageUris*/
            options,
            nullptr, /*targetVolume*/
            nullptr, /*optionalAndRelatedPackageFamilyNames*/
            nullptr /*relatedPackageUris*/);

        AsyncOperationProgressHandler<DeploymentResult, DeploymentProgress> progressCallback(
            [&callback](const IAsyncOperationWithProgress<DeploymentResult, DeploymentProgress>&, DeploymentProgress progress)
            {
                callback.OnProgress(progress.percentage, 100, ProgressType::Percent);
            }
        );

        // Set progress callback.
        deployOperation.Progress(progressCallback);

        auto deployResult = deployOperation.GetResults();

        if (!SUCCEEDED(deployResult.ExtendedErrorCode()))
        {
            AICLI_LOG(Core, Error, << "Deployment failed #" << id << ": " << Utility::ConvertToUTF8(deployResult.ErrorText()));

            THROW_HR_MSG(deployResult.ExtendedErrorCode(), "Install failed: %s", Utility::ConvertToUTF8(deployResult.ErrorText()).c_str());
        }
        else
        {
            AICLI_LOG(Core, Info, << "Successfully deployed #" << id);
        }
    }
}
