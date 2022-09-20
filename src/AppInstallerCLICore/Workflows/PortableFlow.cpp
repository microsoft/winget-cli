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

        void AppendExeExtension(std::filesystem::path& value)
        {
            if (value.extension() != ".exe")
            {
                value += ".exe";
            }
        }

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

        std::filesystem::path GetPortableTargetFullPath(Execution::Context& context)
        {
            const std::filesystem::path& installerPath = context.Get<Execution::Data::InstallerPath>();
            const std::filesystem::path& targetInstallDirectory = GetPortableTargetDirectory(context);
            std::string_view renameArg = context.Args.GetArg(Execution::Args::Type::Rename);

            std::filesystem::path fileName;
            if (!renameArg.empty())
            {
                fileName = ConvertToUTF16(renameArg);
            }
            else
            {
                fileName = installerPath.filename();
            }

            AppendExeExtension(fileName);
            return targetInstallDirectory / fileName;
        }

        std::filesystem::path GetPortableSymlinkFullPath(Execution::Context& context)
        {
            const std::filesystem::path& installerPath = context.Get<Execution::Data::InstallerPath>();
            const std::vector<string_t>& commands = context.Get<Execution::Data::Installer>()->Commands;
            Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
            std::string_view renameArg = context.Args.GetArg(Execution::Args::Type::Rename);

            std::filesystem::path commandAlias;
            if (!renameArg.empty())
            {
                commandAlias = ConvertToUTF16(renameArg);
            }
            else
            {
                if (!commands.empty())
                {
                    commandAlias = ConvertToUTF16(commands[0]);
                }
                else
                {
                    commandAlias = installerPath.filename();
                }
            }

            AppendExeExtension(commandAlias);
            return GetPortableLinksLocation(scope) / commandAlias;
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

        void InitializePortableARPEntry(Execution::Context& context)
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
                    if (!context.Args.Contains(Execution::Args::Type::Force))
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

            portableEntry.Commit(PortableValueName::WinGetPackageIdentifier, portableEntry.WinGetPackageIdentifier = packageIdentifier);
            portableEntry.Commit(PortableValueName::WinGetSourceIdentifier, portableEntry.WinGetSourceIdentifier = sourceIdentifier);
            portableEntry.Commit(PortableValueName::UninstallString, portableEntry.UninstallString = "winget uninstall --product-code " + GetPortableProductCode(context));
            portableEntry.Commit(PortableValueName::WinGetInstallerType, portableEntry.WinGetInstallerType = InstallerTypeToString(InstallerTypeEnum::Portable));
        }
        
        void MovePortableExe(Execution::Context& context)
        {
            Portable::PortableEntry& portableEntry = context.Get<Execution::Data::PortableEntry>();
            const std::filesystem::path& installerPath = context.Get<Execution::Data::InstallerPath>();
            portableEntry.Commit(PortableValueName::PortableTargetFullPath, portableEntry.PortableTargetFullPath = GetPortableTargetFullPath(context));
            portableEntry.Commit(PortableValueName::InstallLocation, portableEntry.InstallLocation = GetPortableTargetDirectory(context));
            portableEntry.Commit(PortableValueName::SHA256, portableEntry.SHA256 = SHA256::ConvertToString(context.Get<Execution::Data::HashPair>().second));
            portableEntry.MovePortableExe(installerPath);
        }

        void RemovePortableExe(Execution::Context& context)
        {
            Portable::PortableEntry& portableEntry = context.Get<Execution::Data::PortableEntry>();
            const auto& targetPath = portableEntry.PortableTargetFullPath;

            if (std::filesystem::exists(targetPath))
            {
                if (!portableEntry.VerifyPortableExeHash())
                {
                    bool overrideHashMismatch = context.Args.Contains(Execution::Args::Type::Force);
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

        void CreatePortableSymlink(Execution::Context& context)
        {
            Portable::PortableEntry& portableEntry = context.Get<Execution::Data::PortableEntry>();
            if (portableEntry.InstallDirectoryAddedToPath)
            {
                AICLI_LOG(CLI, Info, << "Package directory was previously added to PATH. Skipping symlink creation.");
                return;
            }

            const std::filesystem::path& symlinkFullPath = GetPortableSymlinkFullPath(context);
            portableEntry.Commit(PortableValueName::PortableSymlinkFullPath, portableEntry.PortableSymlinkFullPath = symlinkFullPath);

            std::filesystem::file_status status = std::filesystem::status(symlinkFullPath);
            if (std::filesystem::is_directory(status))
            {
                AICLI_LOG(CLI, Info, << "Unable to create symlink. '" << symlinkFullPath << "points to an existing directory.");
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PORTABLE_SYMLINK_PATH_IS_DIRECTORY);
            }

            if (std::filesystem::remove(symlinkFullPath))
            {
                AICLI_LOG(CLI, Info, << "Removed existing file at " << symlinkFullPath);
                context.Reporter.Warn() << Resource::String::OverwritingExistingFileAtMessage << ' ' << symlinkFullPath.u8string() << std::endl;
            }

            portableEntry.CreatePortableSymlink();
        }

        void AddToPathVariable(Execution::Context& context)
        {
            Portable::PortableEntry& portableEntry = context.Get<Execution::Data::PortableEntry>();
            const std::filesystem::path& pathValue = portableEntry.GetPathValue();
            if (portableEntry.AddToPathVariable())
            {
                AICLI_LOG(CLI, Info, << "Appended target directory to PATH registry: " << pathValue);
                context.Reporter.Warn() << Resource::String::ModifiedPathRequiresShellRestart << std::endl;
            }
            else
            {
                AICLI_LOG(CLI, Info, << "Target directory already exists in PATH registry: " << pathValue);
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

        void CommitPortableMetadataToRegistry(Execution::Context& context)
        {
            Portable::PortableEntry& portableEntry = context.Get<Execution::Data::PortableEntry>();
            const AppInstaller::Manifest::Manifest& manifest = context.Get<Execution::Data::Manifest>();
            const Manifest::AppsAndFeaturesEntry& entry = GetAppsAndFeaturesEntryForPortableInstall(context.Get<Execution::Data::Installer>()->AppsAndFeaturesEntries, manifest);

            portableEntry.Commit(PortableValueName::DisplayName, portableEntry.DisplayName = entry.DisplayName);
            portableEntry.Commit(PortableValueName::DisplayVersion, portableEntry.DisplayVersion = entry.DisplayVersion);
            portableEntry.Commit(PortableValueName::Publisher, portableEntry.Publisher = entry.Publisher);
            portableEntry.Commit(PortableValueName::InstallDate, portableEntry.InstallDate = Utility::GetCurrentDateForARP());
            portableEntry.Commit(PortableValueName::URLInfoAbout, portableEntry.URLInfoAbout = manifest.CurrentLocalization.Get<Manifest::Localization::PackageUrl>());
            portableEntry.Commit(PortableValueName::HelpLink, portableEntry.HelpLink = manifest.CurrentLocalization.Get < Manifest::Localization::PublisherSupportUrl>());
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

    void PortableInstallImpl(Execution::Context& context)
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

        PortableARPEntry uninstallEntry = PortableARPEntry(
            scope,
            context.Get<Execution::Data::Installer>()->Arch,
            GetPortableProductCode(context));

        PortableEntry portableEntry = PortableEntry(uninstallEntry);
        context.Add<Execution::Data::PortableEntry>(std::move(portableEntry));

        try
        {
            context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

            context <<
                InitializePortableARPEntry <<
                MovePortableExe <<
                CreatePortableSymlink <<
                AddToPathVariable <<
                CommitPortableMetadataToRegistry;

            context.Add<Execution::Data::OperationReturnCode>(context.GetTerminationHR());
        }
        catch (...)
        {
            context.Add<Execution::Data::OperationReturnCode>(Workflow::HandleException(context, std::current_exception()));
        }

        // Reset termination to allow for ReportInstallResult to process return code.
        context.ResetTermination();

        // Perform cleanup only if the install fails and is not an update.
        const auto& installReturnCode = context.Get<Execution::Data::OperationReturnCode>();

        if (installReturnCode != 0 && installReturnCode != APPINSTALLER_CLI_ERROR_PORTABLE_PACKAGE_ALREADY_EXISTS && !isUpdate)
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