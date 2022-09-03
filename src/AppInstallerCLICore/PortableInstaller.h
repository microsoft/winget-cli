// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winget/PortableARPEntry.h"
#include <filesystem>

using namespace AppInstaller::Registry::Portable;

namespace AppInstaller::CLI::Portable
{
    std::filesystem::path GetPortableLinksLocation(Manifest::ScopeEnum scope);

    std::filesystem::path GetPortableInstallRoot(Manifest::ScopeEnum scope, Utility::Architecture arch);

    // Object representation of the metadata and functionality required for installing a Portable package. 
    struct PortableInstaller
    {
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
        bool IsUpdate = false;
        // If we fail to create a symlink, add install directory to PATH variable
        bool InstallDirectoryAddedToPath = false;

        template<typename T>
        void Commit(PortableValueName valueName, T value)
        {
            m_portableARPEntry.SetValue(valueName, value);
        }

        std::filesystem::path GetInstallDirectoryForPathVariable()
        {
            return  InstallDirectoryAddedToPath ? InstallLocation : GetPortableLinksLocation(GetScope());
        }

        HRESULT SingleInstall(const std::filesystem::path& installerPath);

        HRESULT MultipleInstall(
            const std::vector<Manifest::NestedInstallerFile>& nestedInstallerFiles,
            const std::vector<std::filesystem::path>& extractedItems);

        HRESULT Uninstall(bool purge = false)
        {
            if (std::filesystem::exists(GetPortableIndexPath()))
            {
                return UninstallFromIndex(purge);
            }
            else
            {
                return UninstallSingle(purge);
            }
        }

        bool VerifyPortableFilesForUninstall();

        Manifest::ScopeEnum GetScope() { return m_portableARPEntry.GetScope(); };

        Utility::Architecture GetArch() { return m_portableARPEntry.GetArchitecture(); };

        std::string GetProductCode() { return m_portableARPEntry.GetProductCode(); };

        std::string GetPackageIdentifier() { return m_packageIdentifier; };

        std::string GetSourceIdentifier() { return m_sourceIdentifier; };

        bool Exists() { return m_portableARPEntry.Exists(); };

        PortableInstaller(Manifest::ScopeEnum scope, Utility::Architecture arch, const std::string& productCode);

        void SetAppsAndFeaturesMetadata(const Manifest::AppsAndFeaturesEntry& entry, const Manifest::Manifest& manifest);

        std::string GetOutputMessage()
        {
            return m_stream.str();
        }

        std::filesystem::path GetPortableIndexPath();

        void SetAppsAndFeaturesMetadata(const Manifest::Manifest& manifest, const std::vector<AppInstaller::Manifest::AppsAndFeaturesEntry>& entries);

    private:
        PortableARPEntry m_portableARPEntry;
        std::string m_packageIdentifier;
        std::string m_sourceIdentifier;
        std::stringstream m_stream;

        std::string GetStringValue(PortableValueName valueName);
        std::filesystem::path GetPathValue(PortableValueName valueName);
        bool GetBoolValue(PortableValueName valueName);

        void InitializeRegistryEntry();
        void FinalizeRegistryEntry();

        HRESULT UninstallSingle(bool purge = false);
        HRESULT UninstallFromIndex(bool purge = false);

        void MovePortableExe(const std::filesystem::path& installerPath);
        bool CreatePortableSymlink(const std::filesystem::path& targetPath, const std::filesystem::path& symlinkPath);

        void RemovePortableSymlink(const std::filesystem::path& targetPath, const std::filesystem::path& symlinkPath);
        void RemoveInstallDirectory(bool purge);
        void RemovePortableExe(const std::filesystem::path& targetPath, const std::string& hash);

        void AddToPathVariable();
        void RemoveFromPathVariable();
    };
}