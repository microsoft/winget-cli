// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Common.h"
#include "MsixInstallerHandler.h"
#include <AppInstallerDeployment.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Management::Deployment;
using namespace AppInstaller::CLI;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

namespace AppInstaller::Workflow
{
    void MsixInstallerHandler::Download()
    {
        m_useStreaming = false;
        std::optional<Msix::MsixInfo> msixInfo;

        if (!m_manifestInstallerRef.SignatureSha256.empty())
        {
            try
            {
                msixInfo = std::optional<Msix::MsixInfo>(std::in_place, m_manifestInstallerRef.Url);
                m_useStreaming = true;
            }
            catch (const winrt::hresult_error& e)
            {
                if (e.code() == HRESULT_FROM_WIN32(ERROR_NO_RANGES_PROCESSED))
                {
                    // Server does not support range request, use download
                    m_useStreaming = false;
                }
                else
                {
                    throw;
                }
            }
        }

        if (!m_useStreaming)
        {
            // Signature hash not provided. Go with download flow.
            InstallerHandlerBase::Download();
        }
        else
        {
            // Signature hash provided and host server supports streaming. Just verify signature hash.
            auto signature = msixInfo.value().GetSignature();
            auto signatureHash = SHA256::ComputeHash(signature.data(), static_cast<uint32_t>(signature.size()));

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

                if (!m_reporterRef.PromptForBoolResponse("Package hash verification failed. Continue?", Execution::Reporter::Level::Warning))
                {
                    m_reporterRef.ShowMsg("Canceled. Package hash mismatch.", Execution::Reporter::Level::Error);
                    THROW_EXCEPTION_MSG(WorkflowException(APPINSTALLER_CLI_ERROR_INSTALLFLOW_FAILED), "Package installation canceled");
                }
            }
            else
            {
                AICLI_LOG(CLI, Info, << "Msix package signature hash verified");
                m_reporterRef.ShowMsg("Successfully verified SHA256.");
            }
        }
    }

    void MsixInstallerHandler::Install()
    {
        if (!m_useStreaming && m_downloadedInstaller.empty())
        {
            THROW_EXCEPTION_MSG(WorkflowException(APPINSTALLER_CLI_ERROR_INSTALLFLOW_FAILED), "Installer not downloaded yet");
        }

        Uri target = m_useStreaming ? Uri(Utility::ConvertToUTF16(m_manifestInstallerRef.Url)) : Uri(m_downloadedInstaller.c_str());

        m_reporterRef.ShowMsg("Starting package install...");
        ExecuteInstallerAsync(target);
        m_reporterRef.ShowMsg("Successfully installed.");
    }

    void MsixInstallerHandler::ExecuteInstallerAsync(const winrt::Windows::Foundation::Uri& uri)
    {
        DeploymentOptions deploymentOptions =
            DeploymentOptions::ForceApplicationShutdown |
            DeploymentOptions::ForceTargetApplicationShutdown;
        m_reporterRef.ExecuteWithProgress(std::bind(Deployment::RequestAddPackageAsync, uri, deploymentOptions, std::placeholders::_1));
    }
}