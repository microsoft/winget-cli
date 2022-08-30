// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerStrings.h>
#include <AppInstallerSHA256.h>
#include "winget/PortableARPEntry.h"
#include <AppInstallerRuntime.h>
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

        HRESULT SingleInstall(const std::filesystem::path& installerPath, const std::filesystem::path& alias, bool rename = false);

        HRESULT MultipleInstall(const std::vector<Manifest::NestedInstallerFile>& nestedInstallerFiles, const std::vector<std::filesystem::path>& extractedItems);

        template<typename T>
        void Commit(PortableValueName valueName, T value)
        {
            m_portableARPEntry.SetValue(valueName, value);
        }

        Manifest::ScopeEnum GetScope() { return m_portableARPEntry.GetScope(); };

        bool Exists() { return m_portableARPEntry.Exists(); };

        PortableEntry(Manifest::ScopeEnum scope, Utility::Architecture arch, const std::string& productCode, bool isUpdate = false);

        std::filesystem::path GetPathValue() const
        {
            return  InstallDirectoryAddedToPath ? InstallLocation : PortableSymlinkFullPath.parent_path();
        }

        static std::filesystem::path GetPortableLinksLocation(Manifest::ScopeEnum scope)
        {
            if (scope == Manifest::ScopeEnum::Machine)
            {
                return Runtime::GetPathTo(Runtime::PathName::PortableLinksMachineLocation);
            }
            else
            {
                return Runtime::GetPathTo(Runtime::PathName::PortableLinksUserLocation);
            }
        }

        static std::filesystem::path GetPortableInstallLocation(Manifest::ScopeEnum scope, Utility::Architecture arch)
        {
            if (scope == Manifest::ScopeEnum::Machine)
            {
                if (arch == Utility::Architecture::X86)
                {
                    return Runtime::GetPathTo(Runtime::PathName::PortablePackageMachineRootX86);
                }
                else
                {
                    return Runtime::GetPathTo(Runtime::PathName::PortablePackageMachineRootX64);
                }
            }
            else
            {
                return Runtime::GetPathTo(Runtime::PathName::PortablePackageUserRoot);
            }
        }


        void SetAppsAndFeaturesMetadata(const Manifest::AppsAndFeaturesEntry& entry, const Manifest::Manifest& manifest);

        bool VerifyPortableExeHash();

        bool VerifySymlinkTarget();

        void MovePortableExe(const std::filesystem::path& installerPath);

        void CreatePortableSymlink();

        bool RemoveFromPathVariable();

        void RemoveARPEntry();

        bool IsUpdate() { return m_isUpdate; };

    private:
        //std::unique_ptr<PortableIndex> m_portableIndex;
        bool m_isUpdate = false;
        bool m_addedToPath = false;
        std::string m_productCode;
        PortableARPEntry m_portableARPEntry;

        std::string GetStringValue(PortableValueName valueName);
        std::filesystem::path GetPathValue(PortableValueName valueName);
        bool GetBoolValue(PortableValueName valueName);
    };
}