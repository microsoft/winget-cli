// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winget/PortableARPEntry.h"
#include "winget/PortableFileEntry.h"
#include <filesystem>

using namespace AppInstaller::Registry::Portable;

namespace AppInstaller::CLI::Portable
{
    std::filesystem::path GetPortableLinksLocation(Manifest::ScopeEnum scope);

    std::filesystem::path GetPortableInstallRoot(Manifest::ScopeEnum scope, Utility::Architecture arch);

    // Object representation of the metadata and functionality required for installing a Portable package. 
    struct PortableInstaller
    {
        // These values are initialized based on the values from the entry in ARP
        std::string DisplayName;
        std::string DisplayVersion;
        std::string HelpLink;
        std::string InstallDate;
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
        bool InstallDirectoryCreated = false;
        // If we fail to create a symlink, add install directory to PATH variable
        bool InstallDirectoryAddedToPath = false;

        bool IsUpdate = false;
        bool Purge = false;
        bool RecordToIndex = false;

        std::filesystem::path TargetInstallDirectory;

        void InstallFile(AppInstaller::Portable::PortableFileEntry& desiredState);

        void RemoveFile(AppInstaller::Portable::PortableFileEntry& desiredState);

        PortableInstaller(Manifest::ScopeEnum scope, Utility::Architecture arch, const std::string& productCode);

        // Ensures no modifications have been made that conflict with the expected state.
        bool VerifyExpectedState();

        void SetDesiredState(std::vector<AppInstaller::Portable::PortableFileEntry>& desiredEntries)
        {
            m_desiredEntries = desiredEntries;
        };

        HRESULT Install();

        HRESULT Uninstall();

        template<typename T>
        void CommitToARPEntry(PortableValueName valueName, T value)
        {
            m_portableARPEntry.SetValue(valueName, value);
        }

        std::filesystem::path GetInstallDirectoryForPathVariable()
        {
            return  InstallDirectoryAddedToPath ? TargetInstallDirectory : GetPortableLinksLocation(GetScope());
        }

        std::filesystem::path GetPortableIndexPath();

        Manifest::ScopeEnum GetScope() { return m_portableARPEntry.GetScope(); };

        Utility::Architecture GetArch() { return m_portableARPEntry.GetArchitecture(); };

        std::string GetProductCode() { return m_portableARPEntry.GetProductCode(); };

        bool ARPEntryExists() { return m_portableARPEntry.Exists(); };

        std::string GetOutputMessage()
        {
            return m_stream.str();
        }

        void SetAppsAndFeaturesMetadata(
            const Manifest::Manifest& manifest,
            const std::vector<AppInstaller::Manifest::AppsAndFeaturesEntry>& entries);

    private:
        PortableARPEntry m_portableARPEntry;
        std::vector<AppInstaller::Portable::PortableFileEntry> m_desiredEntries;
        std::vector<AppInstaller::Portable::PortableFileEntry> m_expectedEntries;
        std::stringstream m_stream;

        std::string GetStringValue(PortableValueName valueName);
        std::filesystem::path GetPathValue(PortableValueName valueName);
        bool GetBoolValue(PortableValueName valueName);

        std::vector<AppInstaller::Portable::PortableFileEntry> GetExpectedState();

        void InitializeRegistryEntry();

        void ApplyDesiredState();

        void CreateInstallDirectory();
        void RemoveInstallDirectory();

        bool CreatePortableSymlink(const std::filesystem::path& targetPath, const std::filesystem::path& symlinkPath);

        void AddToPathVariable();
        void RemoveFromPathVariable();
    };
}