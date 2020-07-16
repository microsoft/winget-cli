// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ShellExecuteInstallerHandler.h"

using namespace AppInstaller::CLI;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        // ShellExecutes the given path.
        std::optional<DWORD> InvokeShellExecute(const std::filesystem::path& filePath, const std::string& args, IProgressCallback& progress)
        {
            AICLI_LOG(CLI, Info, << "Starting installer. Path: " << filePath);

            SHELLEXECUTEINFOW execInfo = { 0 };
            execInfo.cbSize = sizeof(execInfo);
            execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
            execInfo.lpFile = filePath.c_str();
            std::wstring argsUtf16 = Utility::ConvertToUTF16(args);
            execInfo.lpParameters = argsUtf16.c_str();
            // Some installers force UI. Setting to SW_HIDE will hide installer UI and installation will never complete.
            // Verified setting to SW_SHOW does not hurt silent mode since no UI will be shown.
            execInfo.nShow = SW_SHOW;
            if (!ShellExecuteExW(&execInfo) || !execInfo.hProcess)
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

        // Gets the escaped installer args.
        std::string GetInstallerArgsTemplate(Execution::Context& context)
        {
            std::string installerArgs = "";
            const std::map<ManifestInstaller::InstallerSwitchType, Utility::NormalizedString>& installerSwitches = context.Get<Execution::Data::Installer>()->Switches;

            // Construct install experience arg.
            // SilentWithProgress is default, so look for it first.
            auto argsItr = installerSwitches.find(ManifestInstaller::InstallerSwitchType::SilentWithProgress);

            if (context.Args.Contains(Execution::Args::Type::Interactive))
            {
                // If interacive requested, always use Interactive (or nothing). If the installer supports
                // interactive it is usually the default, and thus it is cumbersome to put a blank entry in
                // the manifest.
                argsItr = installerSwitches.find(ManifestInstaller::InstallerSwitchType::Interactive);
            }
            // If no SilentWithProgress exists, or Silent requested, try to find Silent.
            else if (argsItr == installerSwitches.end() || context.Args.Contains(Execution::Args::Type::Silent))
            {
                auto silentItr = installerSwitches.find(ManifestInstaller::InstallerSwitchType::Silent);
                // If Silent requested, but doesn't exist, then continue using SilentWithProgress.
                if (silentItr != installerSwitches.end())
                {
                    argsItr = silentItr;
                }
            }

            if (argsItr != installerSwitches.end())
            {
                installerArgs += argsItr->second;
            }

            // Construct language arg if necessary.
            if (context.Args.Contains(Execution::Args::Type::Language) && installerSwitches.find(ManifestInstaller::InstallerSwitchType::Language) != installerSwitches.end())
            {
                installerArgs += ' ' + installerSwitches.at(ManifestInstaller::InstallerSwitchType::Language);
            }

            // Construct install location arg if necessary.
            if (context.Args.Contains(Execution::Args::Type::InstallLocation) && installerSwitches.find(ManifestInstaller::InstallerSwitchType::InstallLocation) != installerSwitches.end())
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

        // Applies values to the template.
        void PopulateInstallerArgsTemplate(Execution::Context& context, std::string& installerArgs)
        {
            // Populate <LogPath> with value from command line or temp path.
            std::string logPath;
            if (context.Args.Contains(Execution::Args::Type::Log))
            {
                logPath = context.Args.GetArg(Execution::Args::Type::Log);
            }
            else
            {
                logPath = Utility::ConvertToUTF8(context.Get<Execution::Data::InstallerPath>().c_str()) + ".log";
            }

            if (Utility::FindAndReplace(installerArgs, std::string(ARG_TOKEN_LOGPATH), logPath))
            {
                context.Add<Execution::Data::LogPath>(Utility::ConvertToUTF16(logPath));
            }

            // Populate <InstallPath> with value from command line.
            if (context.Args.Contains(Execution::Args::Type::InstallLocation))
            {
                Utility::FindAndReplace(installerArgs, std::string(ARG_TOKEN_INSTALLPATH), context.Args.GetArg(Execution::Args::Type::InstallLocation));
            }

            // Todo: language token support will be implemented later
        }
    }

    void ShellExecuteInstallImpl(Execution::Context& context)
    {
        context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

        const std::string& installerArgs = context.Get<Execution::Data::InstallerArgs>();

        auto installResult = context.Reporter.ExecuteWithProgress(
            std::bind(InvokeShellExecute,
                context.Get<Execution::Data::InstallerPath>(),
                installerArgs,
                std::placeholders::_1));

        if (!installResult)
        {
            context.Reporter.Warn() << "Installation abandoned" << std::endl;
            AICLI_TERMINATE_CONTEXT(E_ABORT);
        }
        else if (installResult.value() != 0)
        {
            const auto& manifest = context.Get<Execution::Data::Manifest>();
            Logging::Telemetry().LogInstallerFailure(manifest.Id, manifest.Version, manifest.Channel, "ShellExecute", installResult.value());

            context.Reporter.Error() << "Installer failed with exit code: " << installResult.value() << std::endl;
            // Show installer log path if exists
            if (context.Contains(Execution::Data::LogPath) && std::filesystem::exists(context.Get<Execution::Data::LogPath>()))
            {
                context.Reporter.Info() << "Installer log is available at: " << context.Get<Execution::Data::LogPath>().u8string() << std::endl;
            }

            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_SHELLEXEC_INSTALL_FAILED);
        }
        else
        {
            context.Reporter.Info() << Resource::String::InstallFlowInstallSuccess << std::endl;
        }
    }

    void GetInstallerArgs(Execution::Context& context)
    {
        // If override switch is specified, use the override value as installer args.
        if (context.Args.Contains(Execution::Args::Type::Override))
        {
            context.Add<Execution::Data::InstallerArgs>(std::string{ context.Args.GetArg(Execution::Args::Type::Override) });
            return;
        }

        std::string installerArgs = GetInstallerArgsTemplate(context);

        PopulateInstallerArgsTemplate(context, installerArgs);

        AICLI_LOG(CLI, Info, << "Installer args: " << installerArgs);
        context.Add<Execution::Data::InstallerArgs>(std::move(installerArgs));
    }

    void RenameDownloadedInstaller(Execution::Context& context)
    {
        auto& installerPath = context.Get<Execution::Data::InstallerPath>();
        std::filesystem::path renamedDownloadedInstaller(installerPath);

        switch(context.Get<Execution::Data::Installer>()->InstallerType)
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
        std::filesystem::rename(installerPath, renamedDownloadedInstaller);

        installerPath.assign(renamedDownloadedInstaller);
        AICLI_LOG(CLI, Info, << "Successfully renamed downloaded installer. Path: " << installerPath);
    }
}