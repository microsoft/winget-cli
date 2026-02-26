// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PortableFlow.h"
#include "PortableInstaller.h"
#include "WorkflowBase.h"
#include <winget/Filesystem.h>
#include <winget/PortableFileEntry.h>
#include <winget/PortableIndex.h>

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Utility;
using namespace AppInstaller::CLI::Portable;
using namespace AppInstaller::Portable;
using namespace AppInstaller::Repository::Microsoft;

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
                if (!context.Args.Contains(Execution::Args::Type::Force))
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
        std::shared_ptr<Repository::IPackageVersion> installedVersion;
        if (context.Contains(Execution::Data::InstalledPackageVersion))
        {
            installedVersion = context.Get<Execution::Data::InstalledPackageVersion>();
        }
        if (isUpdate && installedVersion)
        {
            IPackageVersion::Metadata installationMetadata = installedVersion->GetMetadata();
            auto installerScopeItr = installationMetadata.find(Repository::PackageVersionMetadata::InstalledScope);
            if (installerScopeItr != installationMetadata.end())
            {
                scope = Manifest::ConvertToScopeEnum(installerScopeItr->second);
            }
        }
        else
        {
            if (context.Args.Contains(Execution::Args::Type::InstallScope))
            {
                scope = Manifest::ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
            }
            else
            {
                Manifest::ScopeEnum requiredScope = Settings::User().Get<Settings::Setting::InstallScopeRequirement>();
                Manifest::ScopeEnum preferredScope = Settings::User().Get<Settings::Setting::InstallScopePreference>();

                scope = requiredScope != Manifest::ScopeEnum::Unknown ? requiredScope : preferredScope;
            }
        }

        const auto& installer = context.Get<Execution::Data::Installer>().value();
        Utility::Architecture arch = installer.Arch;
        const std::string& productCode = GetPortableProductCode(context);

        PortableInstaller portableInstaller = PortableInstaller(scope, arch, productCode);
        portableInstaller.IsUpdate = isUpdate;

        if (IsArchiveType(installer.BaseInstallerType) && installer.ArchiveBinariesDependOnPath)
        {
            portableInstaller.BinariesDependOnPath = true;
        }

        // Set target install directory
        std::string_view locationArg = context.Args.GetArg(Execution::Args::Type::InstallLocation);
        std::filesystem::path targetInstallDirectory;

        if (!locationArg.empty())
        {
            targetInstallDirectory = std::filesystem::path{ ConvertToUTF16(locationArg) };
        }
        else
        {
            targetInstallDirectory = GetPortableInstallRoot(scope, arch);
            targetInstallDirectory /= ConvertToUTF16(productCode);
        }

        portableInstaller.TargetInstallLocation = targetInstallDirectory;
        portableInstaller.SetAppsAndFeaturesMetadata(context.Get<Execution::Data::Manifest>(), installer.AppsAndFeaturesEntries);
        context.Add<Execution::Data::PortableInstaller>(std::move(portableInstaller));
    }

    std::vector<PortableFileEntry> GetDesiredStateForPortableInstall(Execution::Context& context)
    {
        std::filesystem::path& installerPath = context.Get<Execution::Data::InstallerPath>();
        PortableInstaller& portableInstaller = context.Get<Execution::Data::PortableInstaller>();
        std::vector<PortableFileEntry> entries;

        const std::filesystem::path& targetInstallDirectory = portableInstaller.TargetInstallLocation;
        const std::filesystem::path& symlinkDirectory = GetPortableLinksLocation(portableInstaller.GetScope());

        // InstallerPath will point to a directory if it is extracted from an archive.
        if (std::filesystem::is_directory(installerPath))
        {
            portableInstaller.RecordToIndex = true;

            for (const auto& entry : std::filesystem::directory_iterator(installerPath))
            {
                std::filesystem::path entryPath = entry.path();
                PortableFileEntry portableFile;
                std::filesystem::path relativePath = std::filesystem::relative(entryPath, entryPath.parent_path());
                std::filesystem::path targetPath = targetInstallDirectory / relativePath;

                if (std::filesystem::is_directory(entryPath))
                {
                    entries.emplace_back(std::move(PortableFileEntry::CreateDirectoryEntry(entryPath, targetPath)));
                }
                else
                {
                    entries.emplace_back(std::move(PortableFileEntry::CreateFileEntry(entryPath, targetPath, {})));
                }
            }

            const std::vector<Manifest::NestedInstallerFile>& nestedInstallerFiles = context.Get<Execution::Data::Installer>()->NestedInstallerFiles;

            for (const auto& nestedInstallerFile : nestedInstallerFiles)
            {
                const std::filesystem::path& targetPath = targetInstallDirectory / ConvertToUTF16(nestedInstallerFile.RelativeFilePath);

                std::filesystem::path commandAlias;
                if (nestedInstallerFile.PortableCommandAlias.empty())
                {
                    commandAlias = targetPath.filename();
                }
                else
                {
                    commandAlias = ConvertToUTF16(nestedInstallerFile.PortableCommandAlias);
                }

                Filesystem::AppendExtension(commandAlias, ".exe");
                entries.emplace_back(std::move(PortableFileEntry::CreateSymlinkEntry(symlinkDirectory / commandAlias, targetPath)));
            }
        }
        else
        {
            std::string_view renameArg = context.Args.GetArg(Execution::Args::Type::Rename);
            const std::vector<string_t>& commands = context.Get<Execution::Data::Installer>()->Commands;
            std::filesystem::path commandAlias = installerPath.filename();

            if (!commands.empty())
            {
                commandAlias = ConvertToUTF16(commands[0]);
            }

            if (!renameArg.empty())
            {
                commandAlias = ConvertToUTF16(renameArg);
            }
            AppInstaller::Filesystem::AppendExtension(commandAlias, ".exe");

            const std::filesystem::path& targetFullPath = targetInstallDirectory / commandAlias;
            entries.emplace_back(std::move(PortableFileEntry::CreateFileEntry(installerPath, targetFullPath, {})));
            entries.emplace_back(std::move(PortableFileEntry::CreateSymlinkEntry(symlinkDirectory / commandAlias, targetFullPath)));
        }

        return entries;
    }

    void PortableInstallImpl(Execution::Context& context)
    {
        OperationType installType = WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerExecutionUseUpdate) ? OperationType::Upgrade : OperationType::Install;
        
        PortableInstaller& portableInstaller = context.Get<Execution::Data::PortableInstaller>();
        try
        {
            context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

            std::vector<AppInstaller::Portable::PortableFileEntry> desiredState = GetDesiredStateForPortableInstall(context);

            portableInstaller.SetDesiredState(desiredState);

            if (!portableInstaller.VerifyExpectedState())
            {
                if (context.Args.Contains(Execution::Args::Type::Force))
                {
                    context.Reporter.Warn() << Resource::String::PortableHashMismatchOverridden << std::endl;
                }
                else
                {
                    context.Reporter.Warn() << Resource::String::PortableHashMismatchOverrideRequired << std::endl;
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PORTABLE_UNINSTALL_FAILED);
                }
            }

            portableInstaller.Install(installType);
            context.Add<Execution::Data::CorrelatedAppsAndFeaturesEntries>({ portableInstaller.GetAppsAndFeaturesEntry() });
            context.Add<Execution::Data::OperationReturnCode>(ERROR_SUCCESS);
            context.Reporter.Warn() << portableInstaller.GetOutputMessage();
        }
        catch (...)
        {
            context.Add<Execution::Data::OperationReturnCode>(Workflow::HandleException(context, std::current_exception()));

            if (!portableInstaller.IsUpdate)
            {
                context.Reporter.Warn() << Resource::String::PortableInstallFailed << std::endl;
                portableInstaller.PrepareForCleanUp();
;               portableInstaller.Uninstall();
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PORTABLE_UNINSTALL_FAILED);
            }
        }
    }

    void PortableUninstallImpl(Execution::Context& context)
    {
        PortableInstaller& portableInstaller = context.Get<Execution::Data::PortableInstaller>();

        try
        {
            context.Reporter.Info() << Resource::String::UninstallFlowStartingPackageUninstall << std::endl;

            if (!portableInstaller.VerifyExpectedState())
            {
                if (context.Args.Contains(Execution::Args::Type::Force))
                {
                    context.Reporter.Warn() << Resource::String::PortableHashMismatchOverridden << std::endl;
                }
                else
                {
                    context.Reporter.Warn() << Resource::String::PortableHashMismatchOverrideRequired << std::endl;
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PORTABLE_UNINSTALL_FAILED);
                }
            }

            portableInstaller.Purge = context.Args.Contains(Execution::Args::Type::Purge) ||
                (!portableInstaller.IsUpdate && Settings::User().Get<Settings::Setting::UninstallPurgePortablePackage>() && !context.Args.Contains(Execution::Args::Type::Preserve));

            portableInstaller.Uninstall();
            context.Add<Execution::Data::OperationReturnCode>(ERROR_SUCCESS);
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
                EnsureValidArgsForPortableInstall <<
                EnsureVolumeSupportsReparsePoints;
        }
    }
}
