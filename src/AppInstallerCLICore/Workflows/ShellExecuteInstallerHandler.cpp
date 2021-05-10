// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ShellExecuteInstallerHandler.h"
#include "AppInstallerFileLogger.h"

using namespace AppInstaller::CLI;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        // ShellExecutes the given path.
        std::optional<DWORD> InvokeShellExecute(const std::filesystem::path& filePath, const std::string& args, IProgressCallback& progress)
        {
            AICLI_LOG(CLI, Info, << "Starting: '" << filePath.u8string() << "' with arguments '" << args << '\'');

            SHELLEXECUTEINFOW execInfo = { 0 };
            execInfo.cbSize = sizeof(execInfo);
            execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
            execInfo.lpFile = filePath.c_str();
            std::wstring argsUtf16 = Utility::ConvertToUTF16(args);
            execInfo.lpParameters = argsUtf16.c_str();
            // Some installers force UI. Setting to SW_HIDE will hide installer UI and installation will never complete.
            // Verified setting to SW_SHOW does not hurt silent mode since no UI will be shown.
            execInfo.nShow = SW_SHOW;

            THROW_LAST_ERROR_IF(!ShellExecuteExW(&execInfo) || !execInfo.hProcess);

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
                    THROW_LAST_ERROR_MSG("Unexpected WaitForSingleObjectResult: %lu", waitResult);
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
            bool isUpdate = WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerExecutionUseUpdate);

            const auto& installer = context.Get<Execution::Data::Installer>();
            const auto& installerSwitches = installer->Switches;
            std::string installerArgs = {};

            // Construct install experience arg.
            // SilentWithProgress is default, so look for it first.
            auto experienceArgsItr = installerSwitches.find(InstallerSwitchType::SilentWithProgress);

            if (context.Args.Contains(Execution::Args::Type::Interactive))
            {
                // If interactive requested, always use Interactive (or nothing). If the installer supports
                // interactive it is usually the default, and thus it is cumbersome to put a blank entry in
                // the manifest.
                experienceArgsItr = installerSwitches.find(InstallerSwitchType::Interactive);
            }
            // If no SilentWithProgress exists, or Silent requested, try to find Silent.
            else if (experienceArgsItr == installerSwitches.end() || context.Args.Contains(Execution::Args::Type::Silent))
            {
                auto silentItr = installerSwitches.find(InstallerSwitchType::Silent);
                // If Silent requested, but doesn't exist, then continue using SilentWithProgress.
                if (silentItr != installerSwitches.end())
                {
                    experienceArgsItr = silentItr;
                }
            }

            if (experienceArgsItr != installerSwitches.end())
            {
                installerArgs += experienceArgsItr->second;
            }

            // Construct log path arg.
            if (installerSwitches.find(InstallerSwitchType::Log) != installerSwitches.end())
            {
                installerArgs += ' ' + installerSwitches.at(InstallerSwitchType::Log);
            }

            // Construct custom arg.
            if (installerSwitches.find(InstallerSwitchType::Custom) != installerSwitches.end())
            {
                installerArgs += ' ' + installerSwitches.at(InstallerSwitchType::Custom);
            }

            // Construct update arg if applicable
            if (isUpdate && installerSwitches.find(InstallerSwitchType::Update) != installerSwitches.end())
            {
                installerArgs += ' ' + installerSwitches.at(InstallerSwitchType::Update);
            }

            // Construct install location arg if necessary.
            if (!isUpdate &&
                context.Args.Contains(Execution::Args::Type::InstallLocation) &&
                installerSwitches.find(InstallerSwitchType::InstallLocation) != installerSwitches.end())
            {
                installerArgs += ' ' + installerSwitches.at(InstallerSwitchType::InstallLocation);
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
                const auto& manifest = context.Get<Execution::Data::Manifest>();

                auto path = Runtime::GetPathTo(Runtime::PathName::DefaultLogLocation);
                path /= Logging::FileLogger::DefaultPrefix();
                path += Utility::ConvertToUTF16(manifest.Id + '.' + manifest.Version);
                path += '-';
                path += Utility::GetCurrentTimeForFilename();
                path += Logging::FileLogger::DefaultExt();

                logPath = path.u8string();
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

        // Gets the arguments for uninstalling an MSI with MsiExec
        std::string GetMsiExecUninstallArgs(Execution::Context& context, const Utility::LocIndString& productCode)
        {
            std::string args = "/x" + productCode.get();

            // Set UI level for MsiExec with the /q flag.
            // If interactive is requested, use the default instead of Reduced or Full as the installer may not use them.
            if (context.Args.Contains(Execution::Args::Type::Silent))
            {
                // n = None = silent
                args += " /qn";
            }
            else if (!context.Args.Contains(Execution::Args::Type::Interactive))
            {
                // b = Basic = only progress bar
                args += " /qb";
            }

            return args;
        }
    }

    void ShellExecuteInstallImpl(Execution::Context& context)
    {
        context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

        const std::string& installerArgs = context.Get<Execution::Data::InstallerArgs>();
        const auto& additionalSuccessCodes = context.Get<Execution::Data::Installer>()->InstallerSuccessCodes;

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
        else if (installResult.value() != 0 && (std::find(additionalSuccessCodes.begin(), additionalSuccessCodes.end(), installResult.value()) == additionalSuccessCodes.end()))
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
        case InstallerTypeEnum::Burn:
        case InstallerTypeEnum::Exe:
        case InstallerTypeEnum::Inno:
        case InstallerTypeEnum::Nullsoft:
            renamedDownloadedInstaller += L".exe";
            break;
        case InstallerTypeEnum::Msi:
        case InstallerTypeEnum::Wix:
            renamedDownloadedInstaller += L".msi";
            break;
        }

        // std::filesystem::rename() handles motw correctly if applicable.
        std::filesystem::rename(installerPath, renamedDownloadedInstaller);

        installerPath.assign(renamedDownloadedInstaller);
        AICLI_LOG(CLI, Info, << "Successfully renamed downloaded installer. Path: " << installerPath);
    }

    void ShellExecuteUninstallImpl(Execution::Context& context)
    {
        context.Reporter.Info() << Resource::String::UninstallFlowStartingPackageUninstall << std::endl;
        std::wstring commandUtf16 = Utility::ConvertToUTF16(context.Get<Execution::Data::UninstallString>());

        // Parse the command string as application and command line for CreateProcess
        wil::unique_cotaskmem_string app = nullptr;
        wil::unique_cotaskmem_string args = nullptr;
        THROW_IF_FAILED(SHEvaluateSystemCommandTemplate(commandUtf16.c_str(), &app, NULL, &args));

        auto uninstallResult = context.Reporter.ExecuteWithProgress(
            std::bind(InvokeShellExecute,
                std::filesystem::path(app.get()),
                Utility::ConvertToUTF8(args.get()),
                std::placeholders::_1));

        if (!uninstallResult)
        {
            context.Reporter.Warn() << Resource::String::UninstallAbandoned << std::endl;
            AICLI_TERMINATE_CONTEXT(E_ABORT);
        }
        else if (uninstallResult.value() != 0)
        {
            const auto installedPackageVersion = context.Get<Execution::Data::InstalledPackageVersion>();
            Logging::Telemetry().LogUninstallerFailure(
                installedPackageVersion->GetProperty(PackageVersionProperty::Id),
                installedPackageVersion->GetProperty(PackageVersionProperty::Version),
                "UninstallString",
                uninstallResult.value());

            context.Reporter.Error() << Resource::String::UninstallFailedWithCode << ' ' << uninstallResult.value() << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_EXEC_UNINSTALL_COMMAND_FAILED);
        }
        else
        {
            context.Reporter.Info() << Resource::String::UninstallFlowUninstallSuccess << std::endl;
        }
    }

    void ShellExecuteMsiExecUninstall(Execution::Context& context)
    {
        const auto& productCodes = context.Get<Execution::Data::ProductCodes>();
        context.Reporter.Info() << Resource::String::UninstallFlowStartingPackageUninstall << std::endl;

        const std::filesystem::path msiexecPath{ ExpandEnvironmentVariables(L"%windir%\\system32\\msiexec.exe") };

        for (const auto& productCode : productCodes)
        {
            AICLI_LOG(CLI, Info, << "Removing: " << productCode);
            auto uninstallResult = context.Reporter.ExecuteWithProgress(
                std::bind(InvokeShellExecute,
                    msiexecPath,
                    GetMsiExecUninstallArgs(context, productCode),
                    std::placeholders::_1));

            if (!uninstallResult)
            {
                context.Reporter.Warn() << Resource::String::UninstallAbandoned << std::endl;
                AICLI_TERMINATE_CONTEXT(E_ABORT);
            }
            else if (uninstallResult.value() != 0)
            {
                // TODO: Check for other success codes
                const auto installedPackageVersion = context.Get<Execution::Data::InstalledPackageVersion>();
                Logging::Telemetry().LogUninstallerFailure(
                    installedPackageVersion->GetProperty(PackageVersionProperty::Id),
                    installedPackageVersion->GetProperty(PackageVersionProperty::Version),
                    "MsiExec",
                    uninstallResult.value());

                context.Reporter.Error() << Resource::String::UninstallFailedWithCode << ' ' << uninstallResult.value() << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_EXEC_UNINSTALL_COMMAND_FAILED);
            }
        }

        context.Reporter.Info() << Resource::String::UninstallFlowUninstallSuccess << std::endl;
    }
}