// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Common.h"
#include "MsixInstallerHandler.h"
#include <AppInstallerDeployment.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Management::Deployment;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

namespace AppInstaller::Workflow
{
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

        Uri target = m_useStreaming ? Uri(Utility::ConvertToUTF16(m_manifestInstallerRef.Url)) : Uri(m_downloadedInstaller.c_str());

        auto installTask = ExecuteInstallerAsync(target);
        installTask.SetProgressReceiver(&m_reporterRef);

        m_reporterRef.ShowMsg(WorkflowReporter::Level::Info, "Starting package install...");
        installTask.Get();
        m_reporterRef.ShowMsg(WorkflowReporter::Level::Info, "Successfully installed.");
    }

    Future<void> MsixInstallerHandler::ExecuteInstallerAsync(const winrt::Windows::Foundation::Uri& uri)
    {
        DeploymentOptions deploymentOptions =
            DeploymentOptions::ForceApplicationShutdown |
            DeploymentOptions::ForceTargetApplicationShutdown;
        return Deployment::RequestAddPackageAsync(uri, deploymentOptions);
    }
}