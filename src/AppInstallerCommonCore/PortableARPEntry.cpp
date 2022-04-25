// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/PortableARPEntry.h"
#include "winget/Manifest.h"

using namespace AppInstaller::Utility;

namespace AppInstaller::Registry::Portable
{
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
    constexpr std::wstring_view s_SHA256 = L"SHA256";
    constexpr std::wstring_view s_WinGetPackageIdentifier = L"WinGetPackageIdentifier";
    constexpr std::wstring_view s_WinGetSourceIdentifier = L"WinGetSourceIdentifier";

    PortableARPEntry::PortableARPEntry(Manifest::ScopeEnum scope, Utility::Architecture arch, const std::wstring& productCode)
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
        m_key = Key::OpenIfExists(root, subKey, 0, KEY_ALL_ACCESS);
        if (m_key != NULL)
        {
            m_isExisting = true;
        }
        else
        {
            m_key = Key::Create(root, subKey);
        }
    }

    std::wstring ToString(PortableValueName valueName)
    {
        std::wstring_view nameString;
        if (valueName == PortableValueName::DisplayName)
        {
            nameString = s_DisplayName;
        }
        else if (valueName == PortableValueName::DisplayVersion)
        {
            nameString = s_DisplayVersion;
        }
        else if (valueName == PortableValueName::Publisher)
        {
            nameString = s_Publisher;
        }
        else if (valueName == PortableValueName::InstallDate)
        {
            nameString = s_InstallDate;
        }
        else if (valueName == PortableValueName::URLInfoAbout)
        {
            nameString = s_URLInfoAbout;
        }
        else if (valueName == PortableValueName::HelpLink)
        {
            nameString = s_HelpLink;
        }
        else if (valueName == PortableValueName::UninstallString)
        {
            nameString = s_UninstallString;
        }
        else if (valueName == PortableValueName::WinGetInstallerType)
        {
            nameString = s_WinGetInstallerType;
        }
        else if (valueName == PortableValueName::InstallLocation)
        {
            nameString = s_InstallLocation;
        }
        else if (valueName == PortableValueName::PortableTargetFullPath)
        {
            nameString = s_PortableTargetFullPath;
        }
        else if (valueName == PortableValueName::PortableSymlinkFullPath)
        {
            nameString = s_PortableSymlinkFullPath;
        }
        else if (valueName == PortableValueName::SHA256)
        {
            nameString = s_SHA256;
        }
        else if (valueName == PortableValueName::WinGetPackageIdentifier)
        {
            nameString = s_WinGetPackageIdentifier;
        }
        else if (valueName == PortableValueName::WinGetSourceIdentifier)
        {
            nameString = s_WinGetSourceIdentifier;
        }

        return Normalize(nameString);
    }

    bool PortableARPEntry::IsSamePortablePackageEntry(const std::string& packageId, const std::string& sourceId)
    {
        auto existingWinGetPackageId = m_key[Normalize(s_WinGetPackageIdentifier)];
        auto existingWinGetSourceId = m_key[Normalize(s_WinGetSourceIdentifier)];

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

    void PortableARPEntry::SetValue(PortableValueName valueName, const std::wstring& value)
    {
        m_key.SetValue(ToString(valueName), value, REG_SZ);
    }
}