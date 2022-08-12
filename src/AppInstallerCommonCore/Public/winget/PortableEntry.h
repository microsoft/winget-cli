// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerStrings.h>
#include <AppInstallerSHA256.h>
#include "PortableARPEntry.h"
#include <filesystem>

namespace AppInstaller::Registry::Portable
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
        void CommitToRegistry(PortableValueName valueName, T value)
        {
            m_portableARPEntry.SetValue(valueName, value);
        }

        void RemoveARPEntry();

        PortableEntry(PortableARPEntry& portableARPEntry);

        void GetEntryValue(PortableValueName valueName, std::string& value);
        void GetEntryValue(PortableValueName valueName, std::filesystem::path& value);
        void GetEntryValue(PortableValueName valueName, bool& value);

    private:
        PortableARPEntry m_portableARPEntry;
    };

}