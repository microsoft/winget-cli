// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerStrings.h>
#include <AppInstallerSHA256.h>
#include "PortableARPEntry.h"
#include <filesystem>

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

        // Assigns the value to the corresponding member and commits to the registry.
        template<typename T>
        void Commit(PortableValueName valueName, T& member, const T& value)
        {
            member = value;
            m_portableARPEntry.SetValue(valueName, member);
        }

        void RemoveARPEntry();

        Manifest::ScopeEnum GetScope() { return m_portableARPEntry.GetScope(); };

        PortableEntry(PortableARPEntry& portableARPEntry);

        void GetEntryValue(PortableValueName valueName, std::string& value);
        void GetEntryValue(PortableValueName valueName, std::filesystem::path& value);
        void GetEntryValue(PortableValueName valueName, bool& value);

    private:
        PortableARPEntry m_portableARPEntry;
    };

}