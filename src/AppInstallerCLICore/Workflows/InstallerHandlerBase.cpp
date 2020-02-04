// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "Common.h"
#include "InstallerHandlerBase.h"

using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

namespace AppInstaller::Workflow
{
    void InstallerHandlerBase::Download()
    {
        // Todo: Rework the path logic. The new path logic should work with MOTW.
        std::filesystem::path tempInstallerPath = Runtime::GetPathToTemp();
        tempInstallerPath /= SHA256::ConvertToString(m_manifestInstallerRef.Sha256);

        AICLI_LOG(CLI, Info, << "Generated temp download path: " << tempInstallerPath);

        auto downloader = Downloader::StartDownloadAsync(
            m_manifestInstallerRef.Url,
            tempInstallerPath,
            true,
            &m_downloaderCallback);

        auto downloadResult = downloader->Wait();

        if (downloadResult == DownloaderResult::Failed)
        {
            m_reporterRef.ShowMsg(WorkflowReporter::Level::Error, "Package download failed.");
            THROW_EXCEPTION_MSG(WorkflowException(APPINSTALLER_CLI_ERROR_INSTALLFLOW_FAILED), "Package download failed");
        }
        else if (downloadResult == DownloaderResult::Canceled)
        {
            m_reporterRef.ShowMsg(WorkflowReporter::Level::Info, "Package download canceled.");
            THROW_EXCEPTION_MSG(WorkflowException(APPINSTALLER_CLI_ERROR_INSTALLFLOW_FAILED), "Package download canceled");
        }

        if (!std::equal(
            m_manifestInstallerRef.Sha256.begin(),
            m_manifestInstallerRef.Sha256.end(),
            downloader->GetDownloadHash().begin()))
        {
            AICLI_LOG(CLI, Error,
                << "Package hash verification failed. SHA256 in manifest: "
                << SHA256::ConvertToString(m_manifestInstallerRef.Sha256)
                << "SHA256 from download: "
                << SHA256::ConvertToString(downloader->GetDownloadHash()));

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

    void InstallerHandlerBase::DownloaderCallback::OnStarted()
    {
        m_reporterRef.ShowMsg(WorkflowReporter::Level::Info, "Starting package download ...");
        m_reporterRef.ShowProgress(true, 0);
    }

    void InstallerHandlerBase::DownloaderCallback::OnProgress(LONGLONG progress, LONGLONG downloadSize)
    {
        int progressPercent = static_cast<int>(100 * progress / downloadSize);
        m_reporterRef.ShowProgress(true, progressPercent);
    }

    void InstallerHandlerBase::DownloaderCallback::OnCanceled()
    {
        m_reporterRef.ShowProgress(false, 0);
        m_reporterRef.ShowMsg(WorkflowReporter::Level::Warning, "Package download canceled.");
    }

    void InstallerHandlerBase::DownloaderCallback::OnCompleted()
    {
        m_reporterRef.ShowProgress(false, 0);
        m_reporterRef.ShowMsg(WorkflowReporter::Level::Error, "Package download completed.");
    }
}