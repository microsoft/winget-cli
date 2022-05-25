// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/PortableARPEntry.h"
#include "winget/Manifest.h"

using namespace AppInstaller::Utility;

#define VALUENAMECASE(valueName) case PortableValueName::valueName: return s_##valueName;

namespace AppInstaller::Registry::Portable
{
    namespace
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
        constexpr std::wstring_view s_InstallDirectoryCreated = L"InstallDirectoryCreated";
    }

    PortableARPEntry::PortableARPEntry(Manifest::ScopeEnum scope, Utility::Architecture arch, const std::string& productCode)
    {
        m_scope = scope;
        m_arch = arch;

        if (m_scope == Manifest::ScopeEnum::Machine)
        {
            m_root = HKEY_LOCAL_MACHINE;
            if (m_arch == Utility::Architecture::X64)
            {
                m_subKey = s_UninstallRegistryX64;
                m_samDesired = KEY_WOW64_64KEY;
            }
            else
            {
                m_subKey = s_UninstallRegistryX86;
                m_samDesired = KEY_WOW64_32KEY;
            }
        }
        else
        {
            // HKCU uninstall registry share the x64 registry view.
            m_root = HKEY_CURRENT_USER;
            m_subKey = s_UninstallRegistryX64;
            m_samDesired = KEY_WOW64_64KEY;
        }

        m_subKey += L"\\" + ConvertToUTF16(productCode);
        m_key = Key::OpenIfExists(m_root, m_subKey, 0, KEY_ALL_ACCESS);
        if (m_key != NULL)
        {
            m_exists = true;
        }
        else
        {
            m_exists = false;
            m_key = Key::Create(m_root, m_subKey);
        }
    }

    std::wstring_view ToString(PortableValueName valueName)
    {
        switch (valueName)
        {
            VALUENAMECASE(DisplayName);
            VALUENAMECASE(DisplayVersion);
            VALUENAMECASE(Publisher);
            VALUENAMECASE(InstallDate);
            VALUENAMECASE(URLInfoAbout);
            VALUENAMECASE(HelpLink);
            VALUENAMECASE(UninstallString);
            VALUENAMECASE(WinGetInstallerType);
            VALUENAMECASE(InstallLocation);
            VALUENAMECASE(PortableTargetFullPath);
            VALUENAMECASE(PortableSymlinkFullPath);
            VALUENAMECASE(SHA256);
            VALUENAMECASE(WinGetPackageIdentifier);
            VALUENAMECASE(WinGetSourceIdentifier);
            VALUENAMECASE(InstallDirectoryCreated);
            default: return {};
        }
    }

    bool PortableARPEntry::IsSamePortablePackageEntry(const std::string& packageId, const std::string& sourceId)
    {
        auto existingWinGetPackageId = m_key[std::wstring{ s_WinGetPackageIdentifier }];
        auto existingWinGetSourceId = m_key[std::wstring{ s_WinGetSourceIdentifier }];

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

    std::optional<Value> PortableARPEntry::operator[](PortableValueName valueName) const
    {
        return m_key[std::wstring{ ToString(valueName) }];
    }

    void PortableARPEntry::SetValue(PortableValueName valueName, const std::wstring& value)
    {
        m_key.SetValue(std::wstring{ ToString(valueName) }, value, REG_SZ);
    }

    void PortableARPEntry::SetValue(PortableValueName valueName, const std::string_view& value)
    {
        m_key.SetValue(std::wstring{ ToString(valueName) }, ConvertToUTF16(value), REG_SZ);
    }

    void PortableARPEntry::SetValue(PortableValueName valueName, bool& value)
    {
        m_key.SetValue(std::wstring{ ToString(valueName) }, value);
    }

    void PortableARPEntry::Delete()
    {
        Registry::Key::Delete(m_root, m_subKey, m_samDesired);
    }
}