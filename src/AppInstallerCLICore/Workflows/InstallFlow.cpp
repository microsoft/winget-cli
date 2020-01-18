// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "InstallFlow.h"
#include "AppInstallerDownloader.h"
#include "ManifestComparator.h"

using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

namespace AppInstaller::Workflow {

    void InstallFlow::Install()
    {
        ManifestComparator manifestComparator(m_packageManifest, m_reporter);

        m_selectedLocalization = manifestComparator.GetPreferredLocalization(std::locale(""));

        m_reporter.ShowPackageInfo(
            m_packageManifest.Name,
            m_packageManifest.Version,
            m_packageManifest.Author,
            m_selectedLocalization.Description,
            m_selectedLocalization.Homepage,
            m_selectedLocalization.LicenseUrl
        );

        m_selectedInstaller = manifestComparator.GetPreferredInstaller(std::locale(""));

        DownloadInstaller();
        ExecuteInstaller();
    }

    void InstallFlow::DownloadInstaller()
    {
        // Todo: Rework the path logic. The new path logic should work with MOTW.
        std::filesystem::path tempInstallerPath = Runtime::GetPathToTemp();
        tempInstallerPath /= m_packageManifest.Id + '_' + m_packageManifest.Version + '.' + m_selectedInstaller.InstallerType;

        AICLI_LOG(CLI, Info, << "Generated temp download path: " << tempInstallerPath);

        auto downloader = Downloader::StartDownloadAsync(
            m_selectedInstaller.Url,
            tempInstallerPath,
            true,
            &m_reporter.GetDownloaderCallback());

        auto downloadResult = downloader->Wait();

        if (downloadResult == DownloaderResult::Failed)
        {
            m_reporter.ShowMsg(WorkflowReporter::Level::Error, "Package download failed.");
            THROW_EXCEPTION_MSG(WorkflowException(CLICORE_ERROR_INSTALLFLOW_FAILED), "Package download failed");
        }
        else if (downloadResult == DownloaderResult::Canceled)
        {
            m_reporter.ShowMsg(WorkflowReporter::Level::Info, "Package download canceled.");
            THROW_EXCEPTION_MSG(WorkflowException(CLICORE_ERROR_INSTALLFLOW_FAILED), "Package download canceled");
        }

        if (!std::equal(
            m_selectedInstaller.Sha256.begin(),
            m_selectedInstaller.Sha256.end(),
            downloader->GetDownloadHash().begin()))
        {
            AICLI_LOG(CLI, Error,
                << "Package hash verification failed. SHA256 in manifest: "
                << SHA256::ConvertToString(m_selectedInstaller.Sha256)
                << "SHA256 from download: "
                << SHA256::ConvertToString(downloader->GetDownloadHash()));

            m_reporter.ShowMsg(WorkflowReporter::Level::Warning, "Package hash verification failed. Continue? (Y|N)");

            char response;
            std::cin >> response;
            if (response != 'y' && response != 'Y')
            {
                m_reporter.ShowMsg(WorkflowReporter::Level::Error, "Canceled. Package hash mismatch.");
                THROW_EXCEPTION_MSG(WorkflowException(CLICORE_ERROR_INSTALLFLOW_FAILED), "Package installation canceled");
            }
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Downloaded package hash verified");
            m_reporter.ShowMsg(WorkflowReporter::Level::Info, "Successfully verified SHA256.");
        }

        m_downloadedInstaller = tempInstallerPath;
    }

    void InstallFlow::ExecuteInstaller()
    {
        if (m_downloadedInstaller.empty())
        {
            THROW_EXCEPTION_MSG(WorkflowException(CLICORE_ERROR_INSTALLFLOW_FAILED), "Installer not downloaded yet");
        }

        m_reporter.ShowMsg(WorkflowReporter::Level::Info, "Installing package ...");

        std::string installerArgs = GetInstallerArgs();
        AICLI_LOG(CLI, Info, << "Installer args: " << installerArgs);

        // Todo: add support for other installer types
        std::future<DWORD> installTask;
        if (Utility::ToLower(m_selectedInstaller.InstallerType) == "exe")
        {
            installTask = ExecuteExeInstallerAsync(m_downloadedInstaller, installerArgs);
        }
        else
        {
            m_reporter.ShowMsg(WorkflowReporter::Level::Error, "Installer type not supported.");
            THROW_EXCEPTION_MSG(WorkflowException(CLICORE_ERROR_INSTALLFLOW_FAILED), "Installer type not supported");
        }

        m_reporter.ShowIndefiniteSpinner(true);

        installTask.wait();

        m_reporter.ShowIndefiniteSpinner(false);

        auto installResult = installTask.get();

        if (installResult != 0)
        {
            m_reporter.ShowMsg(WorkflowReporter::Level::Error, "Install failed. Exit code: " + std::to_string(installResult));

            THROW_EXCEPTION_MSG(WorkflowException(CLICORE_ERROR_INSTALLFLOW_FAILED),
                "Install failed. Installer task returned: %u", installResult);
        }

        m_reporter.ShowMsg(WorkflowReporter::Level::Info, "Successfully installed!");
    }

    std::future<DWORD> InstallFlow::ExecuteExeInstallerAsync(const std::filesystem::path& filePath, const std::string& args)
    {
        AICLI_LOG(CLI, Info, << "Staring EXE installer. Path: " << filePath);
        return std::async(std::launch::async, [&filePath, &args] {

            SHELLEXECUTEINFOA execInfo = { 0 };
            execInfo.cbSize = sizeof(SHELLEXECUTEINFO);
            execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
            execInfo.lpFile = Utility::ConvertToUTF8(filePath.c_str()).c_str();
            execInfo.lpParameters = args.c_str();
            execInfo.nShow = SW_SHOW;
            if (!ShellExecuteExA(&execInfo) || !execInfo.hProcess)
            {
                return GetLastError();
            }
            // Wait for installation to finish
            WaitForSingleObject(execInfo.hProcess, INFINITE);

            // Get exe exit code
            DWORD exitCode;
            GetExitCodeProcess(execInfo.hProcess, &exitCode);

            CloseHandle(execInfo.hProcess);

            return exitCode;
        });
    }

    std::string InstallFlow::GetInstallerArgs()
    {
        // Todo: Implement arg selection logic.
        if (m_selectedInstaller.Switches.has_value())
        {
            return m_selectedInstaller.Switches.value().Default;
        }

        return "";
    }
}