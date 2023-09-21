// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ShellExecuteInstallerHandler.h"
#include <AppInstallerFileLogger.h>
#include <AppInstallerRuntime.h>

using namespace AppInstaller::CLI;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        // ShellExecutes the given path.
        std::optional<DWORD> InvokeShellExecuteEx(const std::filesystem::path& filePath, const std::string& args, bool useRunAs, IProgressCallback& progress)
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

            // This installer must be run elevated, but we are not currently.
            // Have ShellExecute elevate the installer since it won't do so itself.
            if (useRunAs)
            {
                execInfo.lpVerb = L"runas";
            }

            THROW_LAST_ERROR_IF(!ShellExecuteExW(&execInfo) || !execInfo.hProcess);

            wil::unique_process_handle process{ execInfo.hProcess };

            // Wait for installation to finish
            while (!progress.IsCancelledBy(CancelReason::User))
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

            if (progress.IsCancelledBy(CancelReason::Any))
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

        std::optional<DWORD> InvokeShellExecute(const std::filesystem::path& filePath, const std::string& args, IProgressCallback& progress)
        {
            return InvokeShellExecuteEx(filePath, args, false, progress);
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

            // Construct custom arg passed in by cli arg
            if (context.Args.Contains(Execution::Args::Type::CustomSwitches))
            {
                std::string_view customSwitches = context.Args.GetArg(Execution::Args::Type::CustomSwitches);
                // Since these arguments are appended to the installer at runtime, it doesn't make sense to append them if empty or whitespace
                if (!Utility::IsEmptyOrWhitespace(customSwitches))
                {
                    installerArgs += ' ' + std::string{ customSwitches };
                }
            }

            // Construct update arg if applicable
            if (isUpdate && installerSwitches.find(InstallerSwitchType::Update) != installerSwitches.end())
            {
                installerArgs += ' ' + installerSwitches.at(InstallerSwitchType::Update);
            }

            // Construct install location arg if necessary.
            if (context.Args.Contains(Execution::Args::Type::InstallLocation) &&
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
                path += '-';
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

            // https://learn.microsoft.com/en-us/windows/win32/msi/standard-installer-command-line-options
            if (context.Args.Contains(Execution::Args::Type::Silent))
            {
                args += " /quiet /norestart";
            }
            else if (!context.Args.Contains(Execution::Args::Type::Interactive))
            {
                args += " /passive /norestart";
            }

            return args;
        }
    }

    void ShellExecuteInstallImpl(Execution::Context& context)
    {
        context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

        const auto& installer = context.Get<Execution::Data::Installer>();
        const std::string& installerArgs = context.Get<Execution::Data::InstallerArgs>();

        // Inform of elevation requirements
        bool isElevated = Runtime::IsRunningAsAdmin();

        // The installer will run elevated, either by direct request or through the installer itself doing so.
        if ((installer->ElevationRequirement == ElevationRequirementEnum::ElevationRequired ||
            installer->ElevationRequirement == ElevationRequirementEnum::ElevatesSelf)
            && !isElevated)
        {
            context.Reporter.Warn() << Resource::String::InstallerElevationExpected << std::endl;
        }

        auto installResult = context.Reporter.ExecuteWithProgress(
            std::bind(InvokeShellExecuteEx,
                context.Get<Execution::Data::InstallerPath>(),
                installerArgs,
                installer->ElevationRequirement == ElevationRequirementEnum::ElevationRequired && !isElevated,
                std::placeholders::_1));

        if (!installResult)
        {
            context.Reporter.Warn() << Resource::String::InstallationAbandoned << std::endl;
            AICLI_TERMINATE_CONTEXT(E_ABORT);
        }
        else
        {
            context.Add<Execution::Data::OperationReturnCode>(installResult.value());
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
        else
        {
            context.Add<Execution::Data::OperationReturnCode>(uninstallResult.value());
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
            else
            {
                context.Add<Execution::Data::OperationReturnCode>(uninstallResult.value());

            }
        }
    }
}