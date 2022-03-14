// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PortableInstallFlow.h"
#include <filesystem>
#include <tlhelp32.h>

using namespace AppInstaller::Manifest;
using namespace std::filesystem;

namespace AppInstaller::CLI::Workflow
{
    // HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\App Paths
    // HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\App Paths

    std::wstring_view appPathsRegistrySubkey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\";
    std::wstring_view uninstallRegistrySubkey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";
    constexpr std::string_view s_PortableAppRoot_Base = "WinGet_Packages"sv;
    constexpr std::string_view s_UserScope = "User"sv;
    constexpr std::string_view s_MachineScope = "Machine"sv;
    std::wstring_view defaultValue = L"(Default)";

    bool IsExeRunning(std::string exeName, bool shouldEndProcess)
    {
        PROCESSENTRY32 pe32 = { 0 };
        HANDLE hSnap;
        int iDone;
        bool isProcessFound;
        bool isExeRunning = false;

        while (true)
        {
            hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            pe32.dwSize = sizeof(PROCESSENTRY32);
            Process32First(hSnap, &pe32);     // Can throw away, never an actual app

            isProcessFound = false;   //init values
            iDone = 1;

            while (iDone)   
            {
                iDone = Process32Next(hSnap, &pe32);
                std::wstring fileName{ pe32.szExeFile };
                std::wstring applicationName{ Utility::ConvertToUTF16(exeName) };
                if (fileName.compare(applicationName) == 0)    // Did we find our process?
                {
                    isExeRunning = true;
                    isProcessFound = true;
                    if (shouldEndProcess)
                    {
                        TerminateProcess(hSnap, 0);
                        isExeRunning = false;

                    }
                    iDone = 0;
                }
            }
            CloseHandle(hSnap);
            return isExeRunning;
        }
    }


    void WriteToAppPathsRegistry(AppInstaller::CLI::Execution::Reporter& out, const std::filesystem::path& entryName, const std::filesystem::path& exePath)
    {
        HKEY hkey;
        LONG lReg;

        std::wstring entryNameWString{ entryName };
        std::wstring registryKey{ appPathsRegistrySubkey};
        std::wstring exePathString = exePath.wstring();
        std::wstring pathString = exePath.parent_path().wstring();
        std::wstring fullRegistryKey = registryKey + entryNameWString;
        std::wstring defaultValueWString{ defaultValue };

        AppInstaller::Registry::Key key = Registry::Key::OpenIfExists(HKEY_CURRENT_USER, registryKey);

        AppInstaller::Registry::Key subKey;
        if (key.SubKey(entryNameWString).has_value())
        {
            subKey = key.SubKey(entryNameWString).value();
            Registry::Value value = subKey[defaultValueWString].value();
            std::string defaultStringValue = value.GetValue<Registry::Value::Type::String>();
            std::string base_filename = defaultStringValue.substr(defaultStringValue.find_last_of("/\\") + 1);

            bool shouldWarn = IsExeRunning(base_filename, false);
            out.Info() << shouldWarn << std::endl;
        }

        //create the key if it doesn't exist
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
            out.Info() << "Successfully opened registry key" << std::endl;

            // Set (Default) Property Value
            if (LONG res = RegSetValueEx(hkey, L"(Default)", NULL, REG_SZ, (LPBYTE)exePathString.c_str(), (DWORD)(exePathString.length() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
            {
                out.Error() << "Failed to write (Default) value. Error Code: " << res << std::endl;
            }

            // Set Path Property Value
            if (LONG res = RegSetValueEx(hkey, L"Path", NULL, REG_SZ, (LPBYTE)pathString.c_str(), (DWORD)(pathString.length() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
            {
                out.Error() << "Failed to write Path value. Error Code: " << res << std::endl;
            }
        }
    }

    void WriteToUninstallRegistry(AppInstaller::CLI::Execution::Reporter& out, std::string& packageIdentifier, AppsAndFeaturesEntry& entry)
    {
        HKEY hkey;
        LONG lReg;


        std::wstring registryKey{ uninstallRegistrySubkey };
        std::wstring productCode = Utility::ConvertToUTF16(entry.ProductCode);
        std::wstring displayName = Utility::ConvertToUTF16(entry.DisplayName);
        std::wstring displayVersion = Utility::ConvertToUTF16(entry.DisplayVersion);
        std::wstring publisher = Utility::ConvertToUTF16(entry.Publisher);
        std::wstring uninstallCommand = L"winget uninstall --id ";
        std::wstring uninstallString = uninstallCommand + Utility::ConvertToUTF16(packageIdentifier);

        std::wstring fullRegistryKey = registryKey + productCode;

        lReg = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
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
            out.Info() << "Successfully opened registry key" << std::endl;

            // Set DisplayName Property Value
            if (LONG res = RegSetValueEx(hkey, L"DisplayName", NULL, REG_SZ, (LPBYTE)displayName.c_str(), (DWORD)(displayName.length() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
            {
                out.Error() << "Failed to write DisplayName value. Error Code: " << res << std::endl;
            }

            // Set DisplayVersion Property Value
            if (LONG res = RegSetValueEx(hkey, L"DisplayVersion", NULL, REG_SZ, (LPBYTE)displayVersion.c_str(), (DWORD)(displayVersion.length() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
            {
                out.Error() << "Failed to write DisplayVersion value. Error Code: " << res << std::endl;
            }

            // Set Publisher Property Value
            if (LONG res = RegSetValueEx(hkey, L"Publisher", NULL, REG_SZ, (LPBYTE)publisher.c_str(), (DWORD)(publisher.length() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
            {
                out.Error() << "Failed to write Publisher value. Error Code: " << res << std::endl;
            }

            // Set DisplayVersion Property Value
            if (LONG res = RegSetValueEx(hkey, L"UninstallString", NULL, REG_SZ, (LPBYTE)uninstallString.c_str(), (DWORD)(uninstallString.length() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
            {
                out.Error() << "Failed to write UninstallString value. Error Code: " << res << std::endl;
            }
        }

    }

    path GetPortableAppInstallLocation(ScopeEnum scope, Utility::Architecture arch)
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
                if (arch == Utility::Architecture::X64)
                {
                    installLocation /= Runtime::GetPathTo(Runtime::PathName::ProgramFiles);
                }
                else if (arch == Utility::Architecture::X86)
                {
                    installLocation /= Runtime::GetPathTo(Runtime::PathName::ProgramFilesX86);
                }

                installLocation /= s_PortableAppRoot_Base;
                installLocation /= s_UserScope;
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

                installLocation /= s_PortableAppRoot_Base;
                installLocation /= s_MachineScope;
            }
        }

        return installLocation;
    }

    void PortableInstallImpl(Execution::Context& context)
    {
        context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;
        const std::filesystem::path& installerPath = context.Get<Execution::Data::InstallerPath>();

        context.Reporter.Info() << Runtime::GetPathTo(Runtime::PathName::ProgramFiles) << std::endl;
        context.Reporter.Info() << Runtime::GetPathTo(Runtime::PathName::ProgramFilesX86) << std::endl;

        Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
        // Ignore scope set in manifest, as by default we will be installing to User scope. Only override if --scope argument is provided.
        std::filesystem::path installLocation = GetPortableAppInstallLocation(scope, context.Get<Execution::Data::Installer>()->Arch);
        context.Reporter.Info() << installLocation << std::endl;


        std::string packageId = context.Get<Execution::Data::Manifest>().Id;
        std::string fileName = installerPath.filename().u8string();
        installLocation /= packageId;
        // check if install root directory exists, if not, then create one

        if (!std::filesystem::create_directories(installLocation))
        {
            context.Reporter.Error() << "Failed to create portable app directory" << std::endl;
        }
        else
        {
            // ensure that permissions 
        }

        // 1. obtain the app path registry entry based on the rename arg or command value.
        std::string appPathEntry;
        std::vector<AppInstaller::Manifest::string_t> commands = context.Get<Execution::Data::Installer>()->Commands;
        std::string_view renameString = context.Args.GetArg(Execution::Args::Type::Rename);

        if (!renameString.empty())
        {
            appPathEntry = renameString;
        }
        else if (commands.size() > 0)
        {
            appPathEntry = commands[0];
        }
        else
        {
            appPathEntry = installerPath.filename().u8string();
        }


        // 2. copy installer to portableAppRoot install location with the proper subdirectory based on the packageIdentifier
        // InstallRoot/PackageIdentifier
                //std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
        std::filesystem::path installerLocationFullPath = installLocation /= fileName;
        std::filesystem::copy_file(installerPath, installerLocationFullPath, std::filesystem::copy_options::overwrite_existing);


        // 3. Write to registry which involves several processes
        // AppPaths (Registering and installing an app)
                // check if the registry subkey already exists, if it does, then warn user...
                // if it does not exist, then create one with correct registry key values
                // if --force is used and the registry key already works, then edit values to point to new path, while removing the existing portable
        // Uninstall Key for registering to the AppsAndFeatures HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall (Creating a new entry for apps and Features/ uninstall string)
        WriteToAppPathsRegistry(context.Reporter, std::filesystem::path(appPathEntry), installLocation);


        // check for product code, if no product code exists, then use packageIdentifier.

        const auto& manifest = context.Get<Execution::Data::Manifest>();
        // TODO: pass in apps and features entry or create new one based on existing manifest data.
        std::vector<AppInstaller::Manifest::AppsAndFeaturesEntry> appsAndFeaturesEntries = context.Get<Execution::Data::Installer>()->AppsAndFeaturesEntries;
        AppInstaller::Manifest::AppsAndFeaturesEntry appsAndFeaturesEntry;
        if (appsAndFeaturesEntries.size() > 0)
        {
            appsAndFeaturesEntry = appsAndFeaturesEntries[0];
        }

        // check for empty values for apps and feature (might have weird behavior if I don't copy the values over)
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

        // 5. Register to Apps and Features
        WriteToUninstallRegistry(context.Reporter, packageId, appsAndFeaturesEntry);



    }
}
