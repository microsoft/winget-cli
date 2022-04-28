// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Registry.h"
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
    };

    std::wstring_view ToString(PortableValueName valueName);

    struct PortableARPEntry : Registry::Key
    {
        PortableARPEntry(Manifest::ScopeEnum scope, Utility::Architecture arch, const std::wstring& productCode);

        bool IsSamePortablePackageEntry(const std::string& packageId, const std::string& sourceId);

        bool Exists() { return m_exists; }

        void SetValue(PortableValueName valueName, const std::wstring& value);

        void SetValue(PortableValueName valueName, const std::string_view& value);

        void SetValue(PortableValueName valueName, bool& value);

        Registry::Key GetKey() { return m_key; };

    private:
        bool m_exists;
        Key m_key;
    };

}