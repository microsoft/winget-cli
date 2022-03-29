// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerRuntime.h"
#include "AppInstallerProgress.h"
#include "winget/Registry.h"
#include "winget/ManifestCommon.h"
#include "winget/Manifest.h"
#include "winget/UserSettings.h"

namespace AppInstaller::Portable
{
    using namespace std::string_view_literals;
    using namespace Runtime;
    using namespace Utility;

    const std::wstring_view appPathsRegistrySubkey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\";
    const std::wstring_view uninstallRegistrySubkey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";

    // Remove after merge with 1.2 schema changes
    constexpr std::string_view s_Microsoft = "Microsoft"sv;
    constexpr std::string_view s_WinGet = "WinGet"sv;
    constexpr std::string_view s_Packages = "Packages"sv;

    std::filesystem::path GetPortableInstallRoot(Manifest::ScopeEnum& scope, Utility::Architecture& arch, std::string_view& installLocationArg)
    {
        std::filesystem::path installLocation;
        std::string defaultPortableUserRoot = Settings::User().Get<Settings::Setting::PortableAppUserRoot>();
        std::string defaultPortableMachineRoot = Settings::User().Get<Settings::Setting::PortableAppMachineRoot>();

        if (!installLocationArg.empty())
        {
            return installLocationArg;
        }

        if (scope == Manifest::ScopeEnum::User || scope == Manifest::ScopeEnum::Unknown)
        {
            if (!defaultPortableUserRoot.empty())
            {
                return std::filesystem::path{ defaultPortableUserRoot };
            }
            else
            {
                installLocation /= Runtime::GetPathTo(Runtime::PathName::LocalAppData);
                installLocation /= s_Microsoft;
                installLocation /= s_WinGet;
                installLocation /= s_Packages;
            }
        }
        else if (scope == Manifest::ScopeEnum::Machine)
        {
            if (!defaultPortableMachineRoot.empty())
            {
                return std::filesystem::path{ defaultPortableMachineRoot };
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

    // Temporary until 1_2 settings schema changes get checked in.
    //std::filesystem::path GetPortableInstallRoot(Manifest::ScopeEnum& scope, Utility::Architecture& arch)
    //{
    //    if (scope == Manifest::ScopeEnum::Machine)
    //    {
    //        if (arch == Utility::Architecture::X86)
    //        {
    //            return GetPathTo(PathName::PortableAppMachineRootX86);
    //        }
    //        else
    //        {
    //            return GetPathTo(PathName::PortableAppMachineRootX64);
    //        }
    //    }
    //    else
    //    {
    //        return GetPathTo(PathName::PortableAppUserRoot);
    //    }
    //}

    bool WriteToAppPathsRegistry(HKEY root, std::string_view entryName, const std::filesystem::path& exePath, bool enablePath)
    {
        std::wstring exePathString = exePath.wstring();
        std::wstring pathString = exePath.parent_path().wstring();
        std::wstring fullRegistryKey = Normalize(appPathsRegistrySubkey) + ConvertToUTF16(entryName);
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
        std::wstring productCode = ConvertToUTF16(entry.ProductCode);
        std::wstring displayName = ConvertToUTF16(entry.DisplayName);
        std::wstring displayVersion = ConvertToUTF16(entry.DisplayVersion);
        std::wstring publisher = ConvertToUTF16(entry.Publisher);
        std::wstring uninstallString = L"winget uninstall --id " + ConvertToUTF16(packageIdentifier);
        std::wstring fullRegistryKey = Normalize(uninstallRegistrySubkey) + productCode;

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

    DWORD CALLBACK CopyPortableExeProgressCallback(
        LARGE_INTEGER TotalFileSize,
        LARGE_INTEGER TotalBytesTransferred,
        [[maybe_unused]] LARGE_INTEGER StreamSize,
        [[maybe_unused]] LARGE_INTEGER StreamBytesTransferred,
        DWORD dwStreamNumber,
        DWORD dwCallbackReason,
        [[maybe_unused]] HANDLE hSourceFile,
        [[maybe_unused]] HANDLE hDestinationFile,
        LPVOID lpData
    )
    {
        ProgressCallback callback = static_cast<ProgressCallback>((ProgressCallback*)lpData);

        if (callback.IsCancelled())
        {
            return PROGRESS_CANCEL;
        }

        if (dwCallbackReason == CALLBACK_STREAM_SWITCH || dwStreamNumber == 1)
        {
            callback.BeginProgress();
        }

        if (dwCallbackReason == CALLBACK_CHUNK_FINISHED)
        {
            callback.OnProgress(TotalBytesTransferred.QuadPart - 1, TotalFileSize.QuadPart, AppInstaller::ProgressType::Percent);
        }

        return PROGRESS_CONTINUE;
    }
}