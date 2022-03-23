// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PortableInstallHelper.h"
#include <filesystem>
#include <tlhelp32.h>

using namespace AppInstaller::Manifest;
using namespace std::filesystem;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        std::wstring_view appPathsRegistrySubkey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\";
        std::wstring_view uninstallRegistrySubkey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";
        constexpr std::string_view s_Microsoft = "Microsoft"sv;
        constexpr std::string_view s_WinGet = "WinGet"sv;
        constexpr std::string_view s_Packages = "Packages"sv;

        struct PortableArguments
        {
            std::string_view AppPathEntryValue;
            Manifest::ScopeEnum Scope;
            Utility::Architecture Arch;
            std::string_view PackageId;
            Manifest::AppsAndFeaturesEntry AppsAndFeatureEntry;
        };
    }

    void PortableInstallImpl(Execution::Context& context)
    {
        const std::filesystem::path& installerPath = context.Get<Execution::Data::InstallerPath>();
        Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
        Utility::Architecture arch = context.Get<Execution::Data::Installer>()->Arch;
        std::string packageId = context.Get<Execution::Data::Manifest>().Id;
        std::string fileName = installerPath.filename().u8string();

        std::filesystem::path installLocation = GetPortableInstallLocation(scope, arch);
        installLocation /= packageId;

        if (!std::filesystem::create_directories(installLocation))
        {
            context.Reporter.Error() << "Failed to create portable app directory" << std::endl;
        }

        std::string appPathEntry;
        std::vector<AppInstaller::Manifest::string_t> commands = context.Get<Execution::Data::Installer>()->Commands;
        std::string_view renameString = context.Args.GetArg(Execution::Args::Type::Rename);

        if (!renameString.empty())
        {
            appPathEntry = renameString;
        }
        else if (commands.size() > 0)
        {
            appPathEntry = commands[0] + ".exe";
        }
        else
        {
            appPathEntry = fileName;
        }

        std::filesystem::path destination = installLocation /= fileName;
        if (!std::filesystem::copy_file(installerPath, destination, std::filesystem::copy_options::overwrite_existing))
        {
            context.Reporter.Error() << "Failed to overwrite existing executable." << std::endl;
        }
        else {
            context.Reporter.Info() << "Successfully copied file" << std::endl;
        }

        WriteToAppPathsRegistry(appPathEntry, destination, true);

        const auto& manifest = context.Get<Execution::Data::Manifest>();
        std::vector<AppInstaller::Manifest::AppsAndFeaturesEntry> appsAndFeaturesEntries = context.Get<Execution::Data::Installer>()->AppsAndFeaturesEntries;
        AppInstaller::Manifest::AppsAndFeaturesEntry appsAndFeaturesEntry;
        // what happens if apps and features entries is empty? create a new one
        if (appsAndFeaturesEntries.size() > 0)
        {
            appsAndFeaturesEntry = appsAndFeaturesEntries[0];
        }

        // check for empty values for apps and feature
        if (appsAndFeaturesEntry.DisplayName.empty())
        {
            appsAndFeaturesEntry.DisplayName = manifest.DefaultLocalization.Get<Localization::PackageName>();
        }
        if (appsAndFeaturesEntry.DisplayVersion.empty())
        {
            appsAndFeaturesEntry.DisplayVersion = context.Get<Execution::Data::Manifest>().Version;
        }
        if (appsAndFeaturesEntry.Publisher.empty())
        {
            appsAndFeaturesEntry.Publisher = manifest.DefaultLocalization.Get<Localization::Publisher>();
        }
        if (appsAndFeaturesEntry.ProductCode.empty())
        {
            appsAndFeaturesEntry.ProductCode = packageId;
        }

        //Uninstall registry is located at HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Uninstall
        WriteToUninstallRegistry(scope, packageId, appsAndFeaturesEntry);

        // Handle with special case.
        if (true)
        {
            context.Reporter.Info() << Resource::String::InstallFlowInstallSuccess << std::endl;
        }
        else
        {
            context.Reporter.Error() << Resource::String::InstallFlowInstallSuccess << std::endl;
        }
    }

    std::filesystem::path GetPortableInstallLocation(Manifest::ScopeEnum scope, Utility::Architecture arch)
    {
        path installLocation;
        std::string defaultPortableUserRoot = Settings::User().Get<Settings::Setting::PortableAppUserRoot>();
        std::string defaultPortableMachineRoot = Settings::User().Get<Settings::Setting::PortableAppMachineRoot>();

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

    void WriteToAppPathsRegistry(std::string_view entryName, const std::filesystem::path& exePath, bool enablePath)
    {
        HKEY hkey;
        LONG lReg;

        std::wstring entryNameWString = Utility::ConvertToUTF16(entryName);
        std::wstring registryKey{ appPathsRegistrySubkey };
        std::wstring exePathString = exePath.wstring();
        std::wstring pathString = exePath.parent_path().wstring();
        std::wstring fullRegistryKey = registryKey + entryNameWString;


        AppInstaller::Registry::Key key = Registry::Key::CreateKeyAndOpen(HKEY_CURRENT_USER, fullRegistryKey);
        // Registry::Key::SetValue(null)
        // Registry::Key::SetValue(path)


        //AppInstaller::Registry::Key subKey;
        //if (key.SubKey(entryNameWString).has_value())
        //{
        //    subKey = key.SubKey(entryNameWString).value();
        //    if (subKey[defaultValueWString].has_value())
        //    {
        //        Registry::Value value = subKey[defaultValueWString].value();
        //        std::string defaultStringValue = value.GetValue<Registry::Value::Type::String>();
        //        std::string base_filename = defaultStringValue.substr(defaultStringValue.find_last_of("/\\") + 1);
        //    }
        //}


        lReg = RegCreateKeyEx(
            HKEY_CURRENT_USER,
            fullRegistryKey.c_str(),
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &hkey,
            NULL);

        if (lReg == ERROR_SUCCESS)
        {

            // Set (Default) Property Value
            if (LONG res = RegSetValueEx(hkey, NULL, NULL, REG_SZ, (LPBYTE)exePathString.c_str(), (DWORD)(exePathString.length() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
            {
                // do something
            }

            if (enablePath &&
                RegSetValueEx(hkey, L"Path", NULL, REG_SZ, (LPBYTE)pathString.c_str(), (DWORD)(pathString.length() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
            {
                // do something
            }
        }
    }

    bool WriteToUninstallRegistry(Manifest::ScopeEnum scope, std::string& packageIdentifier, Manifest::AppsAndFeaturesEntry& entry)
    {
        HKEY hkey;
        LONG lReg;

        std::wstring registryKey{ uninstallRegistrySubkey };
        std::wstring productCode = Utility::ConvertToUTF16(entry.ProductCode);
        std::wstring displayName = Utility::ConvertToUTF16(entry.DisplayName);
        std::wstring displayVersion = Utility::ConvertToUTF16(entry.DisplayVersion);
        std::wstring publisher = Utility::ConvertToUTF16(entry.Publisher);
        std::wstring uninstallString = L"winget uninstall --id " + Utility::ConvertToUTF16(packageIdentifier);
        std::wstring fullRegistryKey = registryKey + productCode;

        HKEY entryPoint;
        if (scope == Manifest::ScopeEnum::Machine)
        {
            entryPoint = HKEY_LOCAL_MACHINE;
        }
        else
        {
            entryPoint = HKEY_CURRENT_USER;
        }

        lReg = RegCreateKeyEx(
            entryPoint,
            fullRegistryKey.c_str(),
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &hkey,
            NULL);

        if (lReg == ERROR_SUCCESS)
        {

            if (LONG res = RegSetValueEx(hkey, L"DisplayName", NULL, REG_SZ, (LPBYTE)displayName.c_str(), (DWORD)(displayName.length() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
            {
                // What should we do if we fail to set the value?
            }

            if (LONG res = RegSetValueEx(hkey, L"DisplayVersion", NULL, REG_SZ, (LPBYTE)displayVersion.c_str(), (DWORD)(displayVersion.length() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
            {
            }

            if (LONG res = RegSetValueEx(hkey, L"Publisher", NULL, REG_SZ, (LPBYTE)publisher.c_str(), (DWORD)(publisher.length() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
            {
            }

            if (LONG res = RegSetValueEx(hkey, L"UninstallString", NULL, REG_SZ, (LPBYTE)uninstallString.c_str(), (DWORD)(uninstallString.length() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
            {
            }

            return true;
        }

        return false;
    }
}
