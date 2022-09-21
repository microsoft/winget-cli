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
        portableInstaller.IsUpdate = isUpdate;

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
        portableInstaller.SetAppsAndFeaturesMetadata(context.Get<Execution::Data::Manifest>(), context.Get<Execution::Data::Installer>()->AppsAndFeaturesEntries);
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

            if (entries.size() > 1)
            {
                portableInstaller.RecordToIndex = true;
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
            std::filesystem::path fileName;
            std::filesystem::path commandAlias;
            
            if (!renameArg.empty())
            {
                fileName = commandAlias = ConvertToUTF16(renameArg);
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

                fileName = installerPath.filename();
            }

            AppInstaller::Filesystem::AppendExtension(fileName, ".exe");
            AppInstaller::Filesystem::AppendExtension(commandAlias, ".exe");

            const std::filesystem::path& targetFullPath = targetInstallDirectory / fileName;
            entries.emplace_back(std::move(PortableFileEntry::CreateFileEntry(installerPath, targetFullPath, {})));
            entries.emplace_back(std::move(PortableFileEntry::CreateSymlinkEntry(symlinkDirectory / commandAlias, targetFullPath)));
        }

        return entries;
    }

    void PortableInstallImpl(Execution::Context& context)
    {
        PortableInstaller& portableInstaller = context.Get<Execution::Data::PortableInstaller>();

        try
        {
            context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

            std::vector<AppInstaller::Portable::PortableFileEntry> desiredState = GetDesiredStateForPortableInstall(context);

            portableInstaller.SetDesiredState(desiredState);

            if (!portableInstaller.VerifyExpectedState())
            {
                if (context.Args.Contains(Execution::Args::Type::HashOverride))
                {
                    context.Reporter.Warn() << Resource::String::PortableHashMismatchOverridden << std::endl;
                }
                else
                {
                    context.Reporter.Warn() << Resource::String::PortableHashMismatchOverrideRequired << std::endl;
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PORTABLE_UNINSTALL_FAILED);
                }
            }

            portableInstaller.Install();
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
                // TODO: replace with appropriate --force argument when available.
                if (context.Args.Contains(Execution::Args::Type::HashOverride))
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