// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Public/AppInstallerDeployment.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::Deployment
{
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Windows::Management::Deployment;

    namespace
    {
        size_t GetDeploymentOperationId()
        {
            static std::atomic_size_t s_deploymentId = 0;
            return s_deploymentId.fetch_add(1);
        }

        void WaitForDeployment(
            IAsyncOperationWithProgress<DeploymentResult, DeploymentProgress>& deployOperation,
            size_t id,
            IProgressCallback& callback)
        {
            AICLI_LOG(Core, Info, << "Begin waiting for deployment #" << id);

            AsyncOperationProgressHandler<DeploymentResult, DeploymentProgress> progressCallback(
                [&callback](const IAsyncOperationWithProgress<DeploymentResult, DeploymentProgress>&, DeploymentProgress progress)
                {
                    callback.OnProgress(progress.percentage, 100, ProgressType::Percent);
                }
            );

            // Set progress callback.
            deployOperation.Progress(progressCallback);

            auto removeCancel = callback.SetCancellationFunction([&]() { deployOperation.Cancel(); });

            AICLI_LOG(Core, Info, << "Begin blocking for deployment #" << id);

            auto deployResult = deployOperation.get();

            if (!SUCCEEDED(deployResult.ExtendedErrorCode()))
            {
                AICLI_LOG(Core, Error, << "Deployment failed #" << id << ": " << Utility::ConvertToUTF8(deployResult.ErrorText()));

                // Note that while the format string is char*, it gets converted to wchar before being used and thus %s needs a wchar.
                THROW_HR_MSG(deployResult.ExtendedErrorCode(), "Install failed: %s", deployResult.ErrorText().c_str());
            }
            else
            {
                AICLI_LOG(Core, Info, << "Successfully deployed #" << id);
            }
        }
    }

    void RequestAddPackage(
        const winrt::Windows::Foundation::Uri& uri,
        winrt::Windows::Management::Deployment::DeploymentOptions options,
        IProgressCallback& callback)
    {
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

        WaitForDeployment(deployOperation, id, callback);
    }

    void RemovePackage(
        std::string_view packageFullName,
        IProgressCallback& callback)
    {
        size_t id = GetDeploymentOperationId();
        AICLI_LOG(Core, Info, << "Starting RemovePackage operation #" << id << ": " << packageFullName);

        PackageManager packageManager;
        winrt::hstring fullName = Utility::ConvertToUTF16(packageFullName).c_str();
        auto deployOperation = packageManager.RemovePackageAsync(fullName, RemovalOptions::None);

        WaitForDeployment(deployOperation, id, callback);
    }
}
