// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/SQLiteIndex.h"
#include <AppInstallerArchitecture.h>
#include <winget/Registry.h>
#include <winget/ManifestInstaller.h>
#include <wil/resource.h>

#include <string>

namespace AppInstaller::Repository::Microsoft
{
    // A helper to find the various locations that contain ARP (Add/Remove Programs) entries.
    struct ARPHelper
    {
        // See https://docs.microsoft.com/en-us/windows/win32/msi/uninstall-registry-key for details.
        std::wstring SubKeyPath{ L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall" };

        // REG_SZ
        std::wstring DisplayName{ L"DisplayName" };
        // REG_SZ
        std::wstring Publisher{ L"Publisher" };
        // REG_SZ
        std::wstring DisplayVersion{ L"DisplayVersion" };
        // REG_DWORD (ex. 0xMMmmbbbb, M[ajor], m[inor], b[uild])
        std::wstring Version{ L"Version" };
        // REG_DWORD
        std::wstring VersionMajor{ L"VersionMajor" };
        // REG_DWORD
        std::wstring VersionMinor{ L"VersionMinor" };
        // REG_SZ
        std::wstring URLInfoAbout{ L"URLInfoAbout" };
        // REG_SZ
        std::wstring HelpLink{ L"HelpLink" };
        // REG_SZ
        std::wstring InstallLocation{ L"InstallLocation" };
        // REG_DWORD (ex. 1033 [en-us])
        std::wstring Language{ L"Language" };
        // REG_SZ (ex. "english")
        std::wstring InnoSetupLanguage{ L"Inno Setup: Language" };
        // REG_EXPAND_SZ
        std::wstring UninstallString{ L"UninstallString" };
        // REG_EXPAND_SZ
        std::wstring QuietUninstallString{ L"QuietUninstallString" };
        // REG_DWORD (bool, true indicates MSI)
        std::wstring WindowsInstaller{ L"WindowsInstaller" };
        // REG_DWORD (bool)
        std::wstring SystemComponent{ L"SystemComponent" };

        // Gets the registry key associated with the given scope and architecture on this platform.
        // May return an empty key if there is no valid location (bad combination or not found).
        Registry::Key GetARPKey(Manifest::ManifestInstaller::ScopeEnum scope, Utility::Architecture architecture) const;

        // Returns true IFF the value exists and contains a non-zero DWORD.
        static bool GetBoolValue(const Registry::Key& arpKey, const std::wstring& name);

        // Determines the version from an ARP entry.
        // The priority is:
        //  DisplayVersion
        //  Version
        //  MajorVersion, MinorVersion
        std::string DetermineVersion(const Registry::Key& arpKey) const;

        // Reads a value and adds it to the metadata if it exists.
        static void AddMetadataIfPresent(const Registry::Key& key, const std::wstring& name, SQLiteIndex& index, SQLiteIndex::IdType manifestId, PackageVersionMetadata metadata);

        // Populates the index with the ARP entries from the given scope (machine/user).
        // Handles all of the architectures for the given scope.
        void PopulateIndexFromARP(SQLiteIndex& index, Manifest::ManifestInstaller::ScopeEnum scope) const;

        // Populates the index with the ARP entries from the given key.
        // This entry point is primarily to allow unit tests to operate of arbitrary keys;
        // product code should use PopulateIndexFromARP.
        void PopulateIndexFromKey(SQLiteIndex& index, const Registry::Key& key, std::string_view scope, std::string_view architecture) const;
    };
}
