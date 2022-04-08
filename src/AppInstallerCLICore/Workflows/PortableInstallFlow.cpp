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
        constexpr std::wstring_view s_PathSubkey_User = L"Environment";
        constexpr std::wstring_view s_PathSubkey_Machine = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
        constexpr std::wstring_view s_UninstallSubkey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";

        std::filesystem::path GetPortableInstallRoot(Manifest::ScopeEnum& scope, Utility::Architecture& arch)
        {
            if (scope == Manifest::ScopeEnum::Machine)
            {
                std::filesystem::path portableAppMachineRoot = Settings::User().Get<Settings::Setting::PortableAppMachineRoot>();
                if (!portableAppMachineRoot.empty())
                {
                    return portableAppMachineRoot;
                }
                else
                {
                    if (arch == Utility::Architecture::X86)
                    {
                        return Runtime::GetPathTo(Runtime::PathName::PortableAppMachineRootX86);
                    }
                    else
                    {
                        return Runtime::GetPathTo(Runtime::PathName::PortableAppMachineRootX64);
                    }
                }
            }
            else
            {
                std::filesystem::path portableAppUserRoot = Settings::User().Get<Settings::Setting::PortableAppUserRoot>();
                if (!portableAppUserRoot.empty())
                {
                    return portableAppUserRoot;
                }
                else
                {
                    return Runtime::GetPathTo(Runtime::PathName::PortableAppUserRoot);
                }
            }
        }

        std::filesystem::path GetPortableLinksLocation(Manifest::ScopeEnum& scope, Utility::Architecture& arch)
        {
            if (scope == Manifest::ScopeEnum::Machine)
            {
                if (arch == Utility::Architecture::X86)
                {
                    return Runtime::GetPathTo(Runtime::PathName::PortableAppMachineRootX86);
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

        Manifest::AppsAndFeaturesEntry GetAppsAndFeaturesEntryForPortableInstall(std::vector<AppInstaller::Manifest::AppsAndFeaturesEntry> appsAndFeaturesEntries, AppInstaller::Manifest::Manifest& manifest)
        {
            AppInstaller::Manifest::AppsAndFeaturesEntry appsAndFeaturesEntry;
            std::string packageId = manifest.Id;
            std::string displayVersion = manifest.Version;
            std::string displayName = manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>();
            std::string publisher = manifest.DefaultLocalization.Get<Manifest::Localization::Publisher>();

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

        void AppendExeExtension(std::string& value)
        {
            if (!HasSuffix(value, ".exe"))
            {
                value += ".exe";
            }
        }

        void AddPortableLinksDirToPathRegistry(Manifest::ScopeEnum& scope, Utility::Architecture& arch)
        {
            Key key;
            if (scope == Manifest::ScopeEnum::Machine)
            {
                key = Registry::Key::CreateKeyAndOpen(HKEY_LOCAL_MACHINE, Normalize(s_PathSubkey_Machine));
            }
            else
            {
                key = Registry::Key::CreateKeyAndOpen(HKEY_CURRENT_USER, Normalize(s_PathSubkey_User));
            }

            std::string portableLinksDir = GetPortableLinksLocation(scope, arch).u8string();
            AICLI_LOG(CLI, Info, << "Adding to Path environment variable: " << portableLinksDir);
            std::wstring pathKey = { L"Path" };
            std::string pathValue = key[pathKey]->GetValue<Value::Type::String>();
            if (pathValue.find(portableLinksDir) == std::string::npos)
            {
                std::string modifiedPathValue = pathValue;

                if (modifiedPathValue.back() != ';')
                {
                    modifiedPathValue += ";";
                }

                modifiedPathValue += portableLinksDir + ";";
                key.SetKeyValue(pathKey, ConvertToUTF16(modifiedPathValue), REG_EXPAND_SZ);
            }
        }

        void AddPortableEntryToUninstallRegistry(Manifest::ScopeEnum& scope, std::string_view packageId, AppsAndFeaturesEntry& entry)
        {
            HKEY root = (scope == Manifest::ScopeEnum::Machine) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
            std::wstring productCode = ConvertToUTF16(entry.ProductCode);
            std::wstring displayName = ConvertToUTF16(entry.DisplayName);
            std::wstring displayVersion = ConvertToUTF16(entry.DisplayVersion);
            std::wstring publisher = ConvertToUTF16(entry.Publisher);
            std::wstring uninstallString = L"winget uninstall --id " + ConvertToUTF16(packageId);
            std::wstring fullRegistryKey = Normalize(s_UninstallSubkey) + L"\\" + productCode;

            Key key = Key::CreateKeyAndOpen(root, fullRegistryKey);
            key.SetKeyValue(L"DisplayName", displayName, REG_SZ);
            key.SetKeyValue(L"DisplayVersion", displayVersion, REG_SZ);
            key.SetKeyValue(L"Publisher", publisher, REG_SZ);
            key.SetKeyValue(L"UninstallString", uninstallString, REG_SZ);
            AICLI_LOG(CLI, Info, << "Writing to Uninstall registry complete.");
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

        std::string fileName;
        if (!renameArg.empty())
        {
            fileName = Normalize(renameArg);
        }
        else
        {
            fileName = installerPath.filename().u8string();
        }

        std::filesystem::path targetInstallFullPath = targetInstallDirectory / fileName;
        return targetInstallFullPath;
    }

    void CreatePortableSymlink(Execution::Context& context)
    {
        const std::filesystem::path installerPath = context.Get<Execution::Data::InstallerPath>();
        Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
        Utility::Architecture arch = context.Get<Execution::Data::Installer>()->Arch;
        std::vector<string_t> commands = context.Get<Execution::Data::Installer>()->Commands;
        std::string_view renameArg = context.Args.GetArg(Execution::Args::Type::Rename);

        std::string fileName;
        std::string commandAlias;
        if (!renameArg.empty())
        {
            std::string renameArgValue = Normalize(renameArg);
            commandAlias = renameArgValue;
            fileName = renameArgValue;
        }
        else
        {
            commandAlias = !commands.empty() ? commands[0] : installerPath.filename().u8string();
        }

        AppendExeExtension(commandAlias);
        std::filesystem::path symlinkPath = GetPortableLinksLocation(scope, arch) / commandAlias;
        std::filesystem::path portableTargetFullPath = GetPortableTargetFullPath(context);
        AppInstaller::Filesystem::CreateSymlink(portableTargetFullPath, symlinkPath);
    }

    std::optional<DWORD> PortableCopyExeInstall(Execution::Context& context, IProgressCallback& progress)
    {
        const std::filesystem::path installerPath = context.Get<Execution::Data::InstallerPath>();
        std::string_view renameArg = context.Args.GetArg(Execution::Args::Type::Rename);

        std::string fileName;
        if (!renameArg.empty())
        {
            std::string renameArgValue = Normalize(renameArg);
            fileName = renameArgValue;
        }
        else
        {
            fileName = installerPath.filename().u8string();
        }

        AppendExeExtension(fileName);

        std::filesystem::path portableTargetFullPath = GetPortableTargetFullPath(context);
        AICLI_LOG(CLI, Info, << "Copying portable to: " << portableTargetFullPath);
        DWORD exitCode = AppInstaller::Filesystem::CopyFileWithProgressCallback(installerPath, portableTargetFullPath, progress);
        context.Add<Execution::Data::OperationReturnCode>(exitCode);
        return exitCode;
    }

    void PortableRegistryInstall(Execution::Context& context)
    {
        Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
        Utility::Architecture arch = context.Get<Execution::Data::Installer>()->Arch;

        std::vector<Manifest::AppsAndFeaturesEntry> appsAndFeatureEntries = context.Get<Execution::Data::Installer>()->AppsAndFeaturesEntries;
        AppInstaller::Manifest::Manifest manifest = context.Get<Execution::Data::Manifest>();
        Manifest::AppsAndFeaturesEntry entry = GetAppsAndFeaturesEntryForPortableInstall(appsAndFeatureEntries, manifest);

        AddPortableLinksDirToPathRegistry(scope, arch);
        AddPortableEntryToUninstallRegistry(scope, manifest.Id, entry);
    }
}