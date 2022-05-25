// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PortableFlow.h"
#include "winget/Filesystem.h"
#include "winget/PortableARPEntry.h"
#include "AppInstallerStrings.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Registry;
using namespace AppInstaller::Registry::Portable;
using namespace AppInstaller::Repository;
using namespace std::filesystem;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        constexpr std::wstring_view s_PathName = L"Path";
        constexpr std::wstring_view s_PathSubkey_User = L"Environment";
        constexpr std::wstring_view s_PathSubkey_Machine = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
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

        bool AddToPathRegistry(Manifest::ScopeEnum scope)
        {
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

        bool RemoveFromPathRegistry(Manifest::ScopeEnum scope)
        {
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
                FindAndReplace(pathValue, portableLinksDir, "");
                FindAndReplace(pathValue, ";;", ";");
                AICLI_LOG(CLI, Info, << "Removing from Path environment variable: " << portableLinksDir);
                key.SetValue(pathName, ConvertToUTF16(pathValue), REG_EXPAND_SZ);
                return true;
            }
            else
            {
                AICLI_LOG(CLI, Verbose, << "Path does not exist in environment variable.");
                return false;
            }
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

            PortableARPEntry& uninstallEntry = context.Get<Execution::Data::PortableARPEntry>();

            if (uninstallEntry.Exists())
            {
                if (!uninstallEntry.IsSamePortablePackageEntry(packageIdentifier, sourceIdentifier))
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

            uninstallEntry.SetValue(PortableValueName::WinGetPackageIdentifier, packageIdentifier);
            uninstallEntry.SetValue(PortableValueName::WinGetSourceIdentifier, sourceIdentifier);
            uninstallEntry.SetValue(PortableValueName::UninstallString, L"winget uninstall --product-code " + ConvertToUTF16(GetPortableProductCode(context)));
            uninstallEntry.SetValue(PortableValueName::WinGetInstallerType, ConvertToUTF16(InstallerTypeToString(InstallerTypeEnum::Portable)));
        }
        
        void MovePortableExe(Execution::Context& context)
        {
            PortableARPEntry& uninstallEntry = context.Get<Execution::Data::PortableARPEntry>();
            const std::filesystem::path& installerPath = context.Get<Execution::Data::InstallerPath>();
            const std::filesystem::path& targetFullPath = GetPortableTargetFullPath(context);
            const std::filesystem::path& targetDirectory = GetPortableTargetDirectory(context);

            bool isDirectoryCreated = false;
            if (std::filesystem::create_directories(targetDirectory))
            {
                AICLI_LOG(CLI, Info, << "Created target install directory: " << targetDirectory);
                isDirectoryCreated = true;
            }

            if (std::filesystem::exists(targetFullPath))
            {
                std::filesystem::remove(targetFullPath);
                AICLI_LOG(CLI, Info, << "Removing existing portable exe at: " << targetFullPath);
            }

            Filesystem::RenameFile(installerPath, targetFullPath);
            AICLI_LOG(CLI, Info, << "Portable exe moved to: " << targetFullPath);

            if (!uninstallEntry[PortableValueName::InstallDirectoryCreated].has_value())
            {
                uninstallEntry.SetValue(PortableValueName::InstallDirectoryCreated, isDirectoryCreated);
            }

            uninstallEntry.SetValue(PortableValueName::PortableTargetFullPath, targetFullPath.wstring());
            uninstallEntry.SetValue(PortableValueName::InstallLocation, GetPortableTargetDirectory(context).wstring());
            uninstallEntry.SetValue(PortableValueName::SHA256, Utility::SHA256::ConvertToWideString(context.Get<Execution::Data::HashPair>().second));
        }

        void RemovePortableExe(Execution::Context& context)
        {
            PortableARPEntry& uninstallEntry = context.Get<Execution::Data::PortableARPEntry>();
            const auto& targetPath = uninstallEntry[PortableValueName::PortableTargetFullPath];

            if (targetPath.has_value())
            {
                const std::filesystem::path& targetPathValue = targetPath.value().GetValue<Value::Type::UTF16String>();
                const auto& expectedHash = uninstallEntry[PortableValueName::SHA256];
                std::string expectedHashValue = expectedHash.has_value() ? uninstallEntry[PortableValueName::SHA256].value().GetValue<Value::Type::String>() : "";

                if (std::filesystem::exists(targetPathValue))
                {
                    std::ifstream inStream{ targetPathValue, std::ifstream::binary };
                    const Utility::SHA256::HashBuffer& targetFileHash = SHA256::ComputeHash(inStream);
                    inStream.close();

                    bool overrideHashMismatch = context.Args.Contains(Execution::Args::Type::HashOverride);

                    if (!SHA256::AreEqual(SHA256::ConvertToBytes(expectedHashValue), targetFileHash))
                    {
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

                    std::filesystem::remove(targetPathValue);
                    AICLI_LOG(CLI, Info, << "Successfully deleted portable exe:" << targetPathValue);
                }
                else
                {
                    AICLI_LOG(CLI, Info, << "Portable exe not found; Unable to delete portable exe: " << targetPathValue);
                }
            }
            else
            {
                AICLI_LOG(CLI, Info, << "The registry value for [TargetFullPath] does not exist");
            }
        }

        void RemoveInstallDirectory(Execution::Context& context)
        {
            PortableARPEntry& uninstallEntry = context.Get<Execution::Data::PortableARPEntry>();
            const auto& installDirectory = uninstallEntry[PortableValueName::InstallLocation];

            if (installDirectory.has_value())
            {
                const std::filesystem::path& installDirectoryValue = installDirectory.value().GetValue<Value::Type::UTF16String>();
                if (std::filesystem::exists(installDirectoryValue))
                {
                    const auto& isDirectoryCreated = uninstallEntry[PortableValueName::InstallDirectoryCreated];
                    const auto& isDirectoryCreatedValue = isDirectoryCreated.has_value() ? isDirectoryCreated.value().GetValue<Value::Type::DWord>() : FALSE;

                    bool isUpdate = WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerExecutionUseUpdate);

                    if (context.Args.Contains(Execution::Args::Type::Purge) ||
                        (!isUpdate && Settings::User().Get<Settings::Setting::UninstallPurgePortablePackage>() && !context.Args.Contains(Execution::Args::Type::Preserve)))
                    {
                        if (isDirectoryCreatedValue)
                        {
                            context.Reporter.Warn() << Resource::String::PurgeInstallDirectory << std::endl;
                            const auto& removedFilesCount = std::filesystem::remove_all(installDirectoryValue);
                            AICLI_LOG(CLI, Info, << "Purged install location directory. Deleted " << removedFilesCount << " files or directories");
                        }
                        else
                        {
                            context.Reporter.Warn() << Resource::String::UnableToPurgeInstallDirectory << std::endl;
                        }

                    }
                    else if (std::filesystem::is_empty(installDirectoryValue))
                    {
                        if (isDirectoryCreatedValue)
                        {
                            std::filesystem::remove(installDirectoryValue);
                            AICLI_LOG(CLI, Info, << "Install directory deleted: " << installDirectoryValue);
                        }
                    }
                    else
                    {
                        context.Reporter.Warn() << Resource::String::FilesRemainInInstallDirectory << installDirectoryValue << std::endl;
                    }
                }
                else
                {
                    AICLI_LOG(CLI, Info, << "Install directory does not exist: " << installDirectoryValue);
                }
            }
            else
            {
                AICLI_LOG(CLI, Info, << "The registry value for [InstallLocation] does not exist");
            }
        }

        void CreatePortableSymlink(Execution::Context& context)
        {
            PortableARPEntry& uninstallEntry = context.Get<Execution::Data::PortableARPEntry>();
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
            uninstallEntry.SetValue(PortableValueName::PortableSymlinkFullPath, symlinkFullPath.wstring());

            Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
            if (AddToPathRegistry(scope))
            {
                context.Reporter.Warn() << Resource::String::ModifiedPathRequiresShellRestart << std::endl;
            }
        }

        void RemovePortableSymlink(Execution::Context& context)
        {
            PortableARPEntry& uninstallEntry = context.Get<Execution::Data::PortableARPEntry>();
            const auto& symlinkPath = uninstallEntry[PortableValueName::PortableSymlinkFullPath];
            if (symlinkPath.has_value())
            {
                const std::filesystem::path& symlinkPathValue = symlinkPath.value().GetValue<Value::Type::UTF16String>();

                if (!std::filesystem::remove(symlinkPathValue))
                {
                    AICLI_LOG(CLI, Info, << "Portable symlink not found; Unable to delete portable symlink: " << symlinkPathValue);
                }
            }
            else
            {
                AICLI_LOG(CLI, Info, << "The registry value for [SymlinkFullPath] does not exist");
            }

            Manifest::ScopeEnum scope = uninstallEntry.GetScope();

            if (std::filesystem::is_empty(GetPortableLinksLocation(scope)))
            {
                RemoveFromPathRegistry(scope);
            }
        }

        void RemovePortableARPEntry(Execution::Context& context)
        {
            PortableARPEntry& uninstallEntry = context.Get<Execution::Data::PortableARPEntry>();
            uninstallEntry.Delete();
            AICLI_LOG(CLI, Info, << "PortableARPEntry deleted.");
        }

        void CommitPortableMetadataToRegistry(Execution::Context& context)
        {
            PortableARPEntry& uninstallEntry = context.Get<Execution::Data::PortableARPEntry>();
            const AppInstaller::Manifest::Manifest& manifest = context.Get<Execution::Data::Manifest>();
            const Manifest::AppsAndFeaturesEntry& entry = GetAppsAndFeaturesEntryForPortableInstall(context.Get<Execution::Data::Installer>()->AppsAndFeaturesEntries, manifest);

            uninstallEntry.SetValue(PortableValueName::DisplayName, entry.DisplayName);
            uninstallEntry.SetValue(PortableValueName::DisplayVersion, entry.DisplayVersion);
            uninstallEntry.SetValue(PortableValueName::Publisher, entry.Publisher);
            uninstallEntry.SetValue(PortableValueName::InstallDate, Utility::GetCurrentDateForARP());
            uninstallEntry.SetValue(PortableValueName::URLInfoAbout, manifest.CurrentLocalization.Get<Manifest::Localization::PackageUrl>());
            uninstallEntry.SetValue(PortableValueName::HelpLink, manifest.CurrentLocalization.Get<Manifest::Localization::PublisherSupportUrl>());
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

    void PortableInstallImpl(Execution::Context& context)
    {
        PortableARPEntry uninstallEntry = PortableARPEntry(
            ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope)),
            context.Get<Execution::Data::Installer>()->Arch,
            GetPortableProductCode(context));

        context.Add<Execution::Data::PortableARPEntry>(std::move(uninstallEntry));

        try
        {
            context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

            context <<
                InitializePortableARPEntry <<
                MovePortableExe <<
                CreatePortableSymlink <<
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
        bool isUpdate = WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerExecutionUseUpdate);

        if (installReturnCode != 0 && installReturnCode != APPINSTALLER_CLI_ERROR_PORTABLE_PACKAGE_ALREADY_EXISTS && !isUpdate)
        {
            context.Reporter.Warn() << Resource::String::PortableInstallFailed << std::endl;
            auto uninstallPortableContextPtr = context.CreateSubContext();
            Execution::Context& uninstallPortableContext = *uninstallPortableContextPtr;
            auto previousThreadGlobals = uninstallPortableContext.SetForCurrentThread();

            uninstallPortableContext.Add<Execution::Data::PortableARPEntry>(context.Get<Execution::Data::PortableARPEntry>());
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