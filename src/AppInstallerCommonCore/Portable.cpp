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

    constexpr std::wstring_view s_UninstallSubkey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";
    constexpr std::wstring_view s_PathSubkey_User = L"Environment";
    constexpr std::wstring_view s_PathSubkey_Machine = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";

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

    bool AddToPathEnvironmentRegistry(HKEY root, const std::string& value)
    {
        AppInstaller::Registry::Key key;
        if (root == HKEY_LOCAL_MACHINE)
        {
            key = Registry::Key::CreateKeyAndOpen(root, Normalize(s_PathSubkey_Machine));
        }
        else
        {
            key = Registry::Key::CreateKeyAndOpen(root, Normalize(s_PathSubkey_User));
        }

        std::wstring pathKey = { L"Path" };
        std::string pathValue = key[pathKey]->GetValue<AppInstaller::Registry::Value::Type::String>();
        if (pathValue.find(value) == std::string::npos)
        {
            std::string modifiedPathValue = pathValue;

            if (modifiedPathValue.back() != ';')
            {
                modifiedPathValue += ";";
            }

            modifiedPathValue += value + ";";
            return key.SetKeyValue(pathKey, ConvertToUTF16(modifiedPathValue), REG_EXPAND_SZ);
        }

        return true;
    }

    void CreateSymlink(const std::filesystem::path& target, const std::filesystem::path& link)
    {
        if (std::filesystem::exists(link))
        {
            std::filesystem::remove(link);
        }

        std::filesystem::create_symlink(target, link);
    }

    bool WriteToUninstallRegistry(HKEY root, std::string_view packageIdentifier, Manifest::AppsAndFeaturesEntry& entry)
    {
        std::wstring productCode = ConvertToUTF16(entry.ProductCode);
        std::wstring displayName = ConvertToUTF16(entry.DisplayName);
        std::wstring displayVersion = ConvertToUTF16(entry.DisplayVersion);
        std::wstring publisher = ConvertToUTF16(entry.Publisher);
        std::wstring uninstallString = L"winget uninstall --id " + ConvertToUTF16(packageIdentifier);
        std::wstring fullRegistryKey = Normalize(s_UninstallSubkey) + productCode;

        AppInstaller::Registry::Key key = Registry::Key::CreateKeyAndOpen(root, fullRegistryKey);

        bool result;
        result = key.SetKeyValue(L"DisplayName", displayName, REG_SZ);
        result = key.SetKeyValue(L"DisplayVersion", displayVersion, REG_SZ);
        result = key.SetKeyValue(L"Publisher", publisher, REG_SZ);
        result = key.SetKeyValue(L"UninstallString", uninstallString, REG_SZ);

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
            callback.OnProgress(TotalBytesTransferred.QuadPart, TotalFileSize.QuadPart, AppInstaller::ProgressType::Percent);
        }

        if (TotalBytesTransferred.QuadPart == TotalFileSize.QuadPart)
        {
            callback.EndProgress(true);
        }

        return PROGRESS_CONTINUE;
    }
}