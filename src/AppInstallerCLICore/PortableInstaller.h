// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerStrings.h>
#include "winget/PortableARPEntry.h"
#include <filesystem>
#include <optional>

using namespace AppInstaller::Registry;
using namespace AppInstaller::Registry::Portable;

namespace AppInstaller::CLI::Portable
{
    // Helper methods for relevant locations.
    std::filesystem::path GetPortableLinksLocation(Manifest::ScopeEnum scope);

    std::filesystem::path GetPortableInstallRoot(Manifest::ScopeEnum scope, Utility::Architecture arch);

    // Object representation of the arguments needed to install a portable. 
    struct PortableInstaller
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
        bool IsUpdate = false;

        template<typename T>
        void Commit(PortableValueName valueName, T value)
        {
            m_portableARPEntry.SetValue(valueName, value);
        }

        Manifest::ScopeEnum GetScope() { return m_portableARPEntry.GetScope(); };

        Utility::Architecture GetArch() { return m_portableARPEntry.GetArchitecture(); };

        std::string GetProductCode() { return m_portableARPEntry.GetProductCode(); };

        std::string GetPackageIdentifier() { return m_packageIdentifier; };

        std::string GetSourceIdentifier() { return m_sourceIdentifier; };

        bool Exists() { return m_portableARPEntry.Exists(); };

        PortableInstaller(Manifest::ScopeEnum scope, Utility::Architecture arch, const std::string& productCode);

        HRESULT SingleInstall(const std::filesystem::path& installerPath);
        HRESULT MultipleInstall(const std::vector<Manifest::NestedInstallerFile>& nestedInstallerFiles, const std::vector<std::filesystem::path>& extractedItems);
        HRESULT Uninstall(bool purge = false);
        HRESULT UninstallFromIndex(bool purge = false);

        std::filesystem::path GetPathDirectory() const
        {
            return  InstallDirectoryAddedToPath ? InstallLocation : PortableSymlinkFullPath.parent_path();
        }

        void SetAppsAndFeaturesMetadata(const Manifest::AppsAndFeaturesEntry& entry, const Manifest::Manifest& manifest);

        void MovePortableExe(const std::filesystem::path& installerPath);

        void CreatePortableSymlink(const std::filesystem::path& targetPath, const std::filesystem::path& symlinkPath);

        bool VerifyPackageAndSourceIdentifiers()
        {
            if (Exists())
            {
                return m_portableARPEntry.IsSamePortablePackageEntry(m_packageIdentifier, m_sourceIdentifier);
            }
            else
            {
                return true;
            }
        };

        std::string GetOutputMessage()
        {
            return m_stream.str();
        }

        std::filesystem::path GetPortableIndexPath();

    private:
        PortableARPEntry m_portableARPEntry;
        std::string m_packageIdentifier;
        std::string m_sourceIdentifier;
        std::stringstream m_stream;

        void Initialize();

        std::string GetStringValue(PortableValueName valueName);
        std::filesystem::path GetPathValue(PortableValueName valueName);
        
        bool GetBoolValue(PortableValueName valueName);

        bool VerifySymlinkTarget(const std::filesystem::path& targetPath, const std::filesystem::path& symlinkPath);
        bool VerifyPortableExeHash(const std::filesystem::path& targetPath, const std::string& hashValue);

        void RemovePortableSymlink(const std::filesystem::path& targetPath, const std::filesystem::path& symlinkPath);
        void RemovePortableDirectory(const std::filesystem::path& directoryPath, bool purge, bool isCreated);
        void RemovePortableExe(const std::filesystem::path& targetPath, const std::string& hash);

        void AddToPathVariable();
        bool RemoveFromPathVariable();
    };
}