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
        Publisher,
        InstallDate,
        URLInfoAbout,
        HelpLink,
        UninstallString,
        WinGetInstallerType,
        InstallLocation,
        PortableTargetFullPath,
        PortableSymlinkFullPath,
        SHA256,
        WinGetPackageIdentifier,
        WinGetSourceIdentifier,
    };

    std::wstring ToString(PortableValueName valueName);

    struct PortableARPEntry : Registry::Key
    {
        PortableARPEntry(Manifest::ScopeEnum scope, Utility::Architecture arch, const std::wstring& productCode);

        bool IsSamePortablePackageEntry(const std::string& packageId, const std::string& sourceId);

        bool Exists() { return m_exists; }

        void SetValue(PortableValueName valueName, const std::wstring& value);

        Registry::Key GetKey() { return m_key; };

    private:
        bool m_exists;
        Key m_key;
    };

}