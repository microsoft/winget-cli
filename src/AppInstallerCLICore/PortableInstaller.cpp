// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ExecutionContext.h"
#include "PortableInstaller.h"
#include "winget/Manifest.h"
#include "winget/ManifestCommon.h"
#include "winget/Filesystem.h"
#include "winget/PathVariable.h"
#include "Microsoft/PortableIndex.h"
#include "Microsoft/Schema/IPortableIndex.h"
#include <AppInstallerErrors.h>

using namespace AppInstaller::Utility;
using namespace AppInstaller::Registry;
using namespace AppInstaller::Registry::Portable;
using namespace AppInstaller::Registry::Environment;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::SQLite;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Repository::Microsoft::Schema;

namespace AppInstaller::CLI::Portable
{
    std::filesystem::path GetPortableLinksLocation(Manifest::ScopeEnum scope)
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

    std::filesystem::path GetPortableInstallRoot(Manifest::ScopeEnum scope, Utility::Architecture arch)
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

    bool VerifyPortableFile(AppInstaller::Portable::PortableFileEntry& entry)
    {
        std::filesystem::path filePath = entry.GetFilePath();
        PortableFileType fileType = entry.FileType;

        if (fileType == PortableFileType::File)
        {
            if (std::filesystem::exists(filePath))
            {
                SHA256::HashBuffer fileHash = SHA256::ComputeHashFromFile(filePath);
                if (!SHA256::AreEqual(fileHash, SHA256::ConvertToBytes(entry.SHA256)))
                {
                    AICLI_LOG(CLI, Warning, << "File hash does not match ARP Entry. Expected: " << entry.SHA256 << " Actual: " << SHA256::ConvertToString(fileHash));
                    return false;
                }
            }
        }
        else if (fileType == PortableFileType::Symlink)
        {
            if (Filesystem::SymlinkExists(filePath) && !Filesystem::VerifySymlink(filePath, entry.SymlinkTarget))
            {
                AICLI_LOG(CLI, Warning, << "Symlink target does not match ARP Entry. Expected: " << entry.SymlinkTarget << " Actual: " << std::filesystem::read_symlink(filePath));
                return false;
            }
        }

        return true;
    }

    void PortableInstaller::InstallFile(AppInstaller::Portable::PortableFileEntry& entry)
    {
        PortableFileType fileType = entry.FileType;
        std::filesystem::path filePath = entry.GetFilePath();

        if (entry.FileType == PortableFileType::File)
        {
            if (std::filesystem::exists(filePath))
            {
                AICLI_LOG(Core, Info, << "Removing existing portable file at: " << filePath);
                std::filesystem::remove(filePath);
            }

            AICLI_LOG(Core, Info, << "Moving portable exe to: " << filePath);

            if (!RecordToIndex)
            {
                CommitToARPEntry(PortableValueName::PortableTargetFullPath, filePath);
                CommitToARPEntry(PortableValueName::SHA256, entry.SHA256);
            }

            Filesystem::RenameFile(entry.CurrentPath, filePath);
        }
        else if (fileType == PortableFileType::Directory)
        {
            AICLI_LOG(Core, Info, << "Moving directory to: " << filePath);
            Filesystem::RenameFile(entry.CurrentPath, filePath);
        }
        else if (entry.FileType == PortableFileType::Symlink)
        {
            if (!InstallDirectoryAddedToPath)
            {
                std::filesystem::file_status status = std::filesystem::status(filePath);
                if (std::filesystem::is_directory(status))
                {
                    AICLI_LOG(CLI, Info, << "Unable to create symlink. '" << filePath << "points to an existing directory.");
                    THROW_HR(APPINSTALLER_CLI_ERROR_PORTABLE_SYMLINK_PATH_IS_DIRECTORY);
                }

                if (!RecordToIndex)
                {
                    CommitToARPEntry(PortableValueName::PortableSymlinkFullPath, filePath);
                }

                if (std::filesystem::remove(filePath))
                {
                    AICLI_LOG(CLI, Info, << "Removed existing file at " << filePath);
                    m_stream << Resource::String::OverwritingExistingFileAtMessage(Utility::LocIndView{ filePath.u8string() }) << std::endl;
                }

                if (Filesystem::CreateSymlink(entry.SymlinkTarget, filePath))
                {
                    AICLI_LOG(Core, Info, << "Symlink created at: " << filePath);
                }
                else
                {
                    // Symlink creation should only fail if the user executes without admin rights or developer mode.
                    // Resort to adding install directory to PATH directly.
                    AICLI_LOG(Core, Info, << "Portable install executed in user mode. Adding package directory to PATH.");
                    CommitToARPEntry(PortableValueName::InstallDirectoryAddedToPath, InstallDirectoryAddedToPath = true);
                }
            }
            m_stream << Resource::String::PortableAliasAdded << ' ' << filePath.stem() << std::endl;
        }
    }

    void PortableInstaller::RemoveFile(AppInstaller::Portable::PortableFileEntry& entry)
    {
        const auto& filePath = entry.GetFilePath();
        PortableFileType fileType = entry.FileType;

        if (fileType == PortableFileType::File && std::filesystem::exists(filePath))
        {
            AICLI_LOG(CLI, Info, << "Deleting portable exe at: " << filePath);
            std::filesystem::remove(filePath);
        }
        else if (fileType == PortableFileType::Symlink && Filesystem::SymlinkExists(filePath))
        {
            AICLI_LOG(CLI, Info, << "Deleting portable symlink at: " << filePath);
            std::filesystem::remove(filePath);
        }
        else if (fileType == PortableFileType::Directory && std::filesystem::exists(filePath))
        {
            AICLI_LOG(CLI, Info, << "Removing directory at " << filePath);
            std::filesystem::remove_all(filePath);
        }
    }

    // TODO: Optimize by applying the difference between expected and desired state.
    void PortableInstaller::ApplyDesiredState()
    {
        std::filesystem::path existingIndexPath = InstallLocation / GetPortableIndexFileName();
        if (std::filesystem::exists(existingIndexPath))
        {
            bool deleteIndex = false;
            {
                PortableIndex existingIndex = PortableIndex::Open(existingIndexPath.u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite);
                
                for (auto expectedEntry : m_expectedEntries)
                {
                    RemoveFile(expectedEntry);
                    existingIndex.RemovePortableFile(expectedEntry);
                }

                deleteIndex = existingIndex.IsEmpty();
            }

            if (deleteIndex)
            {
                std::filesystem::remove(existingIndexPath);
                AICLI_LOG(CLI, Info, << "Portable index deleted: " << existingIndexPath);
            }
        }
        else
        {
            for (auto expectedEntry : m_expectedEntries)
            {
                RemoveFile(expectedEntry);
            }
        }

        // Check if existing install location differs from the target install location for proper cleanup.
        if (!TargetInstallLocation.empty() && TargetInstallLocation != InstallLocation)
        {
            RemoveInstallDirectory();
        }

        if (RecordToIndex)
        {
            std::filesystem::path targetIndexPath = TargetInstallLocation / GetPortableIndexFileName();
            PortableIndex targetIndex = std::filesystem::exists(targetIndexPath) ?
                PortableIndex::Open(targetIndexPath.u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite) :
                PortableIndex::CreateNew(targetIndexPath.u8string());

            for (auto desiredEntry : m_desiredEntries)
            {
                targetIndex.AddOrUpdatePortableFile(desiredEntry);
                InstallFile(desiredEntry);
            }
        }
        else
        {
            for (auto desiredEntry : m_desiredEntries)
            {
                InstallFile(desiredEntry);
            }
        }
    }

    bool PortableInstaller::VerifyExpectedState()
    {
        for (auto entry : m_expectedEntries)
        {
            if (!VerifyPortableFile(entry))
            {
                AICLI_LOG(CLI, Info, << "Portable file has been modified: " << entry.GetFilePath());
                return false;
            }
        }

        return true;
    }

    void PortableInstaller::Install()
    {
        RegisterARPEntry();

        CreateTargetInstallDirectory();

        ApplyDesiredState();

        AddToPathVariable();
    }

    void PortableInstaller::Uninstall()
    {
        ApplyDesiredState();

        RemoveInstallDirectory();

        RemoveFromPathVariable();

        m_portableARPEntry.Delete();
        AICLI_LOG(CLI, Info, << "PortableARPEntry deleted.");
    }

    void PortableInstaller::CreateTargetInstallDirectory()
    {
        if (std::filesystem::create_directories(TargetInstallLocation))
        {
            AICLI_LOG(Core, Info, << "Created target install directory: " << TargetInstallLocation);
            CommitToARPEntry(PortableValueName::InstallDirectoryCreated, true);
        }

        CommitToARPEntry(PortableValueName::InstallLocation, TargetInstallLocation);
    }

    void PortableInstaller::RemoveInstallDirectory()
    {
        if (std::filesystem::exists(InstallLocation) && InstallDirectoryCreated)
        {
            if (Purge)
            {
                m_stream << Resource::String::PurgeInstallDirectory << std::endl;
                const auto& removedFilesCount = std::filesystem::remove_all(InstallLocation);
                AICLI_LOG(CLI, Info, << "Purged install location directory. Deleted " << removedFilesCount << " files or directories");
            }
            else
            {
                if (std::filesystem::is_empty(InstallLocation))
                {
                    AICLI_LOG(CLI, Info, << "Removing empty install directory: " << InstallLocation);
                    std::filesystem::remove(InstallLocation);
                }
                else
                {
                    AICLI_LOG(CLI, Info, << "Unable to remove install directory as there are remaining files in: " << InstallLocation);
                    m_stream << Resource::String::FilesRemainInInstallDirectory(Utility::LocIndView{ InstallLocation.u8string() }) << std::endl;
                }
            }
        }
    }

    void PortableInstaller::AddToPathVariable()
    {
        const std::filesystem::path& pathValue = InstallDirectoryAddedToPath ? TargetInstallLocation : GetPortableLinksLocation(GetScope());
        if (PathVariable(GetScope()).Append(pathValue))
        {
            AICLI_LOG(Core, Info, << "Appended target directory to PATH registry: " << pathValue);
            m_stream << Resource::String::ModifiedPathRequiresShellRestart << std::endl;
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Target directory already exists in PATH registry: " << pathValue);
        }
    }

    void PortableInstaller::RemoveFromPathVariable()
    {
        const std::filesystem::path& pathValue = InstallDirectoryAddedToPath ? InstallLocation : GetPortableLinksLocation(GetScope());
        if (std::filesystem::exists(pathValue) && !std::filesystem::is_empty(pathValue))
        {
            AICLI_LOG(Core, Info, << "Install directory is not empty: " << pathValue);
        }
        else
        {
            if (PathVariable(GetScope()).Remove(pathValue))
            {
                AICLI_LOG(CLI, Info, << "Removed target directory from PATH registry: " << pathValue);
            }
            else
            {
                AICLI_LOG(CLI, Info, << "Target directory not removed from PATH registry: " << pathValue);
            }
        }
    }

    void PortableInstaller::SetAppsAndFeaturesMetadata(const Manifest::Manifest& manifest, const std::vector<AppInstaller::Manifest::AppsAndFeaturesEntry>& entries)
    {
        AppInstaller::Manifest::AppsAndFeaturesEntry entry;
        if (!entries.empty())
        {
            entry = entries[0];
        }

        if (entry.DisplayName.empty())
        {
            entry.DisplayName = manifest.CurrentLocalization.Get<Manifest::Localization::PackageName>();
        }
        if (entry.DisplayVersion.empty())
        {
            entry.DisplayVersion = manifest.Version;
        }
        if (entry.Publisher.empty())
        {
            entry.Publisher = manifest.CurrentLocalization.Get<Manifest::Localization::Publisher>();
        }

        DisplayName = entry.DisplayName;
        DisplayVersion = entry.DisplayVersion;
        Publisher = entry.Publisher;
        InstallDate = Utility::GetCurrentDateForARP();
        URLInfoAbout = manifest.CurrentLocalization.Get<Manifest::Localization::PackageUrl>();
        HelpLink = manifest.CurrentLocalization.Get<Manifest::Localization::PublisherSupportUrl>();
    }

    void PortableInstaller::SetExpectedState()
    {
        const auto& indexPath = InstallLocation / GetPortableIndexFileName();

        if (std::filesystem::exists(indexPath))
        {
            PortableIndex portableIndex = PortableIndex::Open(indexPath.u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite);
            m_expectedEntries = portableIndex.GetAllPortableFiles();
        }
        else
        {
            std::filesystem::path targetFullPath = PortableTargetFullPath;
            std::filesystem::path symlinkFullPath = PortableSymlinkFullPath;

            if (!symlinkFullPath.empty())
            {
                m_expectedEntries.emplace_back(std::move(PortableFileEntry::CreateSymlinkEntry(symlinkFullPath, targetFullPath)));
            }

            if (!targetFullPath.empty())
            {
                m_expectedEntries.emplace_back(std::move(PortableFileEntry::CreateFileEntry({}, targetFullPath, SHA256)));
            }
        }
    }

    PortableInstaller::PortableInstaller(Manifest::ScopeEnum scope, Utility::Architecture arch, const std::string& productCode) :
        m_portableARPEntry(PortableARPEntry(scope, arch, productCode))
    {
        if (ARPEntryExists())
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
            InstallDirectoryAddedToPath = GetBoolValue(PortableValueName::InstallDirectoryAddedToPath);
            InstallDirectoryCreated = GetBoolValue(PortableValueName::InstallDirectoryCreated);
        }
        
        SetExpectedState();
    }

    void PortableInstaller::RegisterARPEntry()
    {
        CommitToARPEntry(PortableValueName::WinGetPackageIdentifier, WinGetPackageIdentifier);
        CommitToARPEntry(PortableValueName::WinGetSourceIdentifier, WinGetSourceIdentifier);
        CommitToARPEntry(PortableValueName::UninstallString, "winget uninstall --product-code " + GetProductCode());
        CommitToARPEntry(PortableValueName::WinGetInstallerType, InstallerTypeToString(Manifest::InstallerTypeEnum::Portable));
        CommitToARPEntry(PortableValueName::DisplayName, DisplayName);
        CommitToARPEntry(PortableValueName::DisplayVersion, DisplayVersion);
        CommitToARPEntry(PortableValueName::Publisher, Publisher);
        CommitToARPEntry(PortableValueName::InstallDate, InstallDate);
        CommitToARPEntry(PortableValueName::URLInfoAbout, URLInfoAbout);
        CommitToARPEntry(PortableValueName::HelpLink, HelpLink);
    }

    std::string PortableInstaller::GetStringValue(PortableValueName valueName)
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

    std::filesystem::path PortableInstaller::GetPathValue(PortableValueName valueName)
    {
        if (m_portableARPEntry[valueName].has_value())
        {
            return m_portableARPEntry[valueName]->GetValue<Value::Type::UTF16String>();
        }
        {
            return {};
        }
    }

    bool PortableInstaller::GetBoolValue(PortableValueName valueName)
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
