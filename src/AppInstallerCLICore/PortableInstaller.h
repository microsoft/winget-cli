// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winget/PortableARPEntry.h"
#include "winget/PortableFileEntry.h"
#include <Workflows/WorkflowBase.h>
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
        bool BinariesDependOnPath = false;
        // If we fail to create a symlink, add install directory to PATH variable
        bool InstallDirectoryAddedToPath = false;

        bool IsUpdate = false;
        bool Purge = false;
        bool RecordToIndex = false;

        // This is the incoming target install location determined from the context args.
        std::filesystem::path TargetInstallLocation;

        PortableInstaller(Manifest::ScopeEnum scope, Utility::Architecture arch, const std::string& productCode);

        bool VerifyExpectedState();

        void SetDesiredState(const std::vector<AppInstaller::Portable::PortableFileEntry>& desiredEntries)
        {
            m_desiredEntries = desiredEntries;
        };

        void PrepareForCleanUp()
        {
            m_expectedEntries = m_desiredEntries;
            m_desiredEntries = {};
        }

        void Install(AppInstaller::CLI::Workflow::OperationType operation = Workflow::OperationType::Install);

        void Uninstall();

        template<typename T>
        void CommitToARPEntry(PortableValueName valueName, T value)
        {
            m_portableARPEntry.SetValue(valueName, value);
        }

        std::filesystem::path GetPortableIndexFileName()
        {
            return Utility::ConvertToUTF16(GetProductCode() + ".db");
        }

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

        AppInstaller::Manifest::AppsAndFeaturesEntry GetAppsAndFeaturesEntry();

    private:
        PortableARPEntry m_portableARPEntry;
        std::vector<AppInstaller::Portable::PortableFileEntry> m_desiredEntries;
        std::vector<AppInstaller::Portable::PortableFileEntry> m_expectedEntries;
        std::stringstream m_stream;

        std::string GetStringValue(PortableValueName valueName);
        std::filesystem::path GetPathValue(PortableValueName valueName);
        bool GetBoolValue(PortableValueName valueName);

        void SetExpectedState();
        void RegisterARPEntry();

        void ApplyDesiredState();
        void InstallFile(AppInstaller::Portable::PortableFileEntry& desiredState);
        void RemoveFile(AppInstaller::Portable::PortableFileEntry& desiredState);

        void CreateTargetInstallDirectory();
        void RemoveInstallDirectory();

        void AddToPathVariable(const std::filesystem::path& value);
        void RemoveFromPathVariable(const std::filesystem::path& value);
    };
}
