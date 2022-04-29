// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PortableInstallFlow.h"
#include "winget/Filesystem.h"
#include "winget/PortableARPEntry.h"
#include "AppInstallerStrings.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Registry;
using namespace AppInstaller::Registry::Portable;
using namespace std::filesystem;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        constexpr std::wstring_view s_PathName = L"Path";
        constexpr std::wstring_view s_PathSubkey_User = L"Environment";
        constexpr std::wstring_view s_PathSubkey_Machine = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
        constexpr std::string_view s_LocalSource = "*Local"sv;

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
            if (context.Contains(Execution::Data::Source))
            {
                source = context.Get<Execution::Data::Source>().GetIdentifier();
            }
            else
            {
                source = s_LocalSource;
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

        Manifest::AppsAndFeaturesEntry GetAppsAndFeaturesEntryForPortableInstall(const std::vector<AppInstaller::Manifest::AppsAndFeaturesEntry>& appsAndFeaturesEntries, const AppInstaller::Manifest::Manifest& manifest)
        {
            AppInstaller::Manifest::AppsAndFeaturesEntry appsAndFeaturesEntry;
            if (!appsAndFeaturesEntries.empty())
            {
                appsAndFeaturesEntry = appsAndFeaturesEntries[0];
            }

            if (appsAndFeaturesEntry.DisplayName.empty())
            {
                appsAndFeaturesEntry.DisplayName = manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>();
            }
            if (appsAndFeaturesEntry.DisplayVersion.empty())
            {
                appsAndFeaturesEntry.DisplayVersion = manifest.Version;
            }
            if (appsAndFeaturesEntry.Publisher.empty())
            {
                appsAndFeaturesEntry.Publisher = manifest.DefaultLocalization.Get<Manifest::Localization::Publisher>();
            }

            return appsAndFeaturesEntry;
        }

        bool AddToPathRegistry(Execution::Context& context)
        {
            Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
            const std::filesystem::path& linksDirectory = GetPortableLinksLocation(scope);

            Key key;
            if (scope == Manifest::ScopeEnum::Machine)
            {
                key = Registry::Key::Create(HKEY_LOCAL_MACHINE, std::wstring{ s_PathSubkey_Machine });
            }
            else
            {
                key = Registry::Key::Create(HKEY_CURRENT_USER, std::wstring{ s_PathSubkey_User });
            }

            std::wstring pathName = std::wstring{ s_PathName };
            std::string portableLinksDir = Normalize(linksDirectory.u8string());
            std::string pathValue = Normalize(key[pathName]->GetValue<Value::Type::String>());
            
            if (pathValue.find(portableLinksDir) == std::string::npos)
            {
                if (pathValue.back() != ';')
                {
                    pathValue += ";";
                }

                pathValue += portableLinksDir + ";";
                AICLI_LOG(CLI, Info, << "Adding to Path environment variable: " << portableLinksDir);
                key.SetValue(pathName, ConvertToUTF16(pathValue), REG_EXPAND_SZ);
                return true;
            }
            else
            {
                AICLI_LOG(CLI, Verbose, << "Path already existed in environment variable. Skipping...");
                return false;
            }
        }

        bool RemoveFromPathRegistry(Execution::Context& context)
        {
            Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
            const std::filesystem::path& linksDirectory = GetPortableLinksLocation(scope);

            Key key;
            if (scope == Manifest::ScopeEnum::Machine)
            {
                key = Registry::Key::Create(HKEY_LOCAL_MACHINE, std::wstring{ s_PathSubkey_Machine });
            }
            else
            {
                key = Registry::Key::Create(HKEY_CURRENT_USER, std::wstring{ s_PathSubkey_User });
            }

            std::wstring pathName = std::wstring{ s_PathName };
            std::string portableLinksDir = Normalize(linksDirectory.u8string());
            std::string pathValue = Normalize(key[pathName]->GetValue<Value::Type::String>());

            if (pathValue.find(portableLinksDir) != std::string::npos)
            {
                portableLinksDir += ";";
                FindAndReplace(pathValue, portableLinksDir, "");
                AICLI_LOG(CLI, Info, << "Removing from Path environment variable: " << portableLinksDir);
                key.SetValue(pathName, ConvertToUTF16(pathValue), REG_EXPAND_SZ);
                return true;
            }
            else
            {
                AICLI_LOG(CLI, Verbose, << "Path already does not exist in environment variable.");
                return false;
            }
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

    void GetPortableARPEntryForInstall(Execution::Context& context)
    {
        Portable::PortableARPEntry uninstallEntry = Portable::PortableARPEntry(
            ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope)),
            context.Get<Execution::Data::Installer>()->Arch,
            ConvertToUTF16(GetPortableProductCode(context)));

        context.Add<Execution::Data::PortableARPEntry>(std::move(uninstallEntry));
    }

    void GetPortableARPEntryForUninstall(Execution::Context& context)
    {
        const auto& productCode = context.Get<Execution::Data::ProductCodes>()[0];
        const std::string installedScope = context.Get<Execution::Data::InstalledPackageVersion>()->GetMetadata()[Repository::PackageVersionMetadata::InstalledScope];
        
        // figure out how to get architecture
        Portable::PortableARPEntry uninstallEntry = Portable::PortableARPEntry(
            ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope)),
            Utility::Architecture::X64,
            ConvertToUTF16(productCode));

        context.Add<Execution::Data::PortableARPEntry>(std::move(uninstallEntry));
    }

    void WritePortableEntryToUninstallRegistry(Execution::Context& context)
    {
        const AppInstaller::Manifest::Manifest& manifest = context.Get<Execution::Data::Manifest>();
        const Manifest::AppsAndFeaturesEntry& entry = GetAppsAndFeaturesEntryForPortableInstall(context.Get<Execution::Data::Installer>()->AppsAndFeaturesEntries, manifest);
        const std::string& packageIdentifier = manifest.Id;

        std::string sourceIdentifier;
        if (context.Contains(Execution::Data::Source))
        {
            sourceIdentifier = context.Get<Execution::Data::Source>().GetIdentifier();
        }
        else
        {
            sourceIdentifier = s_LocalSource;
        }

        const std::wstring& productCode = ConvertToUTF16(GetPortableProductCode(context));
        Portable::PortableARPEntry& uninstallEntry = context.Get<Execution::Data::PortableARPEntry>();

        if (uninstallEntry.Exists())
        {
            if (uninstallEntry.IsSamePortablePackageEntry(packageIdentifier, sourceIdentifier))
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

        AICLI_LOG(CLI, Info, << "Begin writing to Uninstall registry.");
        uninstallEntry.SetValue(PortableValueName::DisplayName, entry.DisplayName);
        uninstallEntry.SetValue(PortableValueName::DisplayVersion, entry.DisplayVersion);
        uninstallEntry.SetValue(PortableValueName::Publisher, entry.Publisher);
        uninstallEntry.SetValue(PortableValueName::InstallDate, Utility::GetCurrentDateForARP());
        uninstallEntry.SetValue(PortableValueName::URLInfoAbout, manifest.DefaultLocalization.Get<Manifest::Localization::PackageUrl>());
        uninstallEntry.SetValue(PortableValueName::HelpLink, manifest.DefaultLocalization.Get<Manifest::Localization::PublisherSupportUrl>());
        uninstallEntry.SetValue(PortableValueName::UninstallString, L"winget uninstall --product-code " + productCode);
        uninstallEntry.SetValue(PortableValueName::WinGetInstallerType, ConvertToUTF16(InstallerTypeToString(InstallerTypeEnum::Portable)));
        uninstallEntry.SetValue(PortableValueName::WinGetPackageIdentifier, manifest.Id);
        uninstallEntry.SetValue(PortableValueName::WinGetSourceIdentifier, sourceIdentifier);
        uninstallEntry.SetValue(PortableValueName::PortableTargetFullPath, GetPortableTargetFullPath(context).wstring());
        uninstallEntry.SetValue(PortableValueName::PortableSymlinkFullPath, GetPortableSymlinkFullPath(context).wstring());
        uninstallEntry.SetValue(PortableValueName::SHA256, Utility::SHA256::ConvertToWideString(context.Get<Execution::Data::HashPair>().second));
        uninstallEntry.SetValue(PortableValueName::InstallLocation, GetPortableTargetDirectory(context).wstring());
        AICLI_LOG(CLI, Info, << "Writing to Uninstall registry complete.");
    }

    void MovePortableExe(Execution::Context& context)
    {
        const std::filesystem::path& installerPath = context.Get<Execution::Data::InstallerPath>();
        const std::filesystem::path& targetFullPath = GetPortableTargetFullPath(context);
        const std::filesystem::path& targetDirectory = GetPortableTargetDirectory(context);

        bool isDirectoryCreated = false;
        if (std::filesystem::create_directories(targetDirectory))
        {
            AICLI_LOG(CLI, Info, << "Created target install directory: " << targetDirectory);
            isDirectoryCreated = true;
        }

        Portable::PortableARPEntry& uninstallEntry = context.Get<Execution::Data::PortableARPEntry>();
        uninstallEntry.SetValue(PortableValueName::InstallDirectoryCreated, isDirectoryCreated);

        Filesystem::RenameFile(installerPath, targetFullPath);
        AICLI_LOG(CLI, Info, << "Portable exe moved to: " << targetFullPath);
    }

    void RemovePortableExe(Execution::Context& context)
    {

        Portable::PortableARPEntry& uninstallEntry = context.Get<Execution::Data::PortableARPEntry>();
        const auto& targetPathValue = uninstallEntry.GetValue(PortableValueName::PortableTargetFullPath);
        const auto& installDirectoryCreated = uninstallEntry.GetValue(PortableValueName::InstallDirectoryCreated);

        if (!targetPathValue.has_value())
        {
            context.Reporter.Error() << "Registry has been modified, unable to locate target exe Path" << std::endl;
        }

        const std::filesystem::path& targetPath = targetPathValue.value().GetValue<Value::Type::UTF16String>();
        bool isCreated = installDirectoryCreated.value().GetValue<Value::Type::DWord>();

        if (std::filesystem::remove(targetPath))
        {
            AICLI_LOG(CLI, Info, << "Successfully deleted portable exe:" << targetPath);
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Portable exe does not exist: " << targetPath);
            context.Reporter.Warn() << "Failed to delete portable exe as it was not found" << std::endl;
        }

        if (isCreated)
        {
            const auto& installDirectoryValue = uninstallEntry.GetValue(PortableValueName::InstallLocation);
            const std::filesystem::path& installDirectory = installDirectoryValue.value().GetValue<Value::Type::UTF16String>();

            if (std::filesystem::exists(installDirectory) && std::filesystem::is_empty(installDirectory))
            {
                std::filesystem::remove(installDirectory);
            }
        }
    }

    void CreatePortableSymlink(Execution::Context& context)
    {
        const std::filesystem::path& targetFullPath = GetPortableTargetFullPath(context);
        const std::filesystem::path& symlinkFullPath = GetPortableSymlinkFullPath(context);

        std::filesystem::file_status status = std::filesystem::status(symlinkFullPath);
        if (std::filesystem::is_directory(status))
        {
            AICLI_LOG(CLI, Info, << "Unable to create symlink. '" << symlinkFullPath << "points to an existing directory.");
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PORTABLE_SYMLINK_PATH_IS_DIRECTORY);
        }
        else
        {
            context.Reporter.Warn() << Resource::String::OverwritingExistingFileAtMessage << symlinkFullPath.u8string() << std::endl;
            std::filesystem::remove(symlinkFullPath);
        }

        std::filesystem::create_symlink(targetFullPath, symlinkFullPath);
        AICLI_LOG(CLI, Info, << "Symlink created at: " << symlinkFullPath);

        if (AddToPathRegistry(context))
        {
            context.Reporter.Warn() << Resource::String::ModifiedPathRequiresShellRestart << std::endl;
        }
    }

    void RemovePortableSymlink(Execution::Context& context)
    {
        Portable::PortableARPEntry& uninstallEntry = context.Get<Execution::Data::PortableARPEntry>();
        const auto& symlinkPathValue = uninstallEntry.GetValue(PortableValueName::PortableSymlinkFullPath);
        const std::filesystem::path& symlinkPath = symlinkPathValue.value().GetValue<Value::Type::UTF16String>();

        if (!std::filesystem::remove(symlinkPath))
        {
            context.Reporter.Warn() << "Unable to delete symlink. File may have been moved or renamed." << std::endl;
        }

        const std::string installedScope = context.Get<Execution::Data::InstalledPackageVersion>()->GetMetadata()[Repository::PackageVersionMetadata::InstalledScope];
        if (std::filesystem::is_empty(GetPortableLinksLocation(ConvertToScopeEnum(installedScope))))
        {
            RemoveFromPathRegistry(context);
        }
    }

    void EnsureSupportForPortableInstall(Execution::Context& context)
    {
        auto installerType = context.Get<Execution::Data::Installer>().value().InstallerType;

        if (installerType == InstallerTypeEnum::Portable)
        {
            context <<
                Workflow::EnsureFeatureEnabled(Settings::ExperimentalFeature::Feature::PortableInstall) <<
                EnsureValidArgsForPortableInstall <<
                EnsureVolumeSupportsReparsePoints;
        }
    }
}