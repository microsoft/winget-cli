// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/PortableEntry.h"
#include "winget/PortableARPEntry.h"
#include "winget/Manifest.h"
#include "winget/Filesystem.h"
#include "winget/PathVariable.h"
#include "Public/AppInstallerLogging.h"

using namespace AppInstaller::Registry;
using namespace AppInstaller::Registry::Portable;
using namespace AppInstaller::Registry::Environment;

namespace AppInstaller::Portable
{
    PortableEntry::PortableEntry(PortableARPEntry& portableARPEntry) :
        m_portableARPEntry(portableARPEntry)
    {
        // Initialize all values if present
        if (Exists())
        {
            DisplayName = GetStringValue(PortableValueName::DisplayName);
            DisplayVersion = GetStringValue(PortableValueName::DisplayVersion);
            HelpLink = GetStringValue(PortableValueName::HelpLink);
            InstallDate = GetStringValue(PortableValueName::InstallDate);
            Publisher = GetStringValue(PortableValueName::Publisher);
            SHA256 = GetStringValue(PortableValueName::SHA256);
            URLInfoAbout = GetStringValue(PortableValueName::URLInfoAbout);
            UninstallString = GetStringValue(PortableValueName::UninstallString);
            WinGetInstallerType = GetStringValue(PortableValueName::WinGetInstallerType);
            WinGetPackageIdentifier = GetStringValue(PortableValueName::WinGetPackageIdentifier);
            WinGetSourceIdentifier = GetStringValue(PortableValueName::WinGetSourceIdentifier);

            InstallLocation = GetPathValue(PortableValueName::InstallLocation);
            PortableSymlinkFullPath = GetPathValue(PortableValueName::PortableSymlinkFullPath);
            PortableTargetFullPath = GetPathValue(PortableValueName::PortableTargetFullPath);
            InstallLocation = GetPathValue(PortableValueName::InstallLocation);

            InstallDirectoryCreated = GetBoolValue(PortableValueName::InstallDirectoryCreated);
            InstallDirectoryAddedToPath = GetBoolValue(PortableValueName::InstallDirectoryAddedToPath);
        }
    }

    void PortableEntry::MovePortableExe(const std::filesystem::path& installerPath)
    {
        bool isDirectoryCreated = false;
        if (std::filesystem::create_directories(InstallLocation))
        {
            AICLI_LOG(Core, Info, << "Created target install directory: " << InstallLocation);
            isDirectoryCreated = true;
        }

        if (std::filesystem::exists(PortableTargetFullPath))
        {
            std::filesystem::remove(PortableTargetFullPath);
            AICLI_LOG(Core, Info, << "Removing existing portable exe at: " << PortableTargetFullPath);
        }

        Filesystem::RenameFile(installerPath, PortableTargetFullPath);
        AICLI_LOG(Core, Info, << "Portable exe moved to: " << PortableTargetFullPath);

        // Only assign this value if this is a new portable install or the install directory was actually created.
        // Otherwise, we want to preserve the existing value from the prior install.
        if (!Exists() || isDirectoryCreated)
        {
            Commit(PortableValueName::InstallDirectoryCreated, InstallDirectoryCreated = isDirectoryCreated);
        }
    }

    bool PortableEntry::VerifyPortableExeHash()
    {
        std::ifstream inStream{ PortableTargetFullPath, std::ifstream::binary };
        const Utility::SHA256::HashBuffer& targetFileHash = Utility::SHA256::ComputeHash(inStream);
        inStream.close();

        return Utility::SHA256::AreEqual(Utility::SHA256::ConvertToBytes(SHA256), targetFileHash);
    }

    void PortableEntry::CreatePortableSymlink()
    {
        if (Filesystem::CreateSymlink(PortableTargetFullPath, PortableSymlinkFullPath))
        {
            AICLI_LOG(Core, Info, << "Symlink created at: " << PortableSymlinkFullPath);
        }
        else
        {
            // Symlink creation should only fail if the user executes in user mode and non-admin.
            // Resort to adding install directory to PATH directly.
            AICLI_LOG(Core, Info, << "Portable install executed in user mode. Adding package directory to PATH.");
            Commit(PortableValueName::InstallDirectoryAddedToPath, InstallDirectoryAddedToPath = true);
        }
    }

    bool PortableEntry::VerifySymlinkTarget()
    {
        AICLI_LOG(Core, Info, << "Expected portable target path: " << PortableTargetFullPath);
        const std::filesystem::path& symlinkTargetPath = std::filesystem::read_symlink(PortableSymlinkFullPath);
        
        if (symlinkTargetPath == PortableTargetFullPath)
        {
            AICLI_LOG(Core, Info, << "Portable symlink target matches portable target path: " << symlinkTargetPath);
            return true;
        }
        else
        {
            AICLI_LOG(Core, Info, << "Portable symlink does not match portable target path: " << symlinkTargetPath);
            return false;
        }
    }

    bool PortableEntry::AddToPathVariable()
    {
        return PathVariable(GetScope()).Append(GetPathValue());
    }

    bool PortableEntry::RemoveFromPathVariable()
    {
        bool removeFromPath = true;
        std::filesystem::path pathValue = GetPathValue();
        if (!InstallDirectoryAddedToPath)
        {
            // Default links directory must be empty before removing from PATH.
            if (!std::filesystem::is_empty(pathValue))
            {
                AICLI_LOG(Core, Info, << "Install directory is not empty: " << pathValue);
                removeFromPath = false;
            }
        }

        if (removeFromPath)
        {
            return PathVariable(GetScope()).Remove(pathValue);
        }
        else
        {
            return false;
        }
    }

    void PortableEntry::RemoveARPEntry()
    {
        m_portableARPEntry.Delete();
    }

    std::string PortableEntry::GetStringValue(PortableValueName valueName)
    {
        if (m_portableARPEntry[valueName].has_value())
        {
            return m_portableARPEntry[valueName]->GetValue<Value::Type::String>();
        }
        else
        {
            return {};
        }
    }

    std::filesystem::path PortableEntry::GetPathValue(PortableValueName valueName)
    {
        if (m_portableARPEntry[valueName].has_value())
        {
            return m_portableARPEntry[valueName]->GetValue<Value::Type::UTF16String>();
        }
        {
            return {};
        }
    }

    bool PortableEntry::GetBoolValue(PortableValueName valueName)
    {
        if (m_portableARPEntry[valueName].has_value())
        {
            return m_portableARPEntry[valueName]->GetValue<Value::Type::DWord>();
        }
        else
        {
            return false;
        }
    }
}