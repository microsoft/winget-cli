// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Common.h"
#include "ShellExecuteInstallerHandler.h"

using namespace AppInstaller::CLI;
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

        m_reporterRef.ShowMsg("Installing package ...");

        std::string installerArgs = GetInstallerArgs();
        AICLI_LOG(CLI, Info, << "Installer args: " << installerArgs);

        RenameDownloadedInstaller();

        auto installResult = m_reporterRef.ExecuteWithProgress(
            std::bind(ExecuteInstaller,
                m_downloadedInstaller,
                installerArgs,
                std::placeholders::_1));

        if (!installResult)
        {
            m_reporterRef.ShowMsg("Installation abandoned", ExecutionReporter::Level::Error);
        }
        else if (installResult.value() != 0)
        {
            m_reporterRef.ShowMsg("Install failed. Exit code: " + std::to_string(installResult.value()), ExecutionReporter::Level::Error);

            THROW_EXCEPTION_MSG(WorkflowException(APPINSTALLER_CLI_ERROR_INSTALLFLOW_FAILED),
                "Install failed. Installer task returned: %u", installResult.value());
        }
        else
        {
            m_reporterRef.ShowMsg("Successfully installed!");
        }
    }

    std::optional<DWORD> ShellExecuteInstallerHandler::ExecuteInstaller(const std::filesystem::path& filePath, const std::string& args, IProgressCallback& progress)
    {
        AICLI_LOG(CLI, Info, << "Staring installer. Path: " << filePath);

        SHELLEXECUTEINFOA execInfo = { 0 };
        execInfo.cbSize = sizeof(SHELLEXECUTEINFO);
        execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        std::string filePathUTF8Str = Utility::ConvertToUTF8(filePath.c_str());
        execInfo.lpFile = filePathUTF8Str.c_str();
        execInfo.lpParameters = args.c_str();
        // Some installer forces UI. Setting to SW_HIDE will hide installer UI and installation will hang forever.
        // Verified setting to SW_SHOW does not hurt silent mode since no UI will be shown.
        execInfo.nShow = SW_SHOW;
        if (!ShellExecuteExA(&execInfo) || !execInfo.hProcess)
        {
            return GetLastError();
        }
                
        wil::unique_process_handle process{ execInfo.hProcess };

        // Wait for installation to finish
        while (!progress.IsCancelled())
        {
            DWORD waitResult = WaitForSingleObject(process.get(), 250);
            if (waitResult == WAIT_OBJECT_0)
            {
                break;
            }
            if (waitResult != WAIT_TIMEOUT)
            {
                THROW_LAST_ERROR_MSG("Unexpected WaitForSingleObjectResult: %d", waitResult);
            }
        }

        if (progress.IsCancelled())
        {
            return {};
        }
        else
        {
            DWORD exitCode = 0;
            GetExitCodeProcess(process.get(), &exitCode);
            return exitCode;
        }
    }

    std::string ShellExecuteInstallerHandler::GetInstallerArgsTemplate()
    {
        std::string installerArgs = "";
        const std::map<ManifestInstaller::InstallerSwitchType, std::string>& installerSwitches = m_manifestInstallerRef.Switches;

        // Construct install experience arg.
        if (m_argsRef.Contains(ExecutionArgs::Type::Silent) && installerSwitches.find(ManifestInstaller::InstallerSwitchType::Silent) != installerSwitches.end())
        {
            installerArgs += installerSwitches.at(ManifestInstaller::InstallerSwitchType::Silent);
        }
        else if (m_argsRef.Contains(ExecutionArgs::Type::Interactive) && installerSwitches.find(ManifestInstaller::InstallerSwitchType::Interactive) != installerSwitches.end())
        {
            installerArgs += installerSwitches.at(ManifestInstaller::InstallerSwitchType::Interactive);
        }
        else if (installerSwitches.find(ManifestInstaller::InstallerSwitchType::SilentWithProgress) != installerSwitches.end())
        {
            installerArgs += installerSwitches.at(ManifestInstaller::InstallerSwitchType::SilentWithProgress);
        }

        // Construct language arg if necessary.
        if (m_argsRef.Contains(ExecutionArgs::Type::Language) && installerSwitches.find(ManifestInstaller::InstallerSwitchType::Language) != installerSwitches.end())
        {
            installerArgs += ' ' + installerSwitches.at(ManifestInstaller::InstallerSwitchType::Language);
        }

        // Construct install location arg if necessary.
        if (m_argsRef.Contains(ExecutionArgs::Type::InstallLocation) && installerSwitches.find(ManifestInstaller::InstallerSwitchType::InstallLocation) != installerSwitches.end())
        {
            installerArgs += ' ' + installerSwitches.at(ManifestInstaller::InstallerSwitchType::InstallLocation);
        }

        // Construct log path arg.
        if (installerSwitches.find(ManifestInstaller::InstallerSwitchType::Log) != installerSwitches.end())
        {
            installerArgs += ' ' + installerSwitches.at(ManifestInstaller::InstallerSwitchType::Log);
        }

        // Construct custom arg.
        if (installerSwitches.find(ManifestInstaller::InstallerSwitchType::Custom) != installerSwitches.end())
        {
            installerArgs += ' ' + installerSwitches.at(ManifestInstaller::InstallerSwitchType::Custom);
        }

        return installerArgs;
    }

    void ShellExecuteInstallerHandler::PopulateInstallerArgsTemplate(std::string& installerArgs)
    {
        // Populate <LogPath> with value from command line or temp path.
        std::string logPath;
        if (m_argsRef.Contains(ExecutionArgs::Type::Log))
        {
            logPath = *m_argsRef.GetArg(ExecutionArgs::Type::Log);
        }
        else
        {
            logPath = Utility::ConvertToUTF8(m_downloadedInstaller.c_str()) + ".log";
        }
        Utility::FindAndReplace(installerArgs, std::string(ARG_TOKEN_LOGPATH), logPath);

        // Populate <InstallPath> with value from command line.
        if (m_argsRef.Contains(ExecutionArgs::Type::InstallLocation))
        {
            Utility::FindAndReplace(installerArgs, std::string(ARG_TOKEN_INSTALLPATH), *m_argsRef.GetArg(ExecutionArgs::Type::InstallLocation));
        }

        // Todo: language token support will be implemented later
    }

    std::string ShellExecuteInstallerHandler::GetInstallerArgs()
    {
        // If override switch is specified, use the override value as installer args.
        if (m_argsRef.Contains(ExecutionArgs::Type::Override))
        {
            return *m_argsRef.GetArg(ExecutionArgs::Type::Override);
        }

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

        // std::filesystem::rename() handles motw correctly if applicable.
        std::filesystem::rename(m_downloadedInstaller, renamedDownloadedInstaller);

        m_downloadedInstaller.assign(renamedDownloadedInstaller);
        AICLI_LOG(CLI, Info, << "Successfully renamed downloaded installer. Path: " << m_downloadedInstaller );
    }
}