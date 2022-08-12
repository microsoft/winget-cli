// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/PortableEntry.h"
#include "winget/PortableARPEntry.h"
#include "winget/Manifest.h"

using namespace AppInstaller::Registry;
using namespace AppInstaller::Registry::Portable;

#define VALUENAMECASE(valueName) case PortableValueName::valueName: return ##valueName;

namespace AppInstaller::Portable
{
    void PortableEntry::RemoveARPEntry()
    {
        m_portableARPEntry.Delete();
    }

    void PortableEntry::GetEntryValue(PortableValueName valueName, std::string& value)
    {
        if (m_portableARPEntry[valueName].has_value())
        {
            value = m_portableARPEntry[valueName]->GetValue<Value::Type::String>();
        }
    }    
    
    void PortableEntry::GetEntryValue(PortableValueName valueName, std::filesystem::path& value)
    {
        if (m_portableARPEntry[valueName].has_value())
        {
            value = m_portableARPEntry[valueName]->GetValue<Value::Type::UTF16String>();
        }
    }

    void PortableEntry::GetEntryValue(PortableValueName valueName, bool& value)
    {
        if (m_portableARPEntry[valueName].has_value())
        {
            value = m_portableARPEntry[valueName]->GetValue<Value::Type::DWord>();
        }
    }

    PortableEntry::PortableEntry(PortableARPEntry& portableARPEntry) :
        m_portableARPEntry(portableARPEntry)
    {
        // Initialize all values if present
        if (m_portableARPEntry.Exists())
        {
            GetEntryValue(PortableValueName::DisplayName, DisplayName);
            GetEntryValue(PortableValueName::DisplayVersion, DisplayVersion);
            GetEntryValue(PortableValueName::HelpLink, HelpLink);
            GetEntryValue(PortableValueName::InstallDate, InstallDate);
            GetEntryValue(PortableValueName::InstallDirectoryCreated, InstallDirectoryCreated);
            GetEntryValue(PortableValueName::InstallLocation, InstallLocation);
            GetEntryValue(PortableValueName::PortableSymlinkFullPath, PortableSymlinkFullPath);
            GetEntryValue(PortableValueName::PortableTargetFullPath, PortableTargetFullPath);
            GetEntryValue(PortableValueName::Publisher, Publisher);
            GetEntryValue(PortableValueName::SHA256, SHA256);
            GetEntryValue(PortableValueName::URLInfoAbout, URLInfoAbout);
            GetEntryValue(PortableValueName::UninstallString, UninstallString);
            GetEntryValue(PortableValueName::WinGetInstallerType, WinGetInstallerType);
            GetEntryValue(PortableValueName::WinGetPackageIdentifier, WinGetPackageIdentifier);
            GetEntryValue(PortableValueName::WinGetSourceIdentifier, WinGetSourceIdentifier);
            GetEntryValue(PortableValueName::InstallDirectoryAddedToPath, InstallDirectoryAddedToPath);
        }
    }
}