// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/PortableEntry.h"
#include "winget/PortableARPEntry.h"
#include "winget/Manifest.h"
#include "winget/Filesystem.h"
#include "winget/PathVariable.h"
#include "Public/AppInstallerLogging.h"
#include <AppInstallerErrors.h>
#include <AppInstallerDateTime.h>

using namespace AppInstaller::Registry;
using namespace AppInstaller::Registry::Portable;
using namespace AppInstaller::Registry::Environment;

namespace AppInstaller::Portable
{
    namespace
    {
        constexpr std::string_view c_PortableIndexFileName = "portable.db";

        void AppendExeExtension(std::filesystem::path& value)
        {
            if (value.extension() != ".exe")
            {
                value += ".exe";
            }
        }
    }

    PortableEntry::PortableEntry(Manifest::ScopeEnum scope, Utility::Architecture arch, const std::string& productCode, bool isUpdate) :
        m_productCode(productCode), m_portableARPEntry(PortableARPEntry(scope, arch, productCode)), m_isUpdate(isUpdate)
    {
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

    HRESULT PortableEntry::MultipleInstall(const std::vector<Manifest::NestedInstallerFile>& nestedInstallerFiles, const std::vector<std::filesystem::path>& extractedItems)
    {
        // Add each file/ directory to database.
        if (extractedItems.size() == 0)
        {

        }

        for (const auto& nestedInstallerFile : nestedInstallerFiles)
        {
            // This is supposed to be where the portable exe is located.
            PortableTargetFullPath = InstallLocation / ConvertToUTF16(nestedInstallerFile.RelativeFilePath);

            // Create symlink
            if (!InstallDirectoryAddedToPath)
            {
                PortableSymlinkFullPath = GetPortableLinksLocation(GetScope()) / ConvertToUTF16(nestedInstallerFile.PortableCommandAlias);
                Commit(PortableValueName::PortableSymlinkFullPath, PortableSymlinkFullPath);

                std::filesystem::file_status status = std::filesystem::status(PortableSymlinkFullPath);
                if (std::filesystem::is_directory(status))
                {
                    AICLI_LOG(CLI, Info, << "Unable to create symlink. '" << PortableSymlinkFullPath << "points to an existing directory.");
                    return APPINSTALLER_CLI_ERROR_PORTABLE_SYMLINK_PATH_IS_DIRECTORY;
                }

                if (std::filesystem::remove(PortableSymlinkFullPath))
                {
                    AICLI_LOG(CLI, Info, << "Removed existing file at " << PortableSymlinkFullPath);
                    //context.Reporter.Warn() << Resource::String::OverwritingExistingFileAtMessage << ' ' << PortableSymlinkFullPath.u8string() << std::endl;
                }

                CreatePortableSymlink();
            }
            else
            {
                AICLI_LOG(CLI, Info, << "Package directory was previously added to PATH. Skipping symlink creation.");
            }
        }

        // Add to path
        const std::filesystem::path& pathValue = GetPathValue();
        if (PathVariable(GetScope()).Append(pathValue))
        {
            AICLI_LOG(Core, Info, << "Appended target directory to PATH registry: " << pathValue);
            //context.Reporter.Warn() << Resource::String::ModifiedPathRequiresShellRestart << std::endl;
            m_addedToPath = true;
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Target directory already exists in PATH registry: " << pathValue);
        }

        return ERROR_SUCCESS;
    }

    HRESULT PortableEntry::SingleInstall(const std::filesystem::path& installerPath, const std::filesystem::path& commandAlias, bool rename)
    {
        // Initial registration.
        Commit(PortableValueName::WinGetPackageIdentifier, WinGetPackageIdentifier);
        Commit(PortableValueName::WinGetSourceIdentifier, WinGetSourceIdentifier);
        Commit(PortableValueName::UninstallString, UninstallString = "winget uninstall --product-code " + m_productCode);
        Commit(PortableValueName::WinGetInstallerType, WinGetInstallerType = InstallerTypeToString(Manifest::InstallerTypeEnum::Portable));
        Commit(PortableValueName::SHA256, SHA256);

        // Move portable exe.
        if (rename)
        {
            PortableTargetFullPath = InstallLocation / commandAlias;
        }
        else
        {
            PortableTargetFullPath = InstallLocation / installerPath.filename();
        }

        AppendExeExtension(PortableTargetFullPath);
        Commit(PortableValueName::PortableTargetFullPath, PortableTargetFullPath);
        Commit(PortableValueName::InstallLocation, InstallLocation);
        MovePortableExe(installerPath);

        
        // Create symlink
        if (!InstallDirectoryAddedToPath)
        {
            PortableSymlinkFullPath = GetPortableLinksLocation(GetScope()) / commandAlias;
            Commit(PortableValueName::PortableSymlinkFullPath, PortableSymlinkFullPath);

            std::filesystem::file_status status = std::filesystem::status(PortableSymlinkFullPath);
            if (std::filesystem::is_directory(status))
            {
                AICLI_LOG(CLI, Info, << "Unable to create symlink. '" << PortableSymlinkFullPath << "points to an existing directory.");
                return APPINSTALLER_CLI_ERROR_PORTABLE_SYMLINK_PATH_IS_DIRECTORY;
            }

            if (std::filesystem::remove(PortableSymlinkFullPath))
            {
                AICLI_LOG(CLI, Info, << "Removed existing file at " << PortableSymlinkFullPath);
                //context.Reporter.Warn() << Resource::String::OverwritingExistingFileAtMessage << ' ' << PortableSymlinkFullPath.u8string() << std::endl;
            }

            CreatePortableSymlink();
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Package directory was previously added to PATH. Skipping symlink creation.");
        }

        // Add to path
        const std::filesystem::path& pathValue = GetPathValue();
        if (PathVariable(GetScope()).Append(pathValue))
        {
            AICLI_LOG(Core, Info, << "Appended target directory to PATH registry: " << pathValue);
            //context.Reporter.Warn() << Resource::String::ModifiedPathRequiresShellRestart << std::endl;
            m_addedToPath = true;
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Target directory already exists in PATH registry: " << pathValue);
        }
    
        return ERROR_SUCCESS;
    }

    void PortableEntry::SetAppsAndFeaturesMetadata(const Manifest::AppsAndFeaturesEntry& entry, const Manifest::Manifest& manifest)
    {
        Commit(PortableValueName::DisplayName, DisplayName = entry.DisplayName);
        Commit(PortableValueName::DisplayVersion, DisplayVersion = entry.DisplayVersion);
        Commit(PortableValueName::Publisher, Publisher = entry.Publisher);
        Commit(PortableValueName::InstallDate, InstallDate = AppInstaller::Utility::GetCurrentDateForARP());
        Commit(PortableValueName::URLInfoAbout, URLInfoAbout = manifest.CurrentLocalization.Get<Manifest::Localization::PackageUrl>());
        Commit(PortableValueName::HelpLink, HelpLink = manifest.CurrentLocalization.Get < Manifest::Localization::PublisherSupportUrl>());
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