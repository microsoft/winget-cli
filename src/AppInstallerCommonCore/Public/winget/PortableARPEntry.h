// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/Registry.h>
#include "Manifest.h"

namespace AppInstaller::Registry::Portable
{
    enum class PortableValueName
    {
        DisplayName,
        DisplayVersion,
        HelpLink,
        InstallDate,
        InstallDirectoryCreated,
        InstallLocation,
        PortableSymlinkFullPath,
        PortableTargetFullPath,
        Publisher,
        SHA256,
        URLInfoAbout,
        UninstallString,
        WinGetInstallerType,
        WinGetPackageIdentifier,
        WinGetSourceIdentifier,
        InstallDirectoryAddedToPath,
    };

    std::wstring_view ToString(PortableValueName valueName);

    struct PortableARPEntry : Registry::Key
    {
        PortableARPEntry(Manifest::ScopeEnum scope, Utility::Architecture arch, const std::string& productCode);

        std::optional<Value> operator[](PortableValueName valueName) const;

        bool Exists() { return m_exists; }

        void SetValue(PortableValueName valueName, const std::wstring& value);
        void SetValue(PortableValueName valueName, const std::string_view& value);
        void SetValue(PortableValueName valueName, bool& value);

        void Delete();

        Registry::Key GetKey() { return m_key; };
        Manifest::ScopeEnum GetScope() { return m_scope; };
        Utility::Architecture GetArchitecture() { return m_arch; };
        std::string GetProductCode() { return m_productCode; };

    private:
        bool m_exists = false;
        std::string m_productCode;
        Key m_key;
        HKEY m_root;
        std::wstring m_subKey;
        DWORD m_samDesired;
        Manifest::ScopeEnum m_scope;
        Utility::Architecture m_arch;
    };

}