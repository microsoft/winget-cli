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
            // Record all extracted items and create database if not created.
        }

        for (const auto& nestedInstallerFile : nestedInstallerFiles)
        {
            const std::filesystem::path& portableTargetPath = InstallLocation / ConvertToUTF16(nestedInstallerFile.RelativeFilePath);

            if (!InstallDirectoryAddedToPath)
            {
                const std::filesystem::path& symlinkFullPath = GetPortableLinksLocation(GetScope()) / ConvertToUTF16(nestedInstallerFile.PortableCommandAlias);
                CreatePortableSymlink(portableTargetPath, symlinkFullPath);
                // Record symlink file
            }
            else
            {
                AICLI_LOG(CLI, Info, << "Package directory was previously added to PATH. Skipping symlink creation.");
            }
        }

        AddToPathVariable();

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

        // Move portable exe
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

            CreatePortableSymlink(PortableTargetFullPath, PortableSymlinkFullPath);
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Package directory was previously added to PATH. Skipping symlink creation.");
        }

        AddToPathVariable();
        return ERROR_SUCCESS;
    }

    HRESULT PortableEntry::Uninstall(bool purge)
    {
        // remove portable
        RemovePortableExe(PortableTargetFullPath, SHA256);

        // remove install directory
        RemovePortableDirectory(InstallLocation, purge, InstallDirectoryCreated);
         
        // remove portable symlink
        RemovePortableSymlink(PortableTargetFullPath, PortableSymlinkFullPath);

        // remove path
        const std::filesystem::path& pathValue = GetPathDirectory();
        if (RemoveFromPathVariable())
        {
            AICLI_LOG(CLI, Info, << "Removed target directory from PATH registry: " << pathValue);
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Target directory not removed from PATH registry: " << pathValue);
        }

        m_portableARPEntry.Delete();
        AICLI_LOG(CLI, Info, << "PortableARPEntry deleted.");
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

    bool PortableEntry::VerifyPortableExeHash(const std::filesystem::path& targetPath, const std::string& hashValue)
    {
        std::ifstream inStream{ targetPath, std::ifstream::binary };
        const Utility::SHA256::HashBuffer& targetFileHash = Utility::SHA256::ComputeHash(inStream);
        inStream.close();

        return Utility::SHA256::AreEqual(Utility::SHA256::ConvertToBytes(hashValue), targetFileHash);
    }

    void PortableEntry::AddToPathVariable()
    {
        const std::filesystem::path& pathValue = GetPathDirectory();
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
    }

    void PortableEntry::CreatePortableSymlink(const std::filesystem::path& targetPath, const std::filesystem::path& symlinkPath)
    {
        std::filesystem::file_status status = std::filesystem::status(symlinkPath);
        if (std::filesystem::is_directory(status))
        {
            AICLI_LOG(CLI, Info, << "Unable to create symlink. '" << symlinkPath << "points to an existing directory.");
            throw APPINSTALLER_CLI_ERROR_PORTABLE_SYMLINK_PATH_IS_DIRECTORY;
        }

        if (std::filesystem::remove(symlinkPath))
        {
            AICLI_LOG(CLI, Info, << "Removed existing file at " << symlinkPath);
            //m_stream << Resource::String::OverwritingExistingFileAtMessage << ' ' << symlinkPath.u8string() << std::endl;
        }

        if (Filesystem::CreateSymlink(targetPath, symlinkPath))
        {
            AICLI_LOG(Core, Info, << "Symlink created at: " << symlinkPath);
        }
        else
        {
            // Symlink creation should only fail if the user executes in user mode and non-admin.
            // Resort to adding install directory to PATH directly.
            AICLI_LOG(Core, Info, << "Portable install executed in user mode. Adding package directory to PATH.");
            Commit(PortableValueName::InstallDirectoryAddedToPath, InstallDirectoryAddedToPath = true);
        }
    }

    bool PortableEntry::VerifySymlinkTarget(const std::filesystem::path& targetPath, const std::filesystem::path& symlinkPath)
    {
        AICLI_LOG(Core, Info, << "Expected portable target path: " << targetPath);
        const std::filesystem::path& symlinkTargetPath = std::filesystem::read_symlink(symlinkPath);
        
        if (symlinkTargetPath == targetPath)
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

    void PortableEntry::RemovePortableExe(const std::filesystem::path& targetPath, const std::string& hash)
    {
        if (std::filesystem::exists(targetPath))
        {
            if (!VerifyPortableExeHash(targetPath, hash))
            {
                //bool overrideHashMismatch = context.Args.Contains(Execution::Args::Type::HashOverride);
                bool overrideHashMismatch = true;
                if (overrideHashMismatch)
                {
                    //context.Reporter.Warn() << Resource::String::PortableHashMismatchOverridden << std::endl;
                }
                else
                {
                    //context.Reporter.Warn() << Resource::String::PortableHashMismatchOverrideRequired << std::endl;
                    throw APPINSTALLER_CLI_ERROR_PORTABLE_UNINSTALL_FAILED;
                }
            }

            std::filesystem::remove(targetPath);
            AICLI_LOG(CLI, Info, << "Successfully deleted portable exe:" << targetPath);
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Portable exe not found; Unable to delete portable exe: " << targetPath);
        }
    }

    void PortableEntry::RemovePortableSymlink(const std::filesystem::path& targetPath, const std::filesystem::path& symlinkPath)
    {
        if (!std::filesystem::is_symlink(std::filesystem::symlink_status(symlinkPath)))
        {
            AICLI_LOG(Core, Info, << "The registry value for [PortableSymlinkFullPath] does not point to a valid symlink file.");
        }
        else
        {
            if (VerifySymlinkTarget(targetPath, symlinkPath))
            {
                if (!std::filesystem::remove(symlinkPath))
                {
                    AICLI_LOG(CLI, Info, << "Portable symlink not found; Unable to delete portable symlink: " << symlinkPath);
                }
            }
            else
            {
                //context.Reporter.Warn() << Resource::String::SymlinkModified << std::endl;
            }
        }
    }

    void PortableEntry::RemovePortableDirectory(const std::filesystem::path& directoryPath, bool purge, bool isCreated)
    {
        if (std::filesystem::exists(directoryPath))
        {
            if (purge)
            {
                if (isCreated)
                {
                    //context.Reporter.Warn() << Resource::String::PurgeInstallDirectory << std::endl;
                    const auto& removedFilesCount = std::filesystem::remove_all(directoryPath);
                    AICLI_LOG(CLI, Info, << "Purged install location directory. Deleted " << removedFilesCount << " files or directories");
                }
                else
                {
                    //context.Reporter.Warn() << Resource::String::UnableToPurgeInstallDirectory << std::endl;
                }

            }
            else if (std::filesystem::is_empty(directoryPath))
            {
                if (isCreated)
                {
                    std::filesystem::remove(directoryPath);
                    AICLI_LOG(CLI, Info, << "Install directory deleted: " << directoryPath);
                }
            }
            else
            {
                //context.Reporter.Warn() << Resource::String::FilesRemainInInstallDirectory << installDirectory << std::endl;
            }
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Install directory does not exist: " << directoryPath);
        }
    }

    bool PortableEntry::RemoveFromPathVariable()
    {
        bool removeFromPath = true;
        std::filesystem::path pathValue = GetPathDirectory();
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