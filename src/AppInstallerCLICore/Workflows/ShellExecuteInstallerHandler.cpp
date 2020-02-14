// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "Common.h"
#include "Commands/Common.h"
#include "ShellExecuteInstallerHandler.h"

using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

namespace AppInstaller::Workflow
{
    void ShellExecuteInstallerHandler::Install()
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

    std::future<DWORD> ShellExecuteInstallerHandler::ExecuteInstallerAsync(const std::filesystem::path& filePath, const std::string& args)
    {
        AICLI_LOG(CLI, Info, << "Staring installer. Path: " << filePath);
        return std::async(std::launch::async, [this, filePath, args]
            {
                SHELLEXECUTEINFOA execInfo = { 0 };
                execInfo.cbSize = sizeof(SHELLEXECUTEINFO);
                execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
                std::string filePathUTF8Str = Utility::ConvertToUTF8(filePath.c_str());
                execInfo.lpFile = filePathUTF8Str.c_str();
                execInfo.lpParameters = args.c_str();
                execInfo.nShow = m_argsRef.Contains(CLI::ARG_INTERACTIVE) ? SW_SHOW : SW_HIDE;
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

    std::string ShellExecuteInstallerHandler::GetInstallerArgsTemplate()
    {
        std::string installerArgs = "";
        const std::map<ManifestInstaller::InstallerSwitchType, std::string>& installerSwitches = m_manifestInstallerRef.Switches;

        // Construct install experience arg.
        if (m_argsRef.Contains(CLI::ARG_SILENT) && installerSwitches.find(ManifestInstaller::InstallerSwitchType::Silent) != installerSwitches.end())
        {
            installerArgs += installerSwitches.at(ManifestInstaller::InstallerSwitchType::Silent);
        }
        else if (m_argsRef.Contains(CLI::ARG_INTERACTIVE) && installerSwitches.find(ManifestInstaller::InstallerSwitchType::Interactive) != installerSwitches.end())
        {
            installerArgs += installerSwitches.at(ManifestInstaller::InstallerSwitchType::Interactive);
        }
        else if (installerSwitches.find(ManifestInstaller::InstallerSwitchType::SilentWithProgress) != installerSwitches.end())
        {
            installerArgs += installerSwitches.at(ManifestInstaller::InstallerSwitchType::SilentWithProgress);
        }

        // Construct language arg if necessary.
        if (m_argsRef.Contains(CLI::ARG_LANGUAGE) && installerSwitches.find(ManifestInstaller::InstallerSwitchType::Language) != installerSwitches.end())
        {
            installerArgs += ' ' + installerSwitches.at(ManifestInstaller::InstallerSwitchType::Language);
        }

        // Construct install location arg if necessary.
        if (m_argsRef.Contains(CLI::ARG_INSTALLLOCATION) && installerSwitches.find(ManifestInstaller::InstallerSwitchType::InstallLocation) != installerSwitches.end())
        {
            installerArgs += ' ' + installerSwitches.at(ManifestInstaller::InstallerSwitchType::InstallLocation);
        }

        // Construct log path arg.
        if (installerSwitches.find(ManifestInstaller::InstallerSwitchType::Log) != installerSwitches.end())
        {
            installerArgs += ' ' + installerSwitches.at(ManifestInstaller::InstallerSwitchType::Log);
        }

        // Construct custom arg. Custom arg from command line overrides ones specified in manifest.
        if (m_argsRef.Contains(CLI::ARG_CUSTOM))
        {
            installerArgs += ' ' + *(m_argsRef.GetArg(CLI::ARG_CUSTOM));
        }
        else if (installerSwitches.find(ManifestInstaller::InstallerSwitchType::Custom) != installerSwitches.end())
        {
            installerArgs += ' ' + installerSwitches.at(ManifestInstaller::InstallerSwitchType::Custom);
        }

        return installerArgs;
    }

    void ShellExecuteInstallerHandler::PopulateInstallerArgsTemplate(std::string& installerArgs)
    {
        // Populate <LogPath> with value from command line or temp path.
        std::string logPath;
        if (m_argsRef.Contains(CLI::ARG_LOG))
        {
            logPath = *m_argsRef.GetArg(CLI::ARG_LOG);
        }
        else
        {
            logPath = Utility::ConvertToUTF8(m_downloadedInstaller.c_str()) + ".log";
        }
        Utility::FindAndReplace(installerArgs, std::string(ARG_TOKEN_LOGPATH), logPath);

        // Populate <InstallPath> with value from command line or current path.
        Utility::FindAndReplace(installerArgs, std::string(ARG_TOKEN_INSTALLPATH), *m_argsRef.GetArg(CLI::ARG_INSTALLLOCATION));

        // Todo: language token support will be implemented later
    }

    std::string ShellExecuteInstallerHandler::GetInstallerArgs()
    {
        std::string installerArgs = GetInstallerArgsTemplate();

        PopulateInstallerArgsTemplate(installerArgs);

        return installerArgs;
    }

    void ShellExecuteInstallerHandler::RenameDownloadedInstaller()
    {
        std::filesystem::path renamedDownloadedInstaller(m_downloadedInstaller);

        switch(m_manifestInstallerRef.InstallerType)
        {
        case ManifestInstaller::InstallerTypeEnum::Burn:
        case ManifestInstaller::InstallerTypeEnum::Exe:
        case ManifestInstaller::InstallerTypeEnum::Inno:
        case ManifestInstaller::InstallerTypeEnum::Nullsoft:
            renamedDownloadedInstaller += L".exe";
            break;
        case ManifestInstaller::InstallerTypeEnum::Msi:
        case ManifestInstaller::InstallerTypeEnum::Wix:
            renamedDownloadedInstaller += L".msi";
            break;
        }

        std::filesystem::rename(m_downloadedInstaller, renamedDownloadedInstaller);

        m_downloadedInstaller.assign(renamedDownloadedInstaller);
        AICLI_LOG(CLI, Info, << "Successfully renamed downloaded installer. Path: " << m_downloadedInstaller );
    }
}