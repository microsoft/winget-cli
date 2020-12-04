// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "UninstallFlow.h"
#include "WorkflowBase.h"
#include "MSStoreInstallerHandler.h"
#include "ShellExecuteInstallerHandler.h"
#include "AppInstallerMsixInfo.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Msix;
using namespace AppInstaller::Repository;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        std::optional<DWORD> InvokeCreateProcess(const std::string& command, IProgressCallback& progress)
        {
            std::wstring commandUtf16 = Utility::ConvertToUTF16(command);

            // Parse the command string as application and command line for CreateProcess
            wil::unique_cotaskmem_string app = nullptr;
            wil::unique_cotaskmem_string commandLine = nullptr;
            THROW_IF_FAILED(SHEvaluateSystemCommandTemplate(commandUtf16.c_str(), &app, &commandLine, NULL));

            STARTUPINFOW startupInfo = { sizeof(startupInfo) };
            wil::unique_process_information processInfo = {};
            BOOL success = CreateProcessW(app.get(), commandLine.get(), NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo);

            if (!success)
            {
                return GetLastError();
            }

            // Wait for uninstallation to finish
            while (!progress.IsCancelled())
            {
                DWORD waitResult = WaitForSingleObject(processInfo.hProcess, 250);
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
                GetExitCodeProcess(processInfo.hProcess, &exitCode);
                return exitCode;
            }
        }
    }

    void GetUninstallInfo(Execution::Context& context)
    {
        auto installedPackageVersion = context.Get<Execution::Data::InstalledPackageVersion>();
        const std::string installedTypeString = installedPackageVersion->GetMetadata()[PackageVersionMetadata::InstalledType];
        switch (ManifestInstaller::ConvertToInstallerTypeEnum(installedTypeString))
        {
        case ManifestInstaller::InstallerTypeEnum::Exe:
        case ManifestInstaller::InstallerTypeEnum::Burn:
        case ManifestInstaller::InstallerTypeEnum::Inno:
        case ManifestInstaller::InstallerTypeEnum::Msi:
        case ManifestInstaller::InstallerTypeEnum::Nullsoft:
        case ManifestInstaller::InstallerTypeEnum::Wix:
        {
            IPackageVersion::Metadata packageMetadata = installedPackageVersion->GetMetadata();
            IPackageVersion::Metadata::iterator itr = packageMetadata.find(PackageVersionMetadata::StandardUninstallCommand);

            if (itr == packageMetadata.end() || context.Args.Contains(Execution::Args::Type::Silent))
            {
                itr = packageMetadata.find(PackageVersionMetadata::SilentUninstallCommand);
            }

            if (itr == packageMetadata.end())
            {
                context.Reporter.Error() << Resource::String::NoUninstallInfoFound << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_UNINSTALL_INFO_FOUND);
            }

            context.Add<Execution::Data::UninstallString>(itr->second);
            break;
        }
        case ManifestInstaller::InstallerTypeEnum::Msix:
        case ManifestInstaller::InstallerTypeEnum::MSStore:
        {
            auto packageFamilyNames = installedPackageVersion->GetMultiProperty(PackageVersionMultiProperty::PackageFamilyName);
            if (packageFamilyNames.empty())
            {
                context.Reporter.Error() << Resource::String::NoUninstallInfoFound << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_UNINSTALL_INFO_FOUND);
            }

            context.Add<Execution::Data::PackageFamilyNames>(packageFamilyNames);
            break;
        }
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }

    void ExecuteUninstaller(Execution::Context& context)
    {
        const std::string installedTypeString = context.Get<Execution::Data::InstalledPackageVersion>()->GetMetadata()[PackageVersionMetadata::InstalledType];
        switch (ManifestInstaller::ConvertToInstallerTypeEnum(installedTypeString))
        {
        case ManifestInstaller::InstallerTypeEnum::Exe:
        case ManifestInstaller::InstallerTypeEnum::Burn:
        case ManifestInstaller::InstallerTypeEnum::Inno:
        case ManifestInstaller::InstallerTypeEnum::Msi:
        case ManifestInstaller::InstallerTypeEnum::Nullsoft:
        case ManifestInstaller::InstallerTypeEnum::Wix:
            context << Workflow::ExecuteUninstallString;
            break;
        case ManifestInstaller::InstallerTypeEnum::Msix:
            context << Workflow::MsixUninstall;
            break;
        case ManifestInstaller::InstallerTypeEnum::MSStore:
            context <<
                Workflow::EnsureFeatureEnabled(Settings::ExperimentalFeature::Feature::ExperimentalMSStore) <<
                Workflow::MsixUninstall;
            break;
        default:
        THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }

    void ExecuteUninstallString(Execution::Context& context)
    {
        context.Reporter.Info() << Resource::String::UninstallFlowStartingPackageUninstall << std::endl;

        auto uninstallResult = context.Reporter.ExecuteWithProgress(
            std::bind(InvokeCreateProcess,
                context.Get<Execution::Data::UninstallString>(),
                std::placeholders::_1));

        if (!uninstallResult)
        {
            context.Reporter.Warn() << Resource::String::UninstallAbandoned << std::endl;
            AICLI_TERMINATE_CONTEXT(E_ABORT);
        }
        else if (uninstallResult.value() != 0)
        {
            // TODO: identify other success exit codes
            context.Reporter.Error() << Resource::String::UninstallFailedWithCode << ' ' << uninstallResult.value() << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_EXEC_UNINSTALL_COMMAND_FAILED);
        }
        else
        {
            context.Reporter.Info() << Resource::String::UninstallFlowUninstallSuccess << std::endl;
        }
    }

    void MsixUninstall(Execution::Context& context)
    {
        const auto& packageFamilyNames = context.Get<Execution::Data::PackageFamilyNames>(); 
        context.Reporter.Info() << Resource::String::UninstallFlowStartingPackageUninstall << std::endl;

        for (const auto& packageFamilyName : packageFamilyNames)
        {
            auto packageFullName = Msix::GetPackageFullNameFromFamilyName(packageFamilyName);
            if (packageFullName.has_value())
            {
                AICLI_LOG(CLI, Info, << "Removing MSIX package: " << packageFullName.value());
                context.Reporter.ExecuteWithProgress(std::bind(Deployment::RemovePackage, packageFullName.value(), std::placeholders::_1));
            }
        }

        context.Reporter.Info() << Resource::String::UninstallFlowUninstallSuccess << std::endl;
    }
}