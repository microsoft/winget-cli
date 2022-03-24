// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PortableInstallHelper.h"
#include <filesystem>

using namespace AppInstaller::Manifest;
using namespace std::filesystem;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        const std::wstring_view appPathsRegistrySubkey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\";
        const std::wstring_view uninstallRegistrySubkey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";
        constexpr std::string_view s_Microsoft = "Microsoft"sv;
        constexpr std::string_view s_WinGet = "WinGet"sv;
        constexpr std::string_view s_Packages = "Packages"sv;

        struct PortableArguments
        {
            HKEY RootKey;
            std::filesystem::path InstallRootDirectory;
            std::string AppPathEntry;
            std::string PackageId;
            Manifest::AppsAndFeaturesEntry AppsAndFeatureEntry;
        };

        std::filesystem::path GetPortableInstallRoot(Manifest::ScopeEnum& scope, Utility::Architecture& arch, std::string_view& installLocationArg)
        {
            path installLocation;
            std::string defaultPortableUserRoot = Settings::User().Get<Settings::Setting::PortableAppUserRoot>();
            std::string defaultPortableMachineRoot = Settings::User().Get<Settings::Setting::PortableAppMachineRoot>();

            if (!installLocationArg.empty())
            {
                return installLocationArg;
            }

            if (scope == ScopeEnum::User || scope == ScopeEnum::Unknown)
            {
                if (!defaultPortableUserRoot.empty())
                {
                    return path{ defaultPortableUserRoot };
                }
                else
                {
                    installLocation /= Runtime::GetPathTo(Runtime::PathName::LocalAppData);
                    installLocation /= s_Microsoft;
                    installLocation /= s_WinGet;
                    installLocation /= s_Packages;
                }
            }
            else if (scope == ScopeEnum::Machine)
            {
                if (!defaultPortableMachineRoot.empty())
                {
                    return path{ defaultPortableMachineRoot };
                }
                else
                {
                    if (arch == Utility::Architecture::X64)
                    {
                        installLocation /= Runtime::GetPathTo(Runtime::PathName::ProgramFiles);
                    }
                    else if (arch == Utility::Architecture::X86)
                    {
                        installLocation /= Runtime::GetPathTo(Runtime::PathName::ProgramFilesX86);
                    }

                    installLocation /= s_WinGet;
                    installLocation /= s_Packages;
                }
            }

            return installLocation;
        }

        bool WriteToAppPathsRegistry(HKEY root, std::string_view entryName, const std::filesystem::path& exePath, bool enablePath)
        {
            std::wstring entryNameWString = Utility::ConvertToUTF16(entryName);
            std::wstring registryKey{ appPathsRegistrySubkey };
            std::wstring exePathString = exePath.wstring();
            std::wstring pathString = exePath.parent_path().wstring();
            std::wstring fullRegistryKey = registryKey + entryNameWString;

            AppInstaller::Registry::Key key = Registry::Key::CreateKeyAndOpen(root, fullRegistryKey);

            bool result = key.SetKeyValue(L"", exePathString, REG_SZ);

            if (enablePath)
            {
                result = key.SetKeyValue(L"Path", pathString, REG_SZ);
            }

            return result;
        }

        bool WriteToUninstallRegistry(HKEY root, std::string_view packageIdentifier, Manifest::AppsAndFeaturesEntry& entry)
        {
            std::wstring registryKey{ uninstallRegistrySubkey };
            std::wstring productCode = Utility::ConvertToUTF16(entry.ProductCode);
            std::wstring displayName = Utility::ConvertToUTF16(entry.DisplayName);
            std::wstring displayVersion = Utility::ConvertToUTF16(entry.DisplayVersion);
            std::wstring publisher = Utility::ConvertToUTF16(entry.Publisher);
            std::wstring uninstallString = L"winget uninstall --id " + Utility::ConvertToUTF16(packageIdentifier);
            std::wstring fullRegistryKey = registryKey + productCode;

            AppInstaller::Registry::Key key = Registry::Key::CreateKeyAndOpen(root, fullRegistryKey);

            bool result;
            result = key.SetKeyValue(L"DisplayName", displayName, REG_SZ);
            result = key.SetKeyValue(L"DisplayVersion", displayVersion, REG_SZ);
            result = key.SetKeyValue(L"Publisher", publisher, REG_SZ);
            result = key.SetKeyValue(L"UninstallString", uninstallString, REG_SZ);

            return result;
        }

        bool CleanUpRegistryEdits(HKEY root, std::string& appPathEntry, std::string& productCode)
        {
            std::wstring fullAppPathSubkey = Utility::Normalize(appPathsRegistrySubkey) + Utility::ConvertToUTF16(appPathEntry);
            std::wstring fullUninstallSubkey = Utility::Normalize(uninstallRegistrySubkey) + Utility::ConvertToUTF16(productCode);

            bool result;
            result = Registry::Key::DeleteKey(root, fullAppPathSubkey);
            result = Registry::Key::DeleteKey(root, fullUninstallSubkey);

            return result;
        }

        Manifest::AppsAndFeaturesEntry GetAppsAndFeaturesEntryForPortableInstall(std::vector<AppInstaller::Manifest::AppsAndFeaturesEntry> appsAndFeaturesEntries, const AppInstaller::Manifest::Manifest& manifest)
        {
            AppInstaller::Manifest::AppsAndFeaturesEntry appsAndFeaturesEntry;
            std::string displayName = manifest.DefaultLocalization.Get<Localization::PackageName>();
            std::string displayVersion = manifest.Version;
            std::string publisher = manifest.DefaultLocalization.Get<Localization::Publisher>();
            std::string packageId = manifest.Id;

            if (appsAndFeaturesEntries.empty())
            {
                appsAndFeaturesEntry = {};
            }
            else
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

        // Add progress to this function
        bool CopyExeToPortableRoot(const std::filesystem::path& source, const std::filesystem::path& dest)
        {
            bool copyResult = std::filesystem::copy_file(source, dest, std::filesystem::copy_options::overwrite_existing);
            if (copyResult)
            {
                AICLI_LOG(Core, Verbose, << "Successfully copied copied portable exe to portable root folder '" << dest << "'.");
            }
            else
            {
                AICLI_LOG(Core, Verbose, << "Failed to copy portable exe to '" << dest << "'.");
            }

            return copyResult;
        }

        PortableArguments GetPortableInstallerArguments(Execution::Context& context)
        {
            PortableArguments portableArgs = {};

            Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));

            if (scope == Manifest::ScopeEnum::Machine)
            {
                portableArgs.RootKey = HKEY_LOCAL_MACHINE;
            }
            else
            {
                portableArgs.RootKey = HKEY_CURRENT_USER;
            }

            Utility::Architecture arch = context.Get<Execution::Data::Installer>()->Arch;
            std::string_view installLocationArg = context.Args.GetArg(Execution::Args::Type::InstallLocation);
            std::filesystem::path installerPath = context.Get<Execution::Data::InstallerPath>();
            std::string fileName = installerPath.filename().u8string();

            portableArgs.InstallRootDirectory = GetPortableInstallRoot(scope, arch, installLocationArg);

            std::vector<AppInstaller::Manifest::string_t> commands = context.Get<Execution::Data::Installer>()->Commands;
            std::string_view rename = context.Args.GetArg(Execution::Args::Type::Rename);

            if (!rename.empty())
            {
                portableArgs.AppPathEntry = rename;
            }
            else if (commands.size() > 0)
            {
                portableArgs.AppPathEntry = commands[0] + ".exe";
            }
            else
            {
                portableArgs.AppPathEntry = fileName;
            }

            std::vector<AppInstaller::Manifest::AppsAndFeaturesEntry> appsAndFeaturesEntries = context.Get<Execution::Data::Installer>()->AppsAndFeaturesEntries;
            AppInstaller::Manifest::Manifest manifest = context.Get<Execution::Data::Manifest>();
            portableArgs.PackageId = manifest.Id;
            portableArgs.AppsAndFeatureEntry = GetAppsAndFeaturesEntryForPortableInstall(appsAndFeaturesEntries, manifest);

            return portableArgs;
        }
    }

    bool InvokePortableInstall(const std::filesystem::path& installerPath, PortableArguments& portableArgs, IProgressCallback&)
    {
        std::filesystem::path portableInstallerPath;

        std::filesystem::path installRootPackageDirectory = portableArgs.InstallRootDirectory / portableArgs.PackageId;

        std::filesystem::create_directories(installRootPackageDirectory);
        std::string fileName = installerPath.filename().u8string();

        std::filesystem::path portableInstallerDestPath = installRootPackageDirectory / fileName;

        CopyExeToPortableRoot(installerPath, portableInstallerDestPath);


        bool installResult;
        installResult = WriteToAppPathsRegistry(portableArgs.RootKey, portableArgs.AppPathEntry, portableInstallerDestPath, true);
        installResult = WriteToUninstallRegistry(portableArgs.RootKey, portableArgs.PackageId, portableArgs.AppsAndFeatureEntry);

        return installResult;
    }

    void PortableInstallImpl(Execution::Context& context)
    {
        context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

        const std::filesystem::path& installerPath = context.Get<Execution::Data::InstallerPath>();

        PortableArguments portableArgs = GetPortableInstallerArguments(context);

        auto installResult = context.Reporter.ExecuteWithProgress(
            std::bind(InvokePortableInstall,
                installerPath,
                portableArgs,
                std::placeholders::_1));

        if (!installResult)
        {
            context.Reporter.Warn() << Resource::String::InstallationAbandoned << std::endl;
            AICLI_TERMINATE_CONTEXT(E_ABORT);
        }
    }
}
