// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "UninstallFlow.h"
#include "WorkflowBase.h"
#include "DependenciesFlow.h"
#include "ShellExecuteInstallerHandler.h"
#include "AppInstallerMsixInfo.h"
#include "PortableFlow.h"
#include "winget/PortableARPEntry.h"

#include <AppInstallerDeployment.h>

using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Msix;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Registry;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        // Helper for RecordUninstall
        struct UninstallCorrelatedSources
        {
            struct Item
            {
                Utility::LocIndString Identifier;
                Source FromSource;
                std::string SourceIdentifier;
            };

            void AddIfRemoteAndNotPresent(const std::shared_ptr<IPackageVersion>& packageVersion)
            {
                auto source = packageVersion->GetSource();
                const auto details = source.GetDetails();
                if (!source.ContainsAvailablePackages())
                {
                    return;
                }

                for (const auto& item : Items)
                {
                    if (item.SourceIdentifier == details.Identifier)
                    {
                        return;
                    }
                }

                Items.emplace_back(Item{ packageVersion->GetProperty(PackageVersionProperty::Id), std::move(source), details.Identifier });
            }

            std::vector<Item> Items;
        };
    }

    void UninstallSinglePackage(Execution::Context& context)
    {
        context <<
            Workflow::GetInstalledPackageVersion <<
            Workflow::GetUninstallInfo <<
            Workflow::GetDependenciesInfoForUninstall <<
            Workflow::ReportDependencies(Resource::String::UninstallCommandReportDependencies) <<
            Workflow::ReportExecutionStage(ExecutionStage::Execution) <<
            Workflow::ExecuteUninstaller <<
            Workflow::ReportExecutionStage(ExecutionStage::PostExecution) <<
            Workflow::RecordUninstall;
    }

    void GetUninstallInfo(Execution::Context& context)
    {
        auto installedPackageVersion = context.Get<Execution::Data::InstalledPackageVersion>();
        const std::string installedTypeString = installedPackageVersion->GetMetadata()[PackageVersionMetadata::InstalledType];
        switch (ConvertToInstallerTypeEnum(installedTypeString))
        {
        case InstallerTypeEnum::Exe:
        case InstallerTypeEnum::Burn:
        case InstallerTypeEnum::Inno:
        case InstallerTypeEnum::Nullsoft:
        {
            IPackageVersion::Metadata packageMetadata = installedPackageVersion->GetMetadata();

            // Default to silent unless it is not present or interactivity is requested
            auto uninstallCommandItr = packageMetadata.find(PackageVersionMetadata::SilentUninstallCommand);
            if (uninstallCommandItr == packageMetadata.end() || context.Args.Contains(Execution::Args::Type::Interactive))
            {
                auto interactiveItr = packageMetadata.find(PackageVersionMetadata::StandardUninstallCommand);
                if (interactiveItr != packageMetadata.end())
                {
                    uninstallCommandItr = interactiveItr;
                }
            }

            if (uninstallCommandItr == packageMetadata.end())
            {
                context.Reporter.Error() << Resource::String::NoUninstallInfoFound << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_UNINSTALL_INFO_FOUND);
            }

            context.Add<Execution::Data::UninstallString>(uninstallCommandItr->second);
            break;
        }
        case InstallerTypeEnum::Msi:
        case InstallerTypeEnum::Wix:
        {
            // Uninstall strings for MSI don't include UI level (/q) needed to avoid interactivity,
            // so we handle them differently.
            auto productCodes = installedPackageVersion->GetMultiProperty(PackageVersionMultiProperty::ProductCode);
            if (productCodes.empty())
            {
                context.Reporter.Error() << Resource::String::NoUninstallInfoFound << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_UNINSTALL_INFO_FOUND);
            }

            context.Add<Execution::Data::ProductCodes>(std::move(productCodes));
            break;
        }
        case InstallerTypeEnum::Msix:
        case InstallerTypeEnum::MSStore:
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
        case InstallerTypeEnum::Portable:
        {
            auto productCodes = installedPackageVersion->GetMultiProperty(PackageVersionMultiProperty::ProductCode);
            if (productCodes.empty())
            {
                context.Reporter.Error() << Resource::String::NoUninstallInfoFound << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_UNINSTALL_INFO_FOUND);
            }

            const std::string installedScope = context.Get<Execution::Data::InstalledPackageVersion>()->GetMetadata()[Repository::PackageVersionMetadata::InstalledScope];
            const std::string installedArch = context.Get<Execution::Data::InstalledPackageVersion>()->GetMetadata()[Repository::PackageVersionMetadata::InstalledArchitecture];
            Portable::PortableARPEntry uninstallEntry = Portable::PortableARPEntry(
                ConvertToScopeEnum(installedScope),
                Utility::ConvertToArchitectureEnum(installedArch),
                productCodes[0]);
            context.Add<Execution::Data::PortableARPEntry>(uninstallEntry);
            break;
        }
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }

    void ExecuteUninstaller(Execution::Context& context)
    {
        const std::string installedTypeString = context.Get<Execution::Data::InstalledPackageVersion>()->GetMetadata()[PackageVersionMetadata::InstalledType];
        switch (ConvertToInstallerTypeEnum(installedTypeString))
        {
        case InstallerTypeEnum::Exe:
        case InstallerTypeEnum::Burn:
        case InstallerTypeEnum::Inno:
        case InstallerTypeEnum::Nullsoft:
            context <<
                Workflow::ShellExecuteUninstallImpl <<
                ReportUninstallerResult("UninstallString", APPINSTALLER_CLI_ERROR_EXEC_UNINSTALL_COMMAND_FAILED);
            break;
        case InstallerTypeEnum::Msi:
        case InstallerTypeEnum::Wix:
            context <<
                Workflow::ShellExecuteMsiExecUninstall <<
                ReportUninstallerResult("MsiExec", APPINSTALLER_CLI_ERROR_EXEC_UNINSTALL_COMMAND_FAILED);
            break;
        case InstallerTypeEnum::Msix:
        case InstallerTypeEnum::MSStore:
            context << Workflow::MsixUninstall;
            break;
        case InstallerTypeEnum::Portable:
            context <<
                Workflow::PortableUninstallImpl <<
                ReportUninstallerResult("PortableUninstall"sv, APPINSTALLER_CLI_ERROR_PORTABLE_UNINSTALL_FAILED, true);
            break;
        default:
        THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }

    void MsixUninstall(Execution::Context& context)
    {
        const auto& packageFamilyNames = context.Get<Execution::Data::PackageFamilyNames>();
        context.Reporter.Info() << Resource::String::UninstallFlowStartingPackageUninstall << std::endl;

        for (const auto& packageFamilyName : packageFamilyNames)
        {
            auto packageFullName = Msix::GetPackageFullNameFromFamilyName(packageFamilyName);
            if (!packageFullName.has_value())
            {
                AICLI_LOG(CLI, Warning, << "No package found with family name: " << packageFamilyName);
                continue;
            }

            AICLI_LOG(CLI, Info, << "Removing MSIX package: " << packageFullName.value());
            try
            {
                context.Reporter.ExecuteWithProgress(std::bind(Deployment::RemovePackage, packageFullName.value(), std::placeholders::_1));
            }
            catch (const wil::ResultException& re)
            {
                context.Add<Execution::Data::OperationReturnCode>(re.GetErrorCode());
                context << ReportUninstallerResult("MSIXUninstall"sv, re.GetErrorCode(), /* isHResult */ true);
                return;
            }
        }

        context.Reporter.Info() << Resource::String::UninstallFlowUninstallSuccess << std::endl;
    }

    void RecordUninstall(Context& context)
    {
        // In order to report an uninstall to every correlated tracking catalog, we first need to find them all.
        auto package = context.Get<Data::Package>();
        UninstallCorrelatedSources correlatedSources;

        // Start with the installed version
        correlatedSources.AddIfRemoteAndNotPresent(package->GetInstalledVersion());

        // Then look through all available versions
        for (const auto& versionKey : package->GetAvailableVersionKeys())
        {
            correlatedSources.AddIfRemoteAndNotPresent(package->GetAvailableVersion(versionKey));
        }

        // Finally record the uninstall for each found value
        for (const auto& item : correlatedSources.Items)
        {
            auto trackingCatalog = item.FromSource.GetTrackingCatalog();
            trackingCatalog.RecordUninstall(item.Identifier);
        }
    }

    void ReportUninstallerResult::operator()(Execution::Context& context) const
    {
        DWORD uninstallResult = context.Get<Execution::Data::OperationReturnCode>();
        if (uninstallResult != 0)
        {
            const auto installedPackageVersion = context.Get<Execution::Data::InstalledPackageVersion>();
            Logging::Telemetry().LogUninstallerFailure(
                installedPackageVersion->GetProperty(PackageVersionProperty::Id),
                installedPackageVersion->GetProperty(PackageVersionProperty::Version),
                m_uninstallerType,
                uninstallResult);

            if (m_isHResult)
            {
                context.Reporter.Error() << Resource::String::UninstallFailedWithCode << ' ' << GetUserPresentableMessage(uninstallResult) << std::endl;
            }
            else
            {
                context.Reporter.Error() << Resource::String::UninstallFailedWithCode << ' ' << uninstallResult << std::endl;
            }

            // Show installer log path if exists
            if (context.Contains(Execution::Data::LogPath) && std::filesystem::exists(context.Get<Execution::Data::LogPath>()))
            {
                context.Reporter.Info() << Resource::String::InstallerLogAvailable << ' ' << context.Get<Execution::Data::LogPath>().u8string() << std::endl;
            }

            AICLI_TERMINATE_CONTEXT(m_hr);
        }
        else
        {
            context.Reporter.Info() << Resource::String::UninstallFlowUninstallSuccess << std::endl;
        }
    }
}
