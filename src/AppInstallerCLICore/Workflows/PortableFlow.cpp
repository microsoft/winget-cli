// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PortableFlow.h"
#include "PortableInstaller.h"
#include "WorkflowBase.h"
#include "winget/Filesystem.h"
#include "winget/PortableFileEntry.h"
#include <Microsoft/PortableIndex.h>

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Utility;
using namespace AppInstaller::CLI::Portable;
using namespace AppInstaller::Portable;

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

            AppInstaller::Filesystem::AppendExtension(fileName, ".exe");
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

            AppInstaller::Filesystem::AppendExtension(commandAlias, ".exe");
            return GetPortableLinksLocation(scope) / commandAlias;
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

            targetInstallDirectory = GetPortableInstallRoot(scope, arch);
        }

        const std::string& productCode = GetPortableProductCode(context);
        targetInstallDirectory /= ConvertToUTF16(productCode);

        return targetInstallDirectory;
    }

    void VerifyPackageAndSourceMatch(Execution::Context& context)
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

        PortableInstaller& portableInstaller = context.Get<Execution::Data::PortableInstaller>();
        if (portableInstaller.ARPEntryExists())
        {
            if (packageIdentifier != portableInstaller.WinGetPackageIdentifier || sourceIdentifier != portableInstaller.WinGetSourceIdentifier)
            {
                // TODO: Replace HashOverride with --Force when argument behavior gets updated.
                if (!context.Args.Contains(Execution::Args::Type::HashOverride))
                {
                    AICLI_LOG(CLI, Error, << "Registry match failed, skipping write to uninstall registry");
                    context.Reporter.Error() << Resource::String::PortablePackageAlreadyExists << std::endl;
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PORTABLE_PACKAGE_ALREADY_EXISTS);
                }
                else
                {
                    AICLI_LOG(CLI, Info, << "Overriding registry match check...");
                    context.Reporter.Warn() << Resource::String::PortableRegistryCollisionOverridden << std::endl;
                }
            }
        }

        portableInstaller.WinGetPackageIdentifier = packageIdentifier;
        portableInstaller.WinGetSourceIdentifier = sourceIdentifier;
    }

    void InitializePortableInstaller(Execution::Context& context)
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
            scope = Manifest::ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
        }

        Utility::Architecture arch = context.Get<Execution::Data::Installer>()->Arch;
        const std::string& productCode = GetPortableProductCode(context);

        PortableInstaller portableInstaller = PortableInstaller(scope, arch, productCode);
        portableInstaller.SHA256 = Utility::SHA256::ConvertToString(context.Get<Execution::Data::HashPair>().second);
        portableInstaller.InstallLocation = GetPortableTargetDirectory(context);
        portableInstaller.IsUpdate = isUpdate;

        if (!IsArchiveType(context.Get<Execution::Data::Installer>()->BaseInstallerType))
        {
            portableInstaller.PortableTargetFullPath = GetPortableTargetFullPath(context);
            portableInstaller.PortableSymlinkFullPath = GetPortableSymlinkFullPath(context);
        }

        portableInstaller.SetAppsAndFeaturesMetadata(context.Get<Execution::Data::Manifest>(), context.Get<Execution::Data::Installer>()->AppsAndFeaturesEntries);
        context.Add<Execution::Data::PortableInstaller>(std::move(portableInstaller));
    }

    std::vector<PortableFileEntry> GetDesiredStateForPortableInstall(Execution::Context& context)
    {
        std::filesystem::path& installerPath = context.Get<Execution::Data::InstallerPath>();
        PortableInstaller& portableInstaller = context.Get<Execution::Data::PortableInstaller>();
        std::vector<PortableFileEntry> entries;

        // The order that we create these state matters. For install the order should be:
        // 1. Install Root Directory
        // 2. Files and Directories
        // 3. Symlinks
        // Uninstall should process these portable files in the reverse order for proper cleanup.

        const std::filesystem::path& installLocation = GetPortableTargetDirectory(context);
        entries.emplace_back(std::move(PortableFileEntry::CreateDirectoryEntry(installLocation)));

        if (std::filesystem::is_directory(installerPath))
        {
            std::vector<PortableFileEntry> directoryEntries;
            std::vector<PortableFileEntry> fileEntries;

            for (const auto& entry : std::filesystem::directory_iterator(installerPath))
            {
                std::filesystem::path entryPath = entry.path();
                PortableFileEntry portableFile;
                if (std::filesystem::is_directory(entryPath))
                {
                    directoryEntries.emplace_back(std::move(PortableFileEntry::CreateDirectoryEntry(entryPath)));
                }
                else
                {
                    std::filesystem::path relativePath = std::filesystem::relative(entryPath, entryPath.parent_path());
                    fileEntries.emplace_back(std::move(PortableFileEntry::CreateFileEntry(entryPath, installLocation / relativePath)));
                }
            }

            // If there is more than one portable file, record to index.
            if (fileEntries.size() > 1)
            {
                portableInstaller.RecordToIndex = true;
            }

            const std::vector<Manifest::NestedInstallerFile>& nestedInstallerFiles = context.Get<Execution::Data::Installer>()->NestedInstallerFiles;

            for (const auto& nestedInstallerFile : nestedInstallerFiles)
            {
                const std::filesystem::path& targetPath = installLocation / ConvertToUTF16(nestedInstallerFile.RelativeFilePath);

                std::filesystem::path commandAlias;
                if (!nestedInstallerFile.PortableCommandAlias.empty())
                {
                    commandAlias = ConvertToUTF16(nestedInstallerFile.PortableCommandAlias);
                }
                else
                {
                    commandAlias = targetPath.filename();
                }

                Filesystem::AppendExtension(commandAlias, ".exe");
                const std::filesystem::path& symlinkFullPath = GetPortableLinksLocation(portableInstaller.GetScope()) / commandAlias;
                entries.emplace_back(std::move(PortableFileEntry::CreateSymlinkEntry(symlinkFullPath, targetPath)));
            }
        }
        else
        {
            const std::filesystem::path& targetFullPath = GetPortableTargetFullPath(context);
            const std::filesystem::path& symlinkFullPath = GetPortableSymlinkFullPath(context);
            entries.emplace_back(std::move(PortableFileEntry::CreateFileEntry(installerPath, targetFullPath)));
            entries.emplace_back(std::move(PortableFileEntry::CreateSymlinkEntry(symlinkFullPath, targetFullPath)));
        }

        return entries;
    }

    std::vector<PortableFileEntry> GetExpectedState(Execution::Context& context)
    {
        PortableInstaller& portableInstaller = context.Get<Execution::Data::PortableInstaller>();

        std::vector<PortableFileEntry> entries;
        const auto& indexPath = portableInstaller.GetPortableIndexPath();
        if (std::filesystem::exists(indexPath))
        {
            AppInstaller::Repository::Microsoft::PortableIndex portableIndex = AppInstaller::Repository::Microsoft::PortableIndex::Open(indexPath.u8string(), AppInstaller::Repository::Microsoft::SQLiteStorageBase::OpenDisposition::ReadWrite);
            entries = portableIndex.GetAllPortableFiles();
        }
        else
        {
            std::filesystem::path targetFullPath = portableInstaller.PortableTargetFullPath;
            std::filesystem::path symlinkFullPath = portableInstaller.PortableSymlinkFullPath;
            std::filesystem::path installLocation = portableInstaller.InstallLocation;

            if (!portableInstaller.PortableSymlinkFullPath.empty())
            {
                AppInstaller::Portable::PortableFileEntry symlinkEntry;
                symlinkEntry.SetFilePath(symlinkFullPath);
                symlinkEntry.SymlinkTarget = targetFullPath.u8string();
                symlinkEntry.FileType = PortableFileType::Symlink;
                entries.emplace_back(symlinkEntry);
            }

            if (!targetFullPath.empty())
            {
                AppInstaller::Portable::PortableFileEntry exeEntry;
                exeEntry.SetFilePath(targetFullPath);
                exeEntry.SHA256 = portableInstaller.SHA256;
                exeEntry.FileType = PortableFileType::File;
                entries.emplace_back(exeEntry);
            }

            if (!installLocation.empty() && portableInstaller.InstallDirectoryCreated)
            {
                AppInstaller::Portable::PortableFileEntry directoryEntry;
                directoryEntry.SetFilePath(installLocation);
                directoryEntry.FileType = PortableFileType::Directory;
                entries.emplace_back(directoryEntry);
            }
        }

        return entries;
    }

    void PortableInstallImpl(Execution::Context& context)
    {
        HRESULT result = ERROR_SUCCESS;
        PortableInstaller& portableInstaller = context.Get<Execution::Data::PortableInstaller>();

        try
        {
            context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

            // Use context to get desired state and pass it to the portable install resolution engine
            std::vector<AppInstaller::Portable::PortableFileEntry> desiredState = GetDesiredStateForPortableInstall(context);
            std::vector<AppInstaller::Portable::PortableFileEntry> expectedState = GetExpectedState(context);

            //portableInstaller.InitializeRegistryEntry();

            if (portableInstaller.VerifyResolution(expectedState))
            {
                // Return that files have been modified
            }

            portableInstaller.ResolutionEngine(desiredState, expectedState);

            //portableInstaller.AddToPathVariable();

            //FinalizeRegistryEntry();


            context.Add<Execution::Data::OperationReturnCode>(result);
            context.Reporter.Warn() << portableInstaller.GetOutputMessage();
        }
        catch (...)
        {
            context.Add<Execution::Data::OperationReturnCode>(Workflow::HandleException(context, std::current_exception()));
        }

        if (result != ERROR_SUCCESS && !portableInstaller.IsUpdate)
        {
            context.Reporter.Warn() << Resource::String::PortableInstallFailed << std::endl;
            portableInstaller.Uninstall();
        }
    }

    void PortableUninstallImpl(Execution::Context& context)
    {
        PortableInstaller& portableInstaller = context.Get<Execution::Data::PortableInstaller>();

        try
        {
            context.Reporter.Info() << Resource::String::UninstallFlowStartingPackageUninstall << std::endl;

            //if (!portableInstaller.VerifyResolution())
            //{
            //    // TODO: replace with appropriate --force argument when available.
            //    if (context.Args.Contains(Execution::Args::Type::HashOverride))
            //    {
            //        context.Reporter.Warn() << Resource::String::PortableHashMismatchOverridden << std::endl;
            //    }
            //    else
            //    {
            //        context.Reporter.Warn() << Resource::String::PortableHashMismatchOverrideRequired << std::endl;
            //        AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PORTABLE_UNINSTALL_FAILED);
            //    }
            //}

            portableInstaller.Purge = context.Args.Contains(Execution::Args::Type::Purge) ||
                (!portableInstaller.IsUpdate && Settings::User().Get<Settings::Setting::UninstallPurgePortablePackage>() && !context.Args.Contains(Execution::Args::Type::Preserve));

            HRESULT result = portableInstaller.Uninstall();
            context.Add<Execution::Data::OperationReturnCode>(result);
            context.Reporter.Warn() << portableInstaller.GetOutputMessage();
        }
        catch (...)
        {
            context.Add<Execution::Data::OperationReturnCode>(Workflow::HandleException(context, std::current_exception()));
        }
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
}