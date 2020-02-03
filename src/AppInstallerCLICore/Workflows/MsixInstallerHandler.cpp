// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "Common.h"
#include "MsixInstallerHandler.h"

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Management::Deployment;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

namespace AppInstaller::Workflow
{
    MsixInstallerHandler::MsixInstallerHandler(
        const Manifest::ManifestInstaller& manifestInstaller,
        WorkflowReporter& reporter) :
        InstallerHandlerBase(manifestInstaller, reporter)
    {
        if (manifestInstaller.InstallerType != ManifestInstaller::InstallerTypeEnum::Msix)
        {
            THROW_HR_MSG(E_UNEXPECTED, "Installer type not supported.");
        }
    }

    void MsixInstallerHandler::Download()
    {
        if (m_manifestInstallerRef.SignatureSha256.empty())
        {
            InstallerHandlerBase::Download();
            m_useStreaming = false;
        }
        else
        {
            auto msixInfo = Msix::MsixInfo::CreateMsixInfo(m_manifestInstallerRef.Url);
            auto signature = msixInfo->GetSignature();

            SHA256::HashBuffer signatureHash;
            SHA256::ComputeHash(signature.data(), signature.size(), signatureHash);

            if (!std::equal(
                m_manifestInstallerRef.SignatureSha256.begin(),
                m_manifestInstallerRef.SignatureSha256.end(),
                signatureHash.begin()))
            {
                AICLI_LOG(CLI, Error,
                    << "Package hash verification failed. Signature SHA256 in manifest: "
                    << SHA256::ConvertToString(m_manifestInstallerRef.SignatureSha256)
                    << "Signature SHA256 from download: "
                    << SHA256::ConvertToString(signatureHash));

                if (!m_reporterRef.PromptForBoolResponse(WorkflowReporter::Level::Warning, "Package hash verification failed. Continue?"))
                {
                    m_reporterRef.ShowMsg(WorkflowReporter::Level::Error, "Canceled. Package hash mismatch.");
                    THROW_EXCEPTION_MSG(WorkflowException(APPINSTALLER_CLI_ERROR_INSTALLFLOW_FAILED), "Package installation canceled");
                }
            }
            else
            {
                AICLI_LOG(CLI, Info, << "Msix package signature hash verified");
                m_reporterRef.ShowMsg(WorkflowReporter::Level::Info, "Successfully verified SHA256.");
            }

            m_useStreaming = true;
        }
    }

    void MsixInstallerHandler::Install()
    {
        auto installTask = ExecuteInstallerAsync();

        auto installTaskResult = installTask.get();

        m_reporterRef.ShowProgress(false, 0);

        if (SUCCEEDED(installTaskResult.ExtendedErrorCode()))
        {
            m_reporterRef.ShowMsg(WorkflowReporter::Level::Info, "Install succeeded.");
        }
        else
        {
            m_reporterRef.ShowMsg(WorkflowReporter::Level::Error, "Install failed. Reason: " + Utility::ConvertToUTF8(installTaskResult.ErrorText()));
        }
    }

    std::future<IDeploymentResult> MsixInstallerHandler::ExecuteInstallerAsync()
    {
        PackageManager packageManager;
        DeploymentOptions deploymentOptions =
            DeploymentOptions::ForceApplicationShutdown |
            DeploymentOptions::ForceTargetApplicationShutdown;

        std::atomic<bool> done = false;

        m_reporterRef.ShowMsg(WorkflowReporter::Level::Info, "Starting package install...");
        m_reporterRef.ShowProgress(true, 0);

        auto opration = packageManager.RequestAddPackageAsync(
            m_useStreaming ? Uri(Utility::ConvertToUTF16(m_manifestInstallerRef.Url)) : Uri(m_downloadedInstaller.c_str()),
            nullptr, /*dependencyPackageUris*/
            deploymentOptions,
            nullptr, /*targetVolume*/
            nullptr, /*optionalAndRelatedPackageFamilyNames*/
            nullptr /*relatedPackageUris*/);

        AsyncOperationProgressHandler<DeploymentResult, DeploymentProgress> progressCallback(
            [this](const IAsyncOperationWithProgress<DeploymentResult, DeploymentProgress>& op, DeploymentProgress progress)
            {
                m_reporterRef.ShowProgress(true, progress.percentage);
            }
        );

        opration.Progress(progressCallback);

        co_await opration;

        co_return std::move(opration.GetResults());
    }
}