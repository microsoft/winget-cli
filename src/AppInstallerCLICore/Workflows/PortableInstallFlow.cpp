// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PortableInstallFlow.h"
#include "winget/Filesystem.h"
#include "AppInstallerStrings.h"
#include "Workflows/InstallFlow.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Registry;
using namespace std::filesystem;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        constexpr std::wstring_view s_PathSubkey_User = L"Environment";
        constexpr std::wstring_view s_PathSubkey_Machine = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
        constexpr std::wstring_view s_UninstallRegistryX64 = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
        constexpr std::wstring_view s_UninstallRegistryX86 = L"Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall";

        constexpr std::wstring_view s_DisplayName = L"DisplayVersion";
        constexpr std::wstring_view s_DisplayVersion = L"DisplayVersion";
        constexpr std::wstring_view s_Publisher = L"Publisher";
        constexpr std::wstring_view s_UninstallString = L"UninstallString";
        constexpr std::wstring_view s_WinGetInstallerType = L"WinGetInstallerType";
        constexpr std::wstring_view s_InstallLocation = L"InstallLocation";
        constexpr std::wstring_view s_PortableTargetFullPath = L"TargetFullPath";
        constexpr std::wstring_view s_PortableSymlinkFullPath = L"SymlinkFullPath";
        constexpr std::wstring_view s_PortableLinksDirectory = L"LinkDirectory";

        constexpr std::wstring_view s_WinGetPackageIdentifier = L"WinGetPackageIdentifier";
        constexpr std::wstring_view s_WinGetSourceIdentifier = L"WinGetSourceIdentifier";
        constexpr std::string_view s_LocalSource = "Local"sv;

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

        std::filesystem::path GetPortableLinksLocation(Manifest::ScopeEnum scope, Utility::Architecture arch)
        {
            if (scope == Manifest::ScopeEnum::Machine)
            {
                if (arch == Utility::Architecture::X86)
                {
                    return Runtime::GetPathTo(Runtime::PathName::PortableLinksMachineLocationX86);
                }
                else
                {
                    return Runtime::GetPathTo(Runtime::PathName::PortableLinksMachineLocationX64);
                }
            }
            else
            {
                return Runtime::GetPathTo(Runtime::PathName::PortableLinksUserLocation);
            }
        }

        std::filesystem::path GetPortableTargetFullPath(Execution::Context& context)
        {
            const std::filesystem::path installerPath = context.Get<Execution::Data::InstallerPath>();
            std::string_view renameArg = context.Args.GetArg(Execution::Args::Type::Rename);
            std::string_view locationArg = context.Args.GetArg(Execution::Args::Type::InstallLocation);
            std::string packageId = context.Get<Execution::Data::Manifest>().Id;
            Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
            Utility::Architecture arch = context.Get<Execution::Data::Installer>()->Arch;

            std::filesystem::path targetInstallDirectory;

            if (!locationArg.empty())
            {
                targetInstallDirectory = std::filesystem::path{ ConvertToUTF16(locationArg) };
            }
            else
            {
                targetInstallDirectory = GetPortableInstallRoot(scope, arch);
            }

            targetInstallDirectory /= packageId;

            std::filesystem::create_directories(targetInstallDirectory);

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
            std::filesystem::path targetInstallFullPath = targetInstallDirectory / fileName;
            return targetInstallFullPath;
        }

        std::filesystem::path GetPortableSymlinkFullPath(Execution::Context& context)
        {
            const std::filesystem::path installerPath = context.Get<Execution::Data::InstallerPath>();
            Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
            Utility::Architecture arch = context.Get<Execution::Data::Installer>()->Arch;
            std::vector<string_t> commands = context.Get<Execution::Data::Installer>()->Commands;
            std::string_view renameArg = context.Args.GetArg(Execution::Args::Type::Rename);

            std::filesystem::path commandAlias;
            if (!renameArg.empty())
            {
                std::string renameArgValue = Normalize(renameArg);
                commandAlias = renameArgValue;
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
            std::filesystem::path symlinkPath = GetPortableLinksLocation(scope, arch) / commandAlias;
            return symlinkPath;
        }

        Manifest::AppsAndFeaturesEntry GetAppsAndFeaturesEntryForPortableInstall(const std::vector<AppInstaller::Manifest::AppsAndFeaturesEntry>& appsAndFeaturesEntries, const AppInstaller::Manifest::Manifest& manifest)
        {
            AppInstaller::Manifest::AppsAndFeaturesEntry appsAndFeaturesEntry;
            std::string packageId = manifest.Id;
            std::string displayVersion = manifest.Version;
            std::string displayName = manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>();
            std::string publisher = manifest.DefaultLocalization.Get<Manifest::Localization::Publisher>();

            if (!appsAndFeaturesEntries.empty())
            {
                appsAndFeaturesEntry = appsAndFeaturesEntries[0];
            }

            if (appsAndFeaturesEntry.DisplayName.empty())
            {
                appsAndFeaturesEntry.DisplayName = displayName;
            }
            if (appsAndFeaturesEntry.DisplayVersion.empty())
            {
                appsAndFeaturesEntry.DisplayVersion = displayVersion;
            }
            if (appsAndFeaturesEntry.Publisher.empty())
            {
                appsAndFeaturesEntry.Publisher = publisher;
            }
            if (appsAndFeaturesEntry.ProductCode.empty())
            {
                appsAndFeaturesEntry.ProductCode = packageId;
            }

            return appsAndFeaturesEntry;
        }

        std::tuple<HKEY, std::wstring> GetPortableRegistryRootAndSubKey(Manifest::ScopeEnum scope, Utility::Architecture arch, const std::wstring& productCode)
        {
            HKEY root;
            std::wstring subKey;
            if (scope == Manifest::ScopeEnum::Machine)
            {
                root = HKEY_LOCAL_MACHINE;
                if (arch == Utility::Architecture::X64)
                {
                    subKey = Normalize(s_UninstallRegistryX64);
                }
                else
                {
                    subKey = Normalize(s_UninstallRegistryX86);
                }
            }
            else
            {
                // HKCU uninstall registry share the x64 registry view.
                root = HKEY_CURRENT_USER;
                subKey = Normalize(s_UninstallRegistryX64);
            }

            subKey += L"\\" + productCode;

            return std::make_tuple(root, subKey);
        }

        void AddToPathRegistry(const std::filesystem::path& linksDirectory, Manifest::ScopeEnum scope, bool& isPathModified)
        {
            Key key;
            if (scope == Manifest::ScopeEnum::Machine)
            {
                key = Registry::Key::Create(HKEY_LOCAL_MACHINE, Normalize(s_PathSubkey_Machine));
            }
            else
            {
                key = Registry::Key::Create(HKEY_CURRENT_USER, Normalize(s_PathSubkey_User));
            }

            std::wstring portableLinksDir = linksDirectory.wstring();
            std::wstring pathKey = { L"Path" };
            std::wstring pathValue = ConvertToUTF16(key[pathKey]->GetValue<Value::Type::String>());

            if (pathValue.find(portableLinksDir) == std::string::npos)
            {
                if (pathValue.back() != ';')
                {
                    pathValue += L";";
                }

                pathValue += portableLinksDir + L";";
                AICLI_LOG(CLI, Info, << "Adding to Path environment variable: " << ConvertToUTF8(portableLinksDir));
                key.SetValue(pathKey, pathValue, REG_EXPAND_SZ);
                isPathModified = true;
            }
            else
            {
                AICLI_LOG(CLI, Verbose, << "Path already existed in environment variable. Skipping...");
            }

            return;
        }

        void RemoveFromPathRegistry(HKEY root, const std::filesystem::path& linksDirectory)
        {
            std::string linksDirectoryString = linksDirectory.u8string();
            Key key;
            if (root == HKEY_LOCAL_MACHINE)
            {
                key = Registry::Key::Create(root, Normalize(s_PathSubkey_Machine));
            }
            else
            {
                key = Registry::Key::Create(root, Normalize(s_PathSubkey_User));
            }

            std::wstring pathKey = { L"Path" };
            std::string pathValue = key[pathKey]->GetValue<Value::Type::String>();
            linksDirectoryString += ";";

            if (pathValue.find(linksDirectoryString) == std::string::npos)
            {
                FindAndReplace(pathValue, linksDirectoryString, "");
                AICLI_LOG(CLI, Info, << "Removing '" << linksDirectoryString << "' from Path environment variable : ");
                key.SetValue(pathKey, ConvertToUTF16(pathValue), REG_EXPAND_SZ);
            }
            else
            {
                AICLI_LOG(CLI, Verbose, << "Path environment variable does not contain '" << linksDirectoryString << "'");
            }
        }

        //void CreatePortableSymlink(const std::filesystem::path& targetPath, const std::filesystem::path& linkPath)
        //{
        //    if (std::filesystem::is_symlink(linkPath))
        //    {
        //        std::filesystem::remove(linkPath);
        //    }

        //    // Check that symlinkPath does not collide with non-symlink files with the same path.
        //    if (std::filesystem::exists(linkPath))
        //    {
        //        std::filesystem::remove(linkPath);
        //    }

        //    if (AppInstaller::Filesystem::SupportsReparsePoints(targetPath))
        //    {
        //        std::filesystem::create_symlink(targetPath, linkPath);
        //    }
        //    else
        //    {
        //        // TODO: Terminate context if we fail to write to registry...
        //    }

        //}

        Registry::Key RegisterPortableToUninstallRegistry(Execution::Context& context)
        {
            Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
            Utility::Architecture arch = context.Get<Execution::Data::Installer>()->Arch;
            AppInstaller::Manifest::Manifest manifest = context.Get<Execution::Data::Manifest>();
            Manifest::AppsAndFeaturesEntry entry = GetAppsAndFeaturesEntryForPortableInstall(context.Get<Execution::Data::Installer>()->AppsAndFeaturesEntries, manifest);
            std::wstring productCode = ConvertToUTF16(entry.ProductCode);
            std::wstring displayName = ConvertToUTF16(entry.DisplayName);
            std::wstring displayVersion = ConvertToUTF16(entry.DisplayVersion);
            std::wstring publisher = ConvertToUTF16(entry.Publisher);
            std::wstring uninstallString = L"winget uninstall --product-code " + productCode;
            std::string_view installerType = InstallerTypeToString(context.Get<Execution::Data::Installer>().value().InstallerType);
            std::string packageIdentifier = context.Get<Execution::Data::Manifest>().Id;
            std::filesystem::path targetFullPath = GetPortableTargetFullPath(context);
            std::filesystem::path symlinkFullPath = GetPortableSymlinkFullPath(context);
            std::filesystem::path installLocation = GetPortableInstallRoot(scope, arch);
            std::filesystem::path linksDirectory = GetPortableLinksLocation(scope, arch);
            installLocation /= packageIdentifier;

            std::string sourceIdentifier;
            try
            {
                auto source = context.Get<Execution::Data::Source>();
                sourceIdentifier = source.GetIdentifier();
            }
            catch (...)
            {
                sourceIdentifier = s_LocalSource;
            }

            auto [root, subKey] = GetPortableRegistryRootAndSubKey(scope, arch, productCode);
            bool isNewEntry = Key::OpenIfExists(root, subKey) == NULL;
            Key uninstallEntry = Key::Create(root, subKey);

            bool isSamePackageId = false;
            bool isSamePackageSource = false;

            if (!isNewEntry)
            {
                // Check if existing WinGetPackageIdentifier and WinGetSourceIdentifier both match the new entry to determine if we can safely overwrite.
                auto existingWinGetPackageId = uninstallEntry[Normalize(s_WinGetPackageIdentifier)];
                auto existingWinGetSourceId = uninstallEntry[Normalize(s_WinGetSourceIdentifier)];

                if (existingWinGetPackageId.has_value())
                {
                    isSamePackageId = existingWinGetPackageId.value().GetValue<Value::Type::String>() == packageIdentifier;
                }

                if (existingWinGetSourceId.has_value())
                {
                    isSamePackageSource = existingWinGetSourceId.value().GetValue<Value::Type::String>() == sourceIdentifier;
                }
            }

            if (isNewEntry || isSamePackageId && isSamePackageSource)
            {
                AICLI_LOG(CLI, Verbose, << "Begin writing to uninstall registry.");
                uninstallEntry.SetValue(Normalize(s_DisplayName), displayName, REG_SZ);
                uninstallEntry.SetValue(Normalize(s_DisplayVersion), displayVersion, REG_SZ);
                uninstallEntry.SetValue(Normalize(s_Publisher), publisher, REG_SZ);
                uninstallEntry.SetValue(Normalize(s_UninstallString), uninstallString, REG_SZ);
                uninstallEntry.SetValue(Normalize(s_WinGetInstallerType), ConvertToUTF16(installerType), REG_SZ);
                uninstallEntry.SetValue(Normalize(s_WinGetPackageIdentifier), ConvertToUTF16(packageIdentifier), REG_SZ);
                uninstallEntry.SetValue(Normalize(s_InstallLocation), installLocation.wstring(), REG_SZ);
                uninstallEntry.SetValue(Normalize(s_WinGetSourceIdentifier), ConvertToUTF16(sourceIdentifier), REG_SZ);
                uninstallEntry.SetValue(Normalize(s_PortableTargetFullPath), targetFullPath.wstring(), REG_SZ);
                uninstallEntry.SetValue(Normalize(s_PortableSymlinkFullPath), symlinkFullPath.wstring(), REG_SZ);
                uninstallEntry.SetValue(Normalize(s_PortableLinksDirectory), linksDirectory.wstring(), REG_SZ);
                AICLI_LOG(CLI, Verbose, << "Writing to Uninstall registry complete.");
            }

            return uninstallEntry;
        }
    }

    void PortableInstall(Execution::Context& context)
    {
        context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

        try
        {
            Registry::Key baseEntry = RegisterPortableToUninstallRegistry(context);

            const std::filesystem::path installerPath = context.Get<Execution::Data::InstallerPath>();
            std::filesystem::path portableTargetFullPath = ConvertToUTF16(baseEntry[Normalize(s_PortableTargetFullPath)].value().GetValue<Value::Type::String>());
            std::filesystem::path portableLinkFullPath = ConvertToUTF16(baseEntry[Normalize(s_PortableSymlinkFullPath)].value().GetValue<Value::Type::String>());

            std::filesystem::rename(installerPath, portableTargetFullPath);
            AICLI_LOG(CLI, Info, << "Portable exe moved to: " << portableTargetFullPath);

            if (std::filesystem::is_symlink(portableLinkFullPath))
            {
                std::filesystem::remove(portableLinkFullPath);
            }

            // Check that symlinkPath does not collide with non-symlink files with the same path.
            if (std::filesystem::exists(portableLinkFullPath))
            {
                std::filesystem::remove(portableLinkFullPath);
            }

            if (AppInstaller::Filesystem::SupportsReparsePoints(portableTargetFullPath))
            {
                std::filesystem::create_symlink(portableTargetFullPath, portableLinkFullPath);
                AICLI_LOG(CLI, Info, << "Portable symlink created at: " << portableTargetFullPath);
            }
            else
            {
                context << ReportInstallerResult("Portable"sv, APPINSTALLER_CLI_ERROR_PORTABLE_REPARSE_POINT_NOT_SUPPORTED);
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PORTABLE_INSTALL_FAILED);
            }

            std::filesystem::path portableLinksDirectory = ConvertToUTF16(baseEntry[Normalize(s_PortableLinksDirectory)].value().GetValue<Value::Type::String>());

            bool isPathModified = false;
            AddToPathRegistry(portableLinksDirectory, ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope)), isPathModified);

            if (isPathModified)
            {
                context.Reporter.Warn() << Resource::String::ModifiedPathRequiresShellRestart << std::endl;
            }

            context.Reporter.Info() << Resource::String::InstallFlowInstallSuccess << std::endl;
        }
        catch (...)
        {
            context.SetTerminationHR(Workflow::HandleException(context, std::current_exception()));
        }
    }
}