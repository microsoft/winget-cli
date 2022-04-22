// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PortableInstallFlow.h"
#include "winget/Filesystem.h"
#include "AppInstallerStrings.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Registry;
using namespace std::filesystem;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        constexpr std::wstring_view s_PathName = L"Path";
        constexpr std::wstring_view s_PathSubkey_User = L"Environment";
        constexpr std::wstring_view s_PathSubkey_Machine = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
        constexpr std::wstring_view s_UninstallRegistryX64 = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
        constexpr std::wstring_view s_UninstallRegistryX86 = L"Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall";

        constexpr std::wstring_view s_DisplayName = L"DisplayName";
        constexpr std::wstring_view s_DisplayVersion = L"DisplayVersion";
        constexpr std::wstring_view s_Publisher = L"Publisher";
        constexpr std::wstring_view s_InstallDate = L"InstallDate";
        constexpr std::wstring_view s_URLInfoAbout = L"URLInfoAbout";
        constexpr std::wstring_view s_HelpLink = L"HelpLink";
        constexpr std::wstring_view s_UninstallString = L"UninstallString";
        constexpr std::wstring_view s_WinGetInstallerType = L"WinGetInstallerType";
        constexpr std::wstring_view s_InstallLocation = L"InstallLocation";
        constexpr std::wstring_view s_PortableTargetFullPath = L"TargetFullPath";
        constexpr std::wstring_view s_PortableSymlinkFullPath = L"SymlinkFullPath";
        constexpr std::wstring_view s_PortableLinksDirectory = L"LinkDirectory";
        constexpr std::wstring_view s_SHA256 = L"SHA256";

        constexpr std::wstring_view s_WinGetPackageIdentifier = L"WinGetPackageIdentifier";
        constexpr std::wstring_view s_WinGetSourceIdentifier = L"WinGetSourceIdentifier";
        constexpr std::string_view s_LocalSource = "*Local"sv;

        void AppendExeExtension(std::filesystem::path& value)
        {
            if (value.extension() != ".exe")
            {
                value += ".exe";
            }
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

        bool IsSamePortablePackageEntry(Registry::Key entry, std::string packageId, std::string sourceId)
        {
            auto existingWinGetPackageId = entry[Normalize(s_WinGetPackageIdentifier)];
            auto existingWinGetSourceId = entry[Normalize(s_WinGetSourceIdentifier)];

            bool isSamePackageId = false;
            bool isSamePackageSource = false;

            if (existingWinGetPackageId.has_value())
            {
                isSamePackageId = existingWinGetPackageId.value().GetValue<Value::Type::String>() == packageId;
            }

            if (existingWinGetSourceId.has_value())
            {
                isSamePackageSource = existingWinGetSourceId.value().GetValue<Value::Type::String>() == sourceId;
            }

            return isSamePackageId && isSamePackageSource;
        }

        std::filesystem::path GetPortableTargetDirectory(Execution::Context& context)
        {
            Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
            Utility::Architecture arch = context.Get<Execution::Data::Installer>()->Arch;
            std::string_view locationArg = context.Args.GetArg(Execution::Args::Type::InstallLocation);
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

            // Make sure that virtual sources don't contain "*" or creating the directory will fail.
            FindAndReplace(source, "*", ""); 
            std::filesystem::path targetInstallDirectory;

            if (!locationArg.empty())
            {
                targetInstallDirectory = std::filesystem::path{ ConvertToUTF16(locationArg) };
            }
            else
            {
                targetInstallDirectory = GetPortableInstallRoot(scope, arch);
                targetInstallDirectory /= packageId + "_" + source;
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
                key = Registry::Key::Create(HKEY_LOCAL_MACHINE, Normalize(s_PathSubkey_Machine));
            }
            else
            {
                key = Registry::Key::Create(HKEY_CURRENT_USER, Normalize(s_PathSubkey_User));
            }

            std::wstring pathName = Normalize(s_PathName);
            std::string portableLinksDir = linksDirectory.u8string();
            std::string pathValue = key[pathName]->GetValue<Value::Type::String>();
            
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

        Registry::Key GetPortableEntryUninstallRegistrySubkey(Manifest::ScopeEnum scope, Utility::Architecture arch, const std::wstring& productCode, bool& isExistingEntry)
        {
            HKEY root;
            std::wstring subKey;
            if (scope == Manifest::ScopeEnum::Machine)
            {
                root = HKEY_LOCAL_MACHINE;
                if (arch == Utility::Architecture::X64)
                {
                    subKey = s_UninstallRegistryX64;
                }
                else
                {
                    subKey = s_UninstallRegistryX86;
                }
            }
            else
            {
                // HKCU uninstall registry share the x64 registry view.
                root = HKEY_CURRENT_USER;
                subKey = s_UninstallRegistryX64;
            }

            subKey += L"\\" + productCode;
            Registry::Key entry = Key::OpenIfExists(root, subKey, 0, KEY_ALL_ACCESS);
            if (entry != NULL)
            {
                isExistingEntry = true;
            }
            else
            {
                entry = Key::Create(root, subKey);
            }

            return entry;
        }

        Registry::Key WritePortableEntryToUninstallRegistry(Execution::Context& context)
        {
            const AppInstaller::Manifest::Manifest& manifest = context.Get<Execution::Data::Manifest>();
            const Manifest::AppsAndFeaturesEntry& entry = GetAppsAndFeaturesEntryForPortableInstall(context.Get<Execution::Data::Installer>()->AppsAndFeaturesEntries, manifest);
            std::string packageIdentifier = manifest.Id;

            std::string sourceIdentifier;
            if (context.Contains(Execution::Data::Source))
            {
                sourceIdentifier = context.Get<Execution::Data::Source>().GetIdentifier();
            }
            else
            {
                sourceIdentifier = s_LocalSource;
            }

            std::wstring productCode;
            if (!entry.ProductCode.empty())
            {
                productCode = ConvertToUTF16(entry.ProductCode);
            }
            else
            {
                productCode = ConvertToUTF16(packageIdentifier + "_" + sourceIdentifier);
            }

            bool isExistingEntry = false;
            Key uninstallEntry = GetPortableEntryUninstallRegistrySubkey(
                ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope)),
                context.Get<Execution::Data::Installer>()->Arch,
                productCode,
                isExistingEntry);

            if(isExistingEntry)
            {
                if (!IsSamePortablePackageEntry(uninstallEntry, packageIdentifier, sourceIdentifier))
                {
                    // TODO: Replace HashOverride with --Force when argument behavior gets updated.
                    if (!context.Args.Contains(Execution::Args::Type::HashOverride))
                    {
                        AICLI_LOG(CLI, Error, << "Registry match failed, skipping write to uninstall registry");
                        context.Reporter.Error() << Resource::String::PortableRegistryCollisionOverrideRequired << std::endl;
                        AICLI_TERMINATE_CONTEXT_RETURN(APPINSTALLER_CLI_ERROR_PORTABLE_PACKAGE_ALREADY_EXISTS, {});
                    }
                    else
                    {
                        AICLI_LOG(CLI, Info, << "Overriding registry match check...");
                        context.Reporter.Warn() << Resource::String::PortableRegistryCollisionOverridden << std::endl;
                    }
                }
            }

            AICLI_LOG(CLI, Info, << "Begin writing to Uninstall registry.");
            uninstallEntry.SetValue(Normalize(s_DisplayName), ConvertToUTF16(entry.DisplayName), REG_SZ);
            uninstallEntry.SetValue(Normalize(s_DisplayVersion), ConvertToUTF16(entry.DisplayVersion), REG_SZ);
            uninstallEntry.SetValue(Normalize(s_Publisher), ConvertToUTF16(entry.Publisher), REG_SZ);
            uninstallEntry.SetValue(Normalize(s_InstallDate), ConvertToUTF16(Utility::GetCurrentDateForARP()), REG_SZ);
            uninstallEntry.SetValue(Normalize(s_URLInfoAbout), ConvertToUTF16(manifest.DefaultLocalization.Get<Manifest::Localization::PackageUrl>()), REG_SZ);
            uninstallEntry.SetValue(Normalize(s_HelpLink), ConvertToUTF16(manifest.DefaultLocalization.Get<Manifest::Localization::PublisherSupportUrl>()), REG_SZ);
            uninstallEntry.SetValue(Normalize(s_UninstallString), L"winget uninstall --product-code " + productCode, REG_SZ);
            uninstallEntry.SetValue(Normalize(s_WinGetInstallerType), ConvertToUTF16(InstallerTypeToString(InstallerTypeEnum::Portable)), REG_SZ);
            uninstallEntry.SetValue(Normalize(s_WinGetPackageIdentifier), ConvertToUTF16(manifest.Id), REG_SZ);
            uninstallEntry.SetValue(Normalize(s_WinGetSourceIdentifier), ConvertToUTF16(sourceIdentifier), REG_SZ);
            uninstallEntry.SetValue(Normalize(s_PortableTargetFullPath), GetPortableTargetFullPath(context).wstring(), REG_SZ);
            uninstallEntry.SetValue(Normalize(s_PortableSymlinkFullPath), GetPortableSymlinkFullPath(context).wstring(), REG_SZ);
            uninstallEntry.SetValue(Normalize(s_SHA256), Utility::SHA256::ConvertToWideString(context.Get<Execution::Data::HashPair>().second), REG_SZ);
            uninstallEntry.SetValue(Normalize(s_InstallLocation), GetPortableTargetDirectory(context).wstring(), REG_SZ);
            AICLI_LOG(CLI, Info, << "Writing to Uninstall registry complete.");
            return uninstallEntry;
        }
    }

    void PortableInstallImpl(Execution::Context& context)
    {
        try
        {
            context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

            Registry::Key entry = WritePortableEntryToUninstallRegistry(context);
            if (context.IsTerminated())
            {
                return;
            }

            const std::filesystem::path& installerPath = context.Get<Execution::Data::InstallerPath>();
            const std::filesystem::path& targetFullPath = entry[Normalize(s_PortableTargetFullPath)].value().GetValue<Value::Type::UTF16String>();
            const std::filesystem::path& symlinkFullPath = entry[Normalize(s_PortableSymlinkFullPath)].value().GetValue<Value::Type::UTF16String>();

            std::filesystem::path targetDirectory = targetFullPath.parent_path();
            if (std::filesystem::create_directories(targetDirectory))
            {
                AICLI_LOG(CLI, Info, << "Creating target install directories: " << targetDirectory);
            }

            Filesystem::RenameFile(installerPath, targetFullPath);
            AICLI_LOG(CLI, Info, << "Portable exe moved to: " << targetFullPath);

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

            if (AddToPathRegistry(context))
            {
                context.Reporter.Warn() << Resource::String::ModifiedPathRequiresShellRestart << std::endl;
            }

            context.Add<Execution::Data::OperationReturnCode>(0);
            context.Reporter.Info() << Resource::String::InstallFlowInstallSuccess << std::endl;
        }
        catch (...)
        {
            Workflow::HandleException(context, std::current_exception());
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PORTABLE_INSTALL_FAILED);
        }
    }

    void EnsureFeatureEnabledForPortableInstall(Execution::Context& context)
    {
        auto installerType = context.Get<Execution::Data::Installer>().value().InstallerType;

        if (installerType == InstallerTypeEnum::Portable)
        {
            context << Workflow::EnsureFeatureEnabled(Settings::ExperimentalFeature::Feature::PortableInstall);
        }
    }

    void EnsureValidArgsForPortableInstall(Execution::Context& context)
    {
        std::string_view renameArg = context.Args.GetArg(Execution::Args::Type::Rename);
        std::string_view installLocationArg = context.Args.GetArg(Execution::Args::Type::InstallLocation);

        if (MakeSuitablePathPart(renameArg) != renameArg)
        {
            context.Reporter.Error() << Resource::String::InvalidRenameArgumentError << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS);
        }

        const std::filesystem::path& targetDirectory = GetPortableTargetDirectory(context);
        if (!std::filesystem::is_directory(targetDirectory) || !std::filesystem::exists(targetDirectory))
        {
            if (!std::filesystem::create_directories(targetDirectory))
            {
                context.Reporter.Error() << Resource::String::PortableInstallDirectoryNotCreated;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_COMMAND_FAILED);
            }
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