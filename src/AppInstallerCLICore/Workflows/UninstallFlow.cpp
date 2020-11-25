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
            // TODO: this probably can be done better
            LPWSTR commandUtf16 = _wcsdup(Utility::ConvertToUTF16(command).c_str());

            // TODO: review parameters
            STARTUPINFOW startupInfo = { sizeof(startupInfo) };
            PROCESS_INFORMATION processInfo;
            BOOL success = CreateProcessW(
                NULL,         // lpApplicationName
                commandUtf16, // lpCommandLine
                NULL,         // lpProcessAttributes
                NULL,         // lpThreadAttributes
                FALSE,        // bInheritAttributes
                0,            // dwCreationFlags
                NULL,         // lpEnvironment - use parent's
                NULL,         // lpCurrentDirectory - use parent's
                &startupInfo,
                &processInfo);

            free(commandUtf16);

            if (!success)
            {
                return GetLastError();
            }

            CloseHandle(processInfo.hThread);
            wil::unique_process_handle process{ processInfo.hProcess };

            // Wait for uninstallation to finish
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
    }

    void GetUninstallInfo(Execution::Context& context)
    {
        auto installedPackageVersion = context.Get<Execution::Data::InstalledPackageVersion>();

        // Note: this only exists for EXE/MSI
        // TODO: check other flags & settings
        IPackageVersion::Metadata packageMetadata = installedPackageVersion->GetMetadata();
        if (context.Args.Contains(Execution::Args::Type::Silent))
        {
            context.Add<Execution::Data::UninstallCommand>(packageMetadata[PackageVersionMetadata::SilentUninstallCommand]);
        }
        else
        {
            context.Add<Execution::Data::UninstallCommand>(packageMetadata[PackageVersionMetadata::StandardUninstallCommand]);
        }
    }

    void ExecuteUninstaller(Execution::Context& context)
    {
        const std::string insalledTypeString = context.Get<Execution::Data::InstalledPackageVersion>()->GetMetadata()[PackageVersionMetadata::InstalledType];
        switch (ManifestInstaller::ConvertToInstallerTypeEnum(insalledTypeString))
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
                Workflow::EnsureStorePolicySatisfied <<
                Workflow::MSStoreUninstall;
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
                context.Get<Execution::Data::UninstallCommand>(),
                std::placeholders::_1));

        if (!uninstallResult)
        {
            // TODO: log & report
            AICLI_TERMINATE_CONTEXT(E_ABORT);
        }
        else if (uninstallResult.value() != 0)
        {
            // TODO: log & report
            // TODO: set appropriate error
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INTERNAL_ERROR);
        }
        else
        {
            context.Reporter.Info() << Resource::String::UninstallFlowUninstallSuccess << std::endl;
        }
    }

    void MsixUninstall(Execution::Context& context)
    {
        auto packageFamilyNames = context.Get<Execution::Data::InstalledPackageVersion>()->GetMultiProperty(PackageVersionMultiProperty::PackageFamilyName);
        context.Reporter.Info() << Resource::String::UninstallFlowStartingPackageUninstall << std::endl;

        try
        {
            for (auto pfn : packageFamilyNames)
            {
                auto fullname = Msix::GetPackageFullNameFromFamilyName(pfn);
                if (fullname.has_value())
                {
                    context.Reporter.ExecuteWithProgress(std::bind(Deployment::RemovePackage, fullname.value(), std::placeholders::_1));
                }
            }
        }
        catch (const wil::ResultException& re)
        {
            // TODO: telemetry?

            context.Reporter.Error() << GetUserPresentableMessage(re) << std::endl;
            AICLI_TERMINATE_CONTEXT(re.GetErrorCode());
        }

        context.Reporter.Info() << Resource::String::UninstallFlowUninstallSuccess << std::endl;
    }
}