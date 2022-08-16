// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerStrings.h>
#include <AppInstallerSHA256.h>
#include "winget/PortableARPEntry.h"
#include <filesystem>
#include <optional>

using namespace AppInstaller::Registry;
using namespace AppInstaller::Registry::Portable;

namespace AppInstaller::Portable
{
    struct PortableEntry
    {
        std::string DisplayName;
        std::string DisplayVersion;
        std::string HelpLink;
        std::string InstallDate;
        bool InstallDirectoryCreated = false;
        std::filesystem::path InstallLocation;
        std::filesystem::path PortableSymlinkFullPath;
        std::filesystem::path PortableTargetFullPath;
        std::string Publisher;
        std::string SHA256;
        std::string URLInfoAbout;
        std::string UninstallString;
        std::string WinGetInstallerType;
        std::string WinGetPackageIdentifier;
        std::string WinGetSourceIdentifier;
        bool InstallDirectoryAddedToPath = false;

        template<typename T>
        void Commit(PortableValueName valueName, T value)
        {
            m_portableARPEntry.SetValue(valueName, value);
        }

        Manifest::ScopeEnum GetScope() { return m_portableARPEntry.GetScope(); };

        bool Exists() { return m_portableARPEntry.Exists(); };

        PortableEntry(PortableARPEntry& portableARPEntry);

        std::filesystem::path GetPathValue() const
        {
            return  InstallDirectoryAddedToPath ? InstallLocation : PortableSymlinkFullPath.parent_path();
        }

        bool VerifyPortableExeHash();

        bool VerifySymlinkTarget();

        void MovePortableExe(const std::filesystem::path& installerPath);

        void CreatePortableSymlink();

        bool AddToPathVariable();

        bool RemoveFromPathVariable();

        void RemoveARPEntry();

    private:
        PortableARPEntry m_portableARPEntry;
        std::string GetStringValue(PortableValueName valueName);
        std::filesystem::path GetPathValue(PortableValueName valueName);
        bool GetBoolValue(PortableValueName valueName);
    };
}