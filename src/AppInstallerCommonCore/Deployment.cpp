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
            AsyncOperationProgressHandler<DeploymentResult, DeploymentProgress> progressCallback(
                [&callback](const IAsyncOperationWithProgress<DeploymentResult, DeploymentProgress>&, DeploymentProgress progress)
                {
                    callback.OnProgress(progress.percentage, 100, ProgressType::Percent);
                }
            );

            // Set progress callback.
            deployOperation.Progress(progressCallback);

            auto removeCancel = callback.SetCancellationFunction([&]() { deployOperation.Cancel(); });
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

        // Type that exists simply to enabled a fire and forget register call as we exit.
        struct DelayRegisterStorage
        {
            DelayRegisterStorage() = default;

            ~DelayRegisterStorage()
            {
                if (!m_familyNames.empty())
                {
                    PackageManager packageManager;

                    for (const auto& fn : m_familyNames)
                    {
                        size_t id = GetDeploymentOperationId();
                        AICLI_LOG(Core, Info, << "Starting RegisterPackageByFamilyName operation #" << id << ": " << fn);

                        winrt::hstring familyName = Utility::ConvertToUTF16(fn).c_str();
                        (void)packageManager.RegisterPackageByFamilyNameAsync(
                            familyName,
                            nullptr,
                            winrt::Windows::Management::Deployment::DeploymentOptions::None,
                            nullptr,
                            nullptr);
                    }
                }
            }

            void Add(std::string_view familyName)
            {
                m_familyNames.emplace_back(familyName);
            }

        private:
            std::vector<std::string> m_familyNames;
        };

        DelayRegisterStorage s_delayRegisterStorage;
    }

    void RequestAddPackageAsync(
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

    void StageAndDelayRegisterPackageAsync(
        std::string_view packageFamilyName,
        const winrt::Windows::Foundation::Uri& uri,
        winrt::Windows::Management::Deployment::DeploymentOptions stageOptions,
        winrt::Windows::Management::Deployment::DeploymentOptions,
        IProgressCallback& callback)
    {
        size_t id = GetDeploymentOperationId();
        AICLI_LOG(Core, Info, << "Starting StagePackage operation #" << id << ": " << Utility::ConvertToUTF8(uri.AbsoluteUri().c_str()));

        PackageManager packageManager;

        // RequestAddPackageAsync will invoke smart screen.
        auto deployOperation = packageManager.StagePackageAsync(
            uri,
            nullptr, /*dependencyPackageUris*/
            stageOptions,
            nullptr, /*targetVolume*/
            nullptr, /*optionalAndRelatedPackageFamilyNames*/
            nullptr /*relatedPackageUris*/);

        WaitForDeployment(deployOperation, id, callback);

        s_delayRegisterStorage.Add(packageFamilyName);
    }

    void RemovePackageFireAndForget(std::string_view packageFullName)
    {
        PackageManager packageManager;
        winrt::hstring fullName = Utility::ConvertToUTF16(packageFullName).c_str();
        (void)packageManager.RemovePackageAsync(fullName, RemovalOptions::None);
    }
}
