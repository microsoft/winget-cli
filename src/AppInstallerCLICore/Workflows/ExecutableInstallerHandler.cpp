// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "Common.h"
#include "ExecutableInstallerHandler.h"

using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

namespace AppInstaller::Workflow
{
    ExecutableInstallerHandler::ExecutableInstallerHandler(
        const Manifest::ManifestInstaller& manifestInstaller,
        WorkflowReporter& reporter) :
    InstallerHandlerBase(manifestInstaller, reporter)
    {
        // Todo: add support for other installer types.
        // This Installer Handler should support Inno, Wix, Nullsoft, Msi and Exe.
        if (manifestInstaller.InstallerType != ManifestInstaller::InstallerTypeEnum::Exe)
        {
            THROW_HR_MSG(E_UNEXPECTED, "Installer type not supported.");
        }
    }

    void ExecutableInstallerHandler::Install()
    {
        if (m_downloadedInstaller.empty())
        {
            THROW_EXCEPTION_MSG(WorkflowException(APPINSTALLER_CLI_ERROR_INSTALLFLOW_FAILED), "Installer not downloaded yet");
        }

        m_reporterRef.ShowMsg(WorkflowReporter::Level::Info, "Installing package ...");

        std::string installerArgs = GetInstallerArgs();
        AICLI_LOG(CLI, Info, << "Installer args: " << installerArgs);

        RenameDownloadedInstaller();
        std::future<DWORD> installTask = ExecuteInstallerAsync(m_downloadedInstaller, installerArgs);

        m_reporterRef.ShowIndefiniteProgress(true);

        installTask.wait();

        m_reporterRef.ShowIndefiniteProgress(false);

        auto installResult = installTask.get();

        if (installResult != 0)
        {
            m_reporterRef.ShowMsg(WorkflowReporter::Level::Error, "Install failed. Exit code: " + std::to_string(installResult));

            THROW_EXCEPTION_MSG(WorkflowException(APPINSTALLER_CLI_ERROR_INSTALLFLOW_FAILED),
                "Install failed. Installer task returned: %u", installResult);
        }

        m_reporterRef.ShowMsg(WorkflowReporter::Level::Info, "Successfully installed!");
    }

    std::future<DWORD> ExecutableInstallerHandler::ExecuteInstallerAsync(const std::filesystem::path& filePath, const std::string& args)
    {
        AICLI_LOG(CLI, Info, << "Staring installer. Path: " << filePath);
        return std::async(std::launch::async, [&filePath, &args]
            {
                SHELLEXECUTEINFOA execInfo = { 0 };
                execInfo.cbSize = sizeof(SHELLEXECUTEINFO);
                execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
                std::string filePathUTF8Str = Utility::ConvertToUTF8(filePath.c_str());
                execInfo.lpFile = filePathUTF8Str.c_str();
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

    std::string ExecutableInstallerHandler::GetInstallerArgs()
    {
        // Todo: Implement arg selection logic.
        if (m_manifestInstallerRef.Switches.has_value())
        {
            return m_manifestInstallerRef.Switches.value().Default;
        }

        return "";
    }

    void ExecutableInstallerHandler::RenameDownloadedInstaller()
    {
        std::filesystem::path renamedDownloadedInstaller(m_downloadedInstaller);

        if (m_manifestInstallerRef.InstallerType == ManifestInstaller::InstallerTypeEnum::Exe)
        {
            renamedDownloadedInstaller += L".exe";
        }

        std::filesystem::rename(m_downloadedInstaller, renamedDownloadedInstaller);

        m_downloadedInstaller.assign(renamedDownloadedInstaller);
        AICLI_LOG(CLI, Info, << "Successfully renamed downloaded installer. Path: " << m_downloadedInstaller );
    }
}