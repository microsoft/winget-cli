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
            // Signature hash not provided. Go with download flow.
            InstallerHandlerBase::Download();
            m_useStreaming = false;
        }
        else
        {
            // Signature hash provided. No download needed. Just verify signature hash.
            Msix::MsixInfo msixInfo(m_manifestInstallerRef.Url);
            auto signature = msixInfo.GetSignature();

            SHA256::HashBuffer signatureHash;
            SHA256::ComputeHash(signature.data(), static_cast<uint32_t>(signature.size()), signatureHash);

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
        if (!m_useStreaming && m_downloadedInstaller.empty())
        {
            THROW_EXCEPTION_MSG(WorkflowException(APPINSTALLER_CLI_ERROR_INSTALLFLOW_FAILED), "Installer not downloaded yet");
        }

        auto installTask = ExecuteInstallerAsync(
            m_useStreaming ? Uri(Utility::ConvertToUTF16(m_manifestInstallerRef.Url)) : Uri(m_downloadedInstaller.c_str()));

        installTask.get();
    }

    std::future<void> MsixInstallerHandler::ExecuteInstallerAsync(const Uri& uri)
    {
        PackageManager packageManager;
        DeploymentOptions deploymentOptions =
            DeploymentOptions::ForceApplicationShutdown |
            DeploymentOptions::ForceTargetApplicationShutdown;

        m_reporterRef.ShowMsg(WorkflowReporter::Level::Info, "Starting package install...");
        m_reporterRef.ShowProgress(true, 0);

        // RequestAddPackageAsync will invoke smart screen.
        auto deployOperation = packageManager.RequestAddPackageAsync(
            uri,
            nullptr, /*dependencyPackageUris*/
            deploymentOptions,
            nullptr, /*targetVolume*/
            nullptr, /*optionalAndRelatedPackageFamilyNames*/
            nullptr /*relatedPackageUris*/);

        AsyncOperationProgressHandler<DeploymentResult, DeploymentProgress> progressCallback(
            [this](const IAsyncOperationWithProgress<DeploymentResult, DeploymentProgress>&, DeploymentProgress progress)
            {
                // Todo: might need to tweak progress reporting logic to account
                // for the time before DeploymentRequest is dequeued.
                m_reporterRef.ShowProgress(true, progress.percentage);
            }
        );

        // Set progress callback.
        deployOperation.Progress(progressCallback);

        co_await deployOperation;

        auto deployResult = deployOperation.GetResults();

        m_reporterRef.ShowProgress(false, 0);

        if (!SUCCEEDED(deployResult.ExtendedErrorCode()))
        {
            m_reporterRef.ShowMsg(WorkflowReporter::Level::Error, "Install failed. Reason: " + Utility::ConvertToUTF8(deployResult.ErrorText()));

            THROW_EXCEPTION_MSG(WorkflowException(APPINSTALLER_CLI_ERROR_INSTALLFLOW_FAILED),
                "Install failed. Installer task returned: %u", deployResult.ExtendedErrorCode());
        }

        m_reporterRef.ShowMsg(WorkflowReporter::Level::Info, "Successfully installed.");
    }
}