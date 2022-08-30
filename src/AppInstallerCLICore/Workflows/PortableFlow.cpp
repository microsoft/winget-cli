// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PortableFlow.h"
#include "WorkflowBase.h"
#include "winget/Filesystem.h"
#include "winget/PortableARPEntry.h"
#include "winget/PortableEntry.h"
#include "winget/PathVariable.h"
#include "AppInstallerStrings.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Portable;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Registry;
using namespace AppInstaller::Registry::Portable;
using namespace AppInstaller::Registry::Environment;
using namespace AppInstaller::Repository;
using namespace std::filesystem;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        constexpr std::string_view s_DefaultSource = "*DefaultSource"sv;

        std::string GetPortableProductCode(Execution::Context& context)
        {
            const std::string& packageId = context.Get<Execution::Data::Manifest>().Id;

            std::string source;
            if (context.Contains(Execution::Data::PackageVersion))
            {
                source = context.Get<Execution::Data::PackageVersion>()->GetSource().GetIdentifier();
            }
            else
            {
                source = s_DefaultSource;
            }

            return MakeSuitablePathPart(packageId + "_" + source);
        }

        std::filesystem::path GetPortableInstallRoot(Manifest::ScopeEnum scope, Utility::Architecture arch)
        {
            if (scope == Manifest::ScopeEnum::Machine)
            {
                if (arch == Utility::Architecture::X86)
                {
                    return Runtime::GetPathTo(Runtime::PathName::PortablePackageMachineRootX86);
                }
                else
                {
                    return Runtime::GetPathTo(Runtime::PathName::PortablePackageMachineRootX64);
                }
            }
            else
            {
                return Runtime::GetPathTo(Runtime::PathName::PortablePackageUserRoot);
            }
        }

        std::filesystem::path GetPortableLinksLocation(Manifest::ScopeEnum scope)
        {
            if (scope == Manifest::ScopeEnum::Machine)
            {
                return Runtime::GetPathTo(Runtime::PathName::PortableLinksMachineLocation);
            }
            else
            {
                return Runtime::GetPathTo(Runtime::PathName::PortableLinksUserLocation);
            }
        }



        Manifest::AppsAndFeaturesEntry GetAppsAndFeaturesEntryForPortableInstall(
            const std::vector<AppInstaller::Manifest::AppsAndFeaturesEntry>& appsAndFeaturesEntries,
            const AppInstaller::Manifest::Manifest& manifest)
        {
            AppInstaller::Manifest::AppsAndFeaturesEntry appsAndFeaturesEntry;
            if (!appsAndFeaturesEntries.empty())
            {
                appsAndFeaturesEntry = appsAndFeaturesEntries[0];
            }

            if (appsAndFeaturesEntry.DisplayName.empty())
            {
                appsAndFeaturesEntry.DisplayName = manifest.CurrentLocalization.Get<Manifest::Localization::PackageName>();
            }
            if (appsAndFeaturesEntry.DisplayVersion.empty())
            {
                appsAndFeaturesEntry.DisplayVersion = manifest.Version;
            }
            if (appsAndFeaturesEntry.Publisher.empty())
            {
                appsAndFeaturesEntry.Publisher = manifest.CurrentLocalization.Get<Manifest::Localization::Publisher>();
            }

            return appsAndFeaturesEntry;
        }

        void RemovePortableExe(Execution::Context& context)
        {
            Portable::PortableEntry& portableEntry = context.Get<Execution::Data::PortableEntry>();
            const auto& targetPath = portableEntry.PortableTargetFullPath;

            if (std::filesystem::exists(targetPath))
            {
                if (!portableEntry.VerifyPortableExeHash())
                {
                    bool overrideHashMismatch = context.Args.Contains(Execution::Args::Type::HashOverride);
                    if (overrideHashMismatch)
                    {
                        context.Reporter.Warn() << Resource::String::PortableHashMismatchOverridden << std::endl;
                    }
                    else
                    {
                        context.Reporter.Warn() << Resource::String::PortableHashMismatchOverrideRequired << std::endl;
                        AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PORTABLE_UNINSTALL_FAILED);
                    }
                }

                std::filesystem::remove(targetPath);
                AICLI_LOG(CLI, Info, << "Successfully deleted portable exe:" << targetPath);
            }
            else
            {
                AICLI_LOG(CLI, Info, << "Portable exe not found; Unable to delete portable exe: " << targetPath);
            }
        }

        void RemoveInstallDirectory(Execution::Context& context)
        {
            Portable::PortableEntry& portableEntry = context.Get<Execution::Data::PortableEntry>();
            const auto& installDirectory = portableEntry.InstallLocation;

            if (std::filesystem::exists(installDirectory))
            {
                const auto& isCreated = portableEntry.InstallDirectoryCreated;
                bool isUpdate = WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerExecutionUseUpdate);

                if (context.Args.Contains(Execution::Args::Type::Purge) ||
                    (!isUpdate && Settings::User().Get<Settings::Setting::UninstallPurgePortablePackage>() && !context.Args.Contains(Execution::Args::Type::Preserve)))
                {
                    if (isCreated)
                    {
                        context.Reporter.Warn() << Resource::String::PurgeInstallDirectory << std::endl;
                        const auto& removedFilesCount = std::filesystem::remove_all(installDirectory);
                        AICLI_LOG(CLI, Info, << "Purged install location directory. Deleted " << removedFilesCount << " files or directories");
                    }
                    else
                    {
                        context.Reporter.Warn() << Resource::String::UnableToPurgeInstallDirectory << std::endl;
                    }

                }
                else if (std::filesystem::is_empty(installDirectory))
                {
                    if (isCreated)
                    {
                        std::filesystem::remove(installDirectory);
                        AICLI_LOG(CLI, Info, << "Install directory deleted: " << installDirectory);
                    }
                }
                else
                {
                    context.Reporter.Warn() << Resource::String::FilesRemainInInstallDirectory << installDirectory << std::endl;
                }
            }
            else
            {
                AICLI_LOG(CLI, Info, << "Install directory does not exist: " << installDirectory);
            }
        }

        void RemoveFromPathVariable(Execution::Context& context)
        {
            Portable::PortableEntry& portableEntry = context.Get<Execution::Data::PortableEntry>();
            const std::filesystem::path& pathValue = portableEntry.GetPathValue();
            if (portableEntry.RemoveFromPathVariable())
            {
                AICLI_LOG(CLI, Info, << "Removed target directory from PATH registry: " << pathValue);
            }
            else
            {
                AICLI_LOG(CLI, Info, << "Target directory not removed from PATH registry: " << pathValue);
            }
        }

        void RemovePortableSymlink(Execution::Context& context)
        {
            Portable::PortableEntry& portableEntry = context.Get<Execution::Data::PortableEntry>();
            const auto& symlinkPath = portableEntry.PortableSymlinkFullPath;

            if (!std::filesystem::is_symlink(std::filesystem::symlink_status(symlinkPath)))
            {
                AICLI_LOG(Core, Info, << "The registry value for [PortableSymlinkFullPath] does not point to a valid symlink file.");
                return;
            }

            if (portableEntry.VerifySymlinkTarget())
            {
                if (!std::filesystem::remove(symlinkPath))
                {
                    AICLI_LOG(CLI, Info, << "Portable symlink not found; Unable to delete portable symlink: " << symlinkPath);
                }
            }
            else
            {
                context.Reporter.Warn() << Resource::String::SymlinkModified << std::endl;
            }
        }

        void RemovePortableARPEntry(Execution::Context& context)
        {
            Portable::PortableEntry& portableEntry = context.Get<Execution::Data::PortableEntry>();
            portableEntry.RemoveARPEntry();
            AICLI_LOG(CLI, Info, << "PortableARPEntry deleted.");
        }

        void EnsureValidArgsForPortableInstall(Execution::Context& context)
        {
            std::string_view renameArg = context.Args.GetArg(Execution::Args::Type::Rename);

            try
            {
                if (MakeSuitablePathPart(renameArg) != renameArg)
                {
                    context.Reporter.Error() << Resource::String::ReservedFilenameError << std::endl;
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS);
                }
            }
            catch (...)
            {
                context.Reporter.Error() << Resource::String::ReservedFilenameError << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS);
            }
        }

        void EnsureVolumeSupportsReparsePoints(Execution::Context& context)
        {
            Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
            const std::filesystem::path& symlinkDirectory = GetPortableLinksLocation(scope);

            if (!AppInstaller::Filesystem::SupportsReparsePoints(symlinkDirectory))
            {
                context.Reporter.Error() << Resource::String::ReparsePointsNotSupportedError << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PORTABLE_REPARSE_POINT_NOT_SUPPORTED);
            }
        }

        void EnsureRunningAsAdminForMachineScopeInstall(Execution::Context& context)
        {
            // Admin is required for machine scope install or else creating a symlink in the %PROGRAMFILES% link location will fail.
            Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
            if (scope == Manifest::ScopeEnum::Machine)
            {
                context << Workflow::EnsureRunningAsAdmin;
            }
        }
    }

    std::filesystem::path GetPortableTargetDirectory(Execution::Context& context)
    {
        Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
        Utility::Architecture arch = context.Get<Execution::Data::Installer>()->Arch;
        std::string_view locationArg = context.Args.GetArg(Execution::Args::Type::InstallLocation);
        std::filesystem::path targetInstallDirectory;

        if (!locationArg.empty())
        {
            targetInstallDirectory = std::filesystem::path{ ConvertToUTF16(locationArg) };
        }
        else
        {
            const std::string& productCode = GetPortableProductCode(context);
            targetInstallDirectory = GetPortableInstallRoot(scope, arch);
            targetInstallDirectory /= ConvertToUTF16(productCode);
        }

        return targetInstallDirectory;
    }

    void VerifyPortableRegistryMatch(Execution::Context& context)
    {
        const std::string& packageIdentifier = context.Get<Execution::Data::Manifest>().Id;

        std::string sourceIdentifier;
        if (context.Contains(Execution::Data::PackageVersion))
        {
            sourceIdentifier = context.Get<Execution::Data::PackageVersion>()->GetSource().GetIdentifier();
        }
        else
        {
            sourceIdentifier = s_DefaultSource;
        }

        Portable::PortableEntry& portableEntry = context.Get<Execution::Data::PortableEntry>();

        if (portableEntry.Exists())
        {
            if (packageIdentifier != portableEntry.WinGetPackageIdentifier || sourceIdentifier != portableEntry.WinGetSourceIdentifier)
            {
                // TODO: Replace HashOverride with --Force when argument behavior gets updated.
                if (!context.Args.Contains(Execution::Args::Type::HashOverride))
                {
                    AICLI_LOG(CLI, Error, << "Registry match failed, skipping write to uninstall registry");
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PORTABLE_PACKAGE_ALREADY_EXISTS);
                }
                else
                {
                    AICLI_LOG(CLI, Info, << "Overriding registry match check...");
                    context.Reporter.Warn() << Resource::String::PortableRegistryCollisionOverridden << std::endl;
                }
            }
        }

        portableEntry.WinGetPackageIdentifier = packageIdentifier;
        portableEntry.WinGetSourceIdentifier = sourceIdentifier;
    }


    void GetPortableInstallInfo(Execution::Context& context)
    {
        Manifest::ScopeEnum scope = Manifest::ScopeEnum::Unknown;
        bool isUpdate = WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerExecutionUseUpdate);
        if (isUpdate)
        {
            IPackageVersion::Metadata installationMetadata = context.Get<Execution::Data::InstalledPackageVersion>()->GetMetadata();
            auto installerScopeItr = installationMetadata.find(Repository::PackageVersionMetadata::InstalledScope);
            if (installerScopeItr != installationMetadata.end())
            {
                scope = Manifest::ConvertToScopeEnum(installerScopeItr->second);
            }
        }
        else
        {
            scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
        }

        PortableEntry portableEntry = PortableEntry(scope, context.Get<Execution::Data::Installer>()->Arch, GetPortableProductCode(context), isUpdate);
        portableEntry.InstallLocation = GetPortableTargetDirectory(context);
        portableEntry.SHA256 = SHA256::ConvertToString(context.Get<Execution::Data::HashPair>().second);

        const AppInstaller::Manifest::Manifest& manifest = context.Get<Execution::Data::Manifest>();
        const Manifest::AppsAndFeaturesEntry& entry = GetAppsAndFeaturesEntryForPortableInstall(context.Get<Execution::Data::Installer>()->AppsAndFeaturesEntries, manifest);
        portableEntry.SetAppsAndFeaturesMetadata(entry, manifest);
        context.Add<Execution::Data::PortableEntry>(std::move(portableEntry));
    }

    void PortableInstallImpl(Execution::Context& context)
    {
        Portable::PortableEntry& portableEntry = context.Get<Execution::Data::PortableEntry>();

        try
        {
            context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

            HRESULT result;
            const std::filesystem::path& installerPath = context.Get<Execution::Data::InstallerPath>();

            if (IsArchiveType(context.Get<Execution::Data::Installer>()->BaseInstallerType))
            {
                const std::vector<std::filesystem::path>& extractedItems = context.Get<Execution::Data::ExtractedItems>();
                const std::vector<Manifest::NestedInstallerFile>& nestedInstallerFiles = context.Get<Execution::Data::Installer>()->NestedInstallerFiles;
                result = portableEntry.MultipleInstall(nestedInstallerFiles, extractedItems);
            }
            else
            {
                std::filesystem::path commandAlias;
                bool rename = false;
                std::string_view renameArg = context.Args.GetArg(Execution::Args::Type::Rename);
                const std::vector<string_t>& commands = context.Get<Execution::Data::Installer>()->Commands;

                if (!renameArg.empty())
                {
                    rename = true;
                    commandAlias = ConvertToUTF16(renameArg);
                }
                else if (!commands.empty())
                {
                    commandAlias = ConvertToUTF16(commands[0]);
                }
                else
                {
                    commandAlias = installerPath.filename();
                }

                result = portableEntry.SingleInstall(installerPath, commandAlias, rename);
            }

            context.Add<Execution::Data::OperationReturnCode>(result);
        }
        catch (...)
        {
            // fix this to handle.
            context.Add<Execution::Data::OperationReturnCode>(Workflow::HandleException(context, std::current_exception()));
        }

        // Reset termination to allow for ReportInstallResult to process return code.
        context.ResetTermination();

        // Perform cleanup only if the install fails and is not an update.
        const auto& installReturnCode = context.Get<Execution::Data::OperationReturnCode>();

        if (installReturnCode != 0 && installReturnCode != APPINSTALLER_CLI_ERROR_PORTABLE_PACKAGE_ALREADY_EXISTS && !portableEntry.IsUpdate())
        {
            context.Reporter.Warn() << Resource::String::PortableInstallFailed << std::endl;
            auto uninstallPortableContextPtr = context.CreateSubContext();
            Execution::Context& uninstallPortableContext = *uninstallPortableContextPtr;
            auto previousThreadGlobals = uninstallPortableContext.SetForCurrentThread();

            uninstallPortableContext.Add<Execution::Data::PortableEntry>(context.Get<Execution::Data::PortableEntry>());
            uninstallPortableContext << PortableUninstallImpl;
        }
    }

    void PortableUninstallImpl(Execution::Context& context)
    {
        try
        {
            context.Reporter.Info() << Resource::String::UninstallFlowStartingPackageUninstall << std::endl;

            context <<
                RemovePortableExe <<
                RemoveInstallDirectory <<
                RemovePortableSymlink <<
                RemoveFromPathVariable <<
                RemovePortableARPEntry;

            context.Add<Execution::Data::OperationReturnCode>(context.GetTerminationHR());
        }
        catch (...)
        {
            context.Add<Execution::Data::OperationReturnCode>(Workflow::HandleException(context, std::current_exception()));
        }

        // Reset termination to allow for ReportUninstallResult to process return code.
        context.ResetTermination();
    }

    void EnsureSupportForPortableInstall(Execution::Context& context)
    {
        auto installerType = context.Get<Execution::Data::Installer>().value().EffectiveInstallerType();

        if (installerType == InstallerTypeEnum::Portable)
        {
            context <<
                EnsureRunningAsAdminForMachineScopeInstall <<
                EnsureValidArgsForPortableInstall <<
                EnsureVolumeSupportsReparsePoints;
        }
    }

    void EnsureSupportForPortableUninstall(Execution::Context& context)
    {
        auto installedPackageVersion = context.Get<Execution::Data::InstalledPackageVersion>();
        const std::string installedTypeString = installedPackageVersion->GetMetadata()[PackageVersionMetadata::InstalledType];
        if (ConvertToInstallerTypeEnum(installedTypeString) == InstallerTypeEnum::Portable)
        {
            const std::string installedScope = installedPackageVersion->GetMetadata()[Repository::PackageVersionMetadata::InstalledScope];
            if (ConvertToScopeEnum(installedScope) == Manifest::ScopeEnum::Machine)
            {
                context << EnsureRunningAsAdmin;
            }
        }
    }

    // TODO: remove this check once support for portable in archive has been implemented
    void EnsureNonPortableTypeForArchiveInstall(Execution::Context& context)
    {
        auto nestedInstallerType = context.Get<Execution::Data::Installer>().value().NestedInstallerType;

        if (nestedInstallerType == InstallerTypeEnum::Portable)
        {
            context.Reporter.Error() << Resource::String::PortableInstallFromArchiveNotSupported << std::endl;
            AICLI_TERMINATE_CONTEXT(ERROR_NOT_SUPPORTED);
        }
    }
}