// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Common.h"
#include "InstallerHandlerBase.h"

using namespace AppInstaller::Manifest;

namespace AppInstaller::Workflow
{
    void InstallerHandlerBase::Download()
    {
        // Todo: Rework the path logic. The new path logic should work with MOTW.
        std::filesystem::path tempInstallerPath = Runtime::GetPathToTemp();
        tempInstallerPath /= Utility::SHA256::ConvertToString(m_manifestInstallerRef.Sha256);

        AICLI_LOG(CLI, Info, << "Generated temp download path: " << tempInstallerPath);

        auto hash = m_reporterRef.ExecuteWithProgress(std::bind(Utility::Download,
            m_manifestInstallerRef.Url,
            tempInstallerPath,
            std::placeholders::_1,
            true));

        if (!hash)
        {
            m_reporterRef.ShowMsg(WorkflowReporter::Level::Info, "Package download canceled.");
            THROW_EXCEPTION_MSG(WorkflowException(APPINSTALLER_CLI_ERROR_INSTALLFLOW_FAILED), "Package download canceled");
        }

        if (!std::equal(
            m_manifestInstallerRef.Sha256.begin(),
            m_manifestInstallerRef.Sha256.end(),
            hash.value().begin()))
        {
            AICLI_LOG(CLI, Error,
                << "Package hash verification failed. SHA256 in manifest: "
                << Utility::SHA256::ConvertToString(m_manifestInstallerRef.Sha256)
                << " SHA256 from download: "
                << Utility::SHA256::ConvertToString(hash.value()));

            if (!m_reporterRef.PromptForBoolResponse(WorkflowReporter::Level::Warning, "Package hash verification failed. Continue?"))
            {
                m_reporterRef.ShowMsg(WorkflowReporter::Level::Error, "Canceled. Package hash mismatch.");
                THROW_EXCEPTION_MSG(WorkflowException(APPINSTALLER_CLI_ERROR_INSTALLFLOW_FAILED), "Package installation canceled");
            }
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Downloaded installer hash verified");
            m_reporterRef.ShowMsg(WorkflowReporter::Level::Info, "Successfully verified SHA256.");
        }

        m_downloadedInstaller = tempInstallerPath;
    }
}
