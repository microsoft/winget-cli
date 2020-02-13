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
        return std::async(std::launch::async, [this, &filePath, &args]
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

    std::string ShellExecuteInstallerHandler::GetDefaultArg(std::string_view argType)
    {
        // This contains knowledge about known installer type's known default switches.
        std::string arg;
        if (argType == CLI::ARG_SILENT)
        {
            switch (m_manifestInstallerRef.InstallerType)
            {
            case ManifestInstaller::InstallerTypeEnum::Burn:
            case ManifestInstaller::InstallerTypeEnum::Wix:
            case ManifestInstaller::InstallerTypeEnum::Msi:
                arg = "/quiet";
                break;
            case ManifestInstaller::InstallerTypeEnum::Nullsoft:
                arg = "/S";
                break;
            case ManifestInstaller::InstallerTypeEnum::Inno:
                arg = "/VERYSILENT";
                break;
            }
        }
        else if (argType == CLI::ARG_SILENTWITHPROGRESS)
        {
            switch (m_manifestInstallerRef.InstallerType)
            {
            case ManifestInstaller::InstallerTypeEnum::Burn:
            case ManifestInstaller::InstallerTypeEnum::Wix:
            case ManifestInstaller::InstallerTypeEnum::Msi:
                arg = "/passive";
                break;
            case ManifestInstaller::InstallerTypeEnum::Nullsoft:
                arg = "/S";
                break;
            case ManifestInstaller::InstallerTypeEnum::Inno:
                arg = "/SILENT";
                break;
            }
        }
        else if (argType == CLI::ARG_INTERACTIVE)
        {
            // Nothing to do for now
        }
        else if (argType == CLI::ARG_LANGUAGE)
        {
            // Todo: language arguments will be implemented later
        }
        else if (argType == CLI::ARG_LOG)
        {
            switch (m_manifestInstallerRef.InstallerType)
            {
            case ManifestInstaller::InstallerTypeEnum::Burn:
            case ManifestInstaller::InstallerTypeEnum::Wix:
            case ManifestInstaller::InstallerTypeEnum::Msi:
                arg = "/log \"" + std::string(ARG_TOKEN_LOGPATH) + "\"";
                break;
            case ManifestInstaller::InstallerTypeEnum::Inno:
                arg = "/LOG=\"" + std::string(ARG_TOKEN_LOGPATH) + "\"";
                break;
            }
        }
        else if (argType == CLI::ARG_INSTALLLOCATION)
        {
            switch (m_manifestInstallerRef.InstallerType)
            {
            case ManifestInstaller::InstallerTypeEnum::Burn:
            case ManifestInstaller::InstallerTypeEnum::Wix:
            case ManifestInstaller::InstallerTypeEnum::Msi:
                arg = "TARGETDIR=\"" + std::string(ARG_TOKEN_INSTALLPATH) + "\"";
                break;
            case ManifestInstaller::InstallerTypeEnum::Inno:
                arg = "/DIR=\"" + std::string(ARG_TOKEN_INSTALLPATH) + "\"";
                break;
            case ManifestInstaller::InstallerTypeEnum::Nullsoft:
                arg = "/D=\"" + std::string(ARG_TOKEN_INSTALLPATH) + "\"";
                break;
            }
        }

        return arg;
    }

    std::string ShellExecuteInstallerHandler::GetInstallerArgsTemplate()
    {
        InstallerSwitches const* installerSwitches = nullptr;

        if (m_manifestInstallerRef.Switches.has_value())
        {
            installerSwitches = &(m_manifestInstallerRef.Switches.value());
        }

        std::string installerArgs = "";

        // Construct install experience arg.
        if (m_argsRef.Contains(CLI::ARG_SILENT))
        {
            installerArgs += installerSwitches && !installerSwitches->Silent.empty() ?
                installerSwitches->Silent : GetDefaultArg(CLI::ARG_SILENT);
        }
        else if (m_argsRef.Contains(CLI::ARG_INTERACTIVE))
        {
            installerArgs += installerSwitches && !installerSwitches->Interactive.empty() ?
                installerSwitches->Interactive : GetDefaultArg(CLI::ARG_INTERACTIVE);
        }
        else
        {
            installerArgs += installerSwitches && !installerSwitches->SilentWithProgress.empty() ?
                installerSwitches->SilentWithProgress : GetDefaultArg(CLI::ARG_SILENTWITHPROGRESS);
        }

        // Construct language arg.
        installerArgs += ' ';
        installerArgs += installerSwitches && !installerSwitches->Language.empty() ?
            installerSwitches->Language : GetDefaultArg(CLI::ARG_LANGUAGE);

        // Construct log path arg.
        installerArgs += ' ';
        installerArgs += installerSwitches && !installerSwitches->Log.empty() ?
            installerSwitches->Log : GetDefaultArg(CLI::ARG_LOG);

        // Construct custom arg. Custom arg from command line overrides ones specified in manifest.
        installerArgs += ' ';
        if (m_argsRef.Contains(CLI::ARG_CUSTOM))
        {
            installerArgs += *(m_argsRef.GetArg(CLI::ARG_CUSTOM));
        }
        else if (installerSwitches && !installerSwitches->Custom.empty())
        {
            installerArgs += installerSwitches->Custom;
        }

        return installerArgs;
    }

    void ShellExecuteInstallerHandler::PopulateInstallerArgsTemplate(std::string& installerArgs)
    {
        // Populate <LogPath> with value from command line or temp path.
        std::string::size_type pos = 0u;
        while ((pos = installerArgs.find(ARG_TOKEN_LOGPATH, pos)) != std::string::npos)
        {
            std::string logPath;
            if (m_argsRef.Contains(CLI::ARG_LOG))
            {
                logPath = *m_argsRef.GetArg(CLI::ARG_LOG);
            }
            else
            {
                logPath = Utility::ConvertToUTF8(m_downloadedInstaller.c_str()) + ".log";
            }
            installerArgs.replace(pos, ARG_TOKEN_LOGPATH.length(), logPath);
            pos += logPath.length();
        }

        // Populate <InstallPath> with value from command line or current path.
        pos = 0u;
        while ((pos = installerArgs.find(ARG_TOKEN_INSTALLPATH, pos)) != std::string::npos)
        {
            std::string installPath;
            if (m_argsRef.Contains(CLI::ARG_INSTALLLOCATION))
            {
                installPath = *m_argsRef.GetArg(CLI::ARG_INSTALLLOCATION);
            }
            else
            {
                installPath = Utility::ConvertToUTF8(std::filesystem::current_path().c_str());
            }
            installerArgs.replace(pos, ARG_TOKEN_LOGPATH.length(), installPath);
            pos += installPath.length();
        }

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