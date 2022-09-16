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
    namespace
    {
        constexpr std::wstring_view c_PortableIndexFileName = L"portable.db";

        void RemoveItemsFromIndex(const std::filesystem::path& indexPath)
        {
            bool deleteIndex = false;
            {
                PortableIndex portableIndex = PortableIndex::Open(indexPath.u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite);
                std::vector<AppInstaller::Portable::PortableFileEntry> portableFiles = portableIndex.GetAllPortableFiles();

                for (const auto& file : portableFiles)
                {
                    const auto& filePath = file.GetFilePath();

                    if (std::filesystem::exists(filePath) || Filesystem::SymlinkExists(filePath))
                    {
                        std::cout << filePath << std::endl;
                        std::filesystem::remove_all(filePath);
                    }

                    portableIndex.RemovePortableFile(file);
                }

                if (portableIndex.IsEmpty())
                {
                    deleteIndex = true;
                };
            }

            if (deleteIndex)
            {
                AICLI_LOG(CLI, Info, << "Removing portable index: " << indexPath);
                std::filesystem::remove(indexPath);
            }
        }
    }

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
            if (std::filesystem::exists(filePath) && !SHA256::AreEqual(SHA256::ComputeHashFromFile(filePath), SHA256::ConvertToBytes(entry.SHA256)))
            {
                return false;
            }
        }
        else if (fileType == PortableFileType::Symlink)
        {
            if (Filesystem::SymlinkExists(filePath) && !Filesystem::VerifySymlink(filePath, entry.SymlinkTarget))
            {
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
                AICLI_LOG(Core, Info, << "Removing existing portable exe at: " << filePath);
                std::filesystem::remove(filePath);
            }

            AICLI_LOG(Core, Info, << "Portable exe moved to: " << filePath);

            if (!RecordToIndex)
            {
                CommitToARPEntry(PortableValueName::PortableTargetFullPath, filePath);
                CommitToARPEntry(PortableValueName::SHA256, entry.SHA256);
            }

            Filesystem::RenameFile(entry.CurrentPath, filePath);
        }
        else if (fileType == PortableFileType::Directory)
        {
            if (!RecordToIndex)
            {
                CommitToARPEntry(PortableValueName::InstallLocation, filePath);
            }

            if (std::filesystem::create_directories(filePath))
            {
                AICLI_LOG(Core, Info, << "Created target install directory: " << filePath);
            }
        }
        else if (entry.FileType == PortableFileType::Symlink)
        {
            const std::filesystem::path& symlinkPath = entry.SymlinkTarget;

            std::filesystem::file_status status = std::filesystem::status(symlinkPath);
            if (std::filesystem::is_directory(status))
            {
                AICLI_LOG(CLI, Info, << "Unable to create symlink. '" << symlinkPath << "points to an existing directory.");
                //return APPINSTALLER_CLI_ERROR_PORTABLE_SYMLINK_PATH_IS_DIRECTORY;
            }

            if (!RecordToIndex)
            {
                CommitToARPEntry(PortableValueName::PortableSymlinkFullPath, filePath);
            }

            PortableInstaller::CreatePortableSymlink(entry.SymlinkTarget, filePath);
        }

        if (RecordToIndex)
        {
            std::filesystem::path indexPath = GetPortableIndexPath();
            if (std::filesystem::exists(indexPath))
            {
                AppInstaller::Repository::Microsoft::PortableIndex portableIndex = AppInstaller::Repository::Microsoft::PortableIndex::CreateNew(indexPath.u8string());
                portableIndex.AddOrUpdatePortableFile(entry);
            }
            else
            {
                AppInstaller::Repository::Microsoft::PortableIndex portableIndex = AppInstaller::Repository::Microsoft::PortableIndex::Open(indexPath.u8string(), AppInstaller::Repository::Microsoft::SQLiteStorageBase::OpenDisposition::ReadWrite);
                portableIndex.AddOrUpdatePortableFile(entry);
            }
        }
    }

    void PortableInstaller::RemoveFile(AppInstaller::Portable::PortableFileEntry& desiredState)
    {
        const auto& filePath = desiredState.GetFilePath();

        if (std::filesystem::exists(filePath) || Filesystem::SymlinkExists(filePath))
        {
            if (desiredState.FileType == PortableFileType::Directory && filePath == InstallLocation)
            {
                if (Purge)
                {
                    m_stream << Resource::String::PurgeInstallDirectory << std::endl;
                    const auto& removedFilesCount = std::filesystem::remove_all(InstallLocation);
                    AICLI_LOG(CLI, Info, << "Purged install location directory. Deleted " << removedFilesCount << " files or directories");
                }
                else
                {
                    std::filesystem::path indexPath = GetPortableIndexPath();
                    bool isDirectoryEmpty = true;
                    for (const auto& item : std::filesystem::directory_iterator(filePath))
                    {
                        if (item.path() != indexPath)
                        {
                            isDirectoryEmpty = false;
                        }
                    }

                    if (isDirectoryEmpty)
                    {
                        AICLI_LOG(CLI, Info, << "Removing empty install directory: " << filePath);
                        std::filesystem::remove(filePath);
                    }
                    else
                    {
                        AICLI_LOG(CLI, Info, << "Unable to remove install directory as there are remaining files in: " << filePath);
                        m_stream << Resource::String::FilesRemainInInstallDirectory << filePath << std::endl;
                    }
                }
            }
            else
            {
                std::filesystem::remove_all(filePath);

                if (RecordToIndex)
                {
                    std::filesystem::path indexPath = GetPortableIndexPath();
                    PortableIndex index = PortableIndex::Open(indexPath.u8string(), AppInstaller::Repository::Microsoft::SQLiteStorageBase::OpenDisposition::Read);
                    index.RemovePortableFile(desiredState);
;                }
            }
        }
    }

    void PortableInstaller::ResolutionEngine(std::vector<AppInstaller::Portable::PortableFileEntry>& desiredState, std::vector<AppInstaller::Portable::PortableFileEntry>& expectedState)
    {
        // Remove all entries from expected state first, then perform install. If this is an uninstall, desiredState will be empty
        for (auto entry : expectedState)
        {
            RemoveFile(entry);
        }

        for (auto entry : desiredState)
        {
            // for each one, install and record
            InstallFile(entry);
        }
    }

    bool PortableInstaller::VerifyResolution(std::vector<AppInstaller::Portable::PortableFileEntry>& expectedState)
    {
        for (auto entry : expectedState)
        {
            if (!VerifyPortableFile(entry))
            {
                AICLI_LOG(CLI, Info, << "Portable file has been modified: " << entry.GetFilePath());
                return false;
            }
        }

        return true;
    }

    // get desired state and expected state from context
    // 

    std::vector<AppInstaller::Portable::PortableFileEntry> PortableInstaller::GetDesiredState(std::vector<std::filesystem::path> files)
    {
        std::vector<AppInstaller::Portable::PortableFileEntry> entries;

        for (const auto file : files)
        {
            AppInstaller::Portable::PortableFileEntry portableFile;

            if (std::filesystem::is_directory(file))
            {
                portableFile.SetFilePath(file);
                portableFile.FileType = PortableFileType::Directory;
            }
            else
            {
                std::filesystem::path relativePath = std::filesystem::relative(file, file.parent_path());
                portableFile.CurrentPath = file;
                portableFile.SetFilePath(InstallLocation / relativePath);
                portableFile.FileType = PortableFileType::File;
                portableFile.SHA256 = Utility::SHA256::ConvertToString(Utility::SHA256::ComputeHashFromFile(file));
            }

            entries.emplace_back(std::move(portableFile));

            if (portableFile.FileType == PortableFileType::File)
            {
                AppInstaller::Portable::PortableFileEntry symlinkEntry;
                symlinkEntry.SetFilePath(PortableSymlinkFullPath);
                symlinkEntry.SymlinkTarget = PortableTargetFullPath.u8string();
                symlinkEntry.FileType = PortableFileType::Symlink;
                entries.emplace_back(symlinkEntry);
            }
        }

        return entries;
    }

    std::vector<AppInstaller::Portable::PortableFileEntry> PortableInstaller::GetExpectedState()
    {
        std::vector<AppInstaller::Portable::PortableFileEntry> entries;
        const auto& indexPath = GetPortableIndexPath();
        if (std::filesystem::exists(indexPath))
        {
            PortableIndex portableIndex = PortableIndex::Open(indexPath.u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite);
            entries = portableIndex.GetAllPortableFiles();
        }
        else
        {
            if (!PortableSymlinkFullPath.empty())
            {
                AppInstaller::Portable::PortableFileEntry symlinkEntry;
                symlinkEntry.SetFilePath(PortableSymlinkFullPath);
                symlinkEntry.SymlinkTarget = PortableTargetFullPath.u8string();
                symlinkEntry.FileType = PortableFileType::Symlink;
                entries.emplace_back(symlinkEntry);
            }

            if (!PortableTargetFullPath.empty())
            {
                AppInstaller::Portable::PortableFileEntry exeEntry;
                exeEntry.SetFilePath(PortableTargetFullPath);
                exeEntry.SHA256 = SHA256;
                exeEntry.FileType = PortableFileType::File;
                entries.emplace_back(exeEntry);
            }

            if (!InstallLocation.empty() && InstallDirectoryCreated)
            {
                AppInstaller::Portable::PortableFileEntry directoryEntry;
                directoryEntry.SetFilePath(InstallLocation);
                directoryEntry.FileType = PortableFileType::Directory;
                entries.emplace_back(directoryEntry);
            }
        }

        return entries;
    }

    HRESULT PortableInstaller::Install(const std::filesystem::path& installerPath)
    {
        InitializeRegistryEntry();

        std::vector<std::filesystem::path> installFiles;
        if (std::filesystem::is_directory(installerPath))
        {
            for (const auto& entry : std::filesystem::directory_iterator(installerPath))
            {
                installFiles.emplace_back(entry.path());
            }
        }
        else
        {
            installFiles.emplace_back(installerPath);
        }


        std::vector<AppInstaller::Portable::PortableFileEntry> desiredEntries = GetDesiredState(installFiles);
        std::vector<AppInstaller::Portable::PortableFileEntry> expectedEntries = GetExpectedState();

        if (VerifyResolution(expectedEntries))
        {
            // Return that files have been modified
        }

        ResolutionEngine(desiredEntries, expectedEntries);

        AddToPathVariable();

        FinalizeRegistryEntry();

        return ERROR_SUCCESS;
    }

    HRESULT PortableInstaller::Uninstall()
    {
        std::vector<AppInstaller::Portable::PortableFileEntry> desiredEntries;
        std::vector<AppInstaller::Portable::PortableFileEntry> expectedEntries = GetExpectedState();

        if (VerifyResolution(expectedEntries))
        {
            // Return that files have been modified
        }

        ResolutionEngine(desiredEntries, expectedEntries);

        RemoveFromPathVariable();

        m_portableARPEntry.Delete();
        AICLI_LOG(CLI, Info, << "PortableARPEntry deleted.");
        
        return ERROR_SUCCESS;
    }

    void PortableInstaller::MovePortableExe(const std::filesystem::path& installerPath)
    {
        bool isDirectoryCreated = false;

        if (std::filesystem::create_directories(InstallLocation))
        {
            AICLI_LOG(Core, Info, << "Created target install directory: " << InstallLocation);
            isDirectoryCreated = true;
        }
        
        if (std::filesystem::exists(PortableTargetFullPath))
        {
            AICLI_LOG(Core, Info, << "Removing existing portable exe at: " << PortableTargetFullPath);
            std::filesystem::remove(PortableTargetFullPath);
        }

        AICLI_LOG(Core, Info, << "Portable exe moved to: " << PortableTargetFullPath);

        CommitToARPEntry(PortableValueName::PortableTargetFullPath, PortableTargetFullPath);

        Filesystem::RenameFile(installerPath, PortableTargetFullPath);
    }

    bool PortableInstaller::CreatePortableSymlink(const std::filesystem::path& targetPath, const std::filesystem::path& symlinkPath)
    {
        if (std::filesystem::remove(symlinkPath))
        {
            AICLI_LOG(CLI, Info, << "Removed existing file at " << symlinkPath);
            m_stream << Resource::String::OverwritingExistingFileAtMessage << ' ' << symlinkPath.u8string() << std::endl;
        }

        if (Filesystem::CreateSymlink(targetPath, symlinkPath))
        {
            AICLI_LOG(Core, Info, << "Symlink created at: " << symlinkPath);
            return true;
        }
        else
        {
            // Symlink creation should only fail if the user executes without admin rights or developer mode.
            // Resort to adding install directory to PATH directly.
            AICLI_LOG(Core, Info, << "Portable install executed in user mode. Adding package directory to PATH.");
            CommitToARPEntry(PortableValueName::InstallDirectoryAddedToPath, InstallDirectoryAddedToPath = true);
            return false;
        }
    }

    void PortableInstaller::RemoveInstallDirectory(bool purge)
    {
        if (std::filesystem::exists(InstallLocation))
        {
            if (purge)
            {
                m_stream << Resource::String::PurgeInstallDirectory << std::endl;
                const auto& removedFilesCount = std::filesystem::remove_all(InstallLocation);
                AICLI_LOG(CLI, Info, << "Purged install location directory. Deleted " << removedFilesCount << " files or directories");
            }
            else if (std::filesystem::is_empty(InstallLocation))
            {
                AICLI_LOG(CLI, Info, << "Removing empty install directory: " << InstallLocation);
                std::filesystem::remove(InstallLocation);
            }
            else
            {
                AICLI_LOG(CLI, Info, << "Unable to remove install directory as there are remaining files in: " << InstallLocation);
                m_stream << Resource::String::FilesRemainInInstallDirectory << InstallLocation << std::endl;
            }
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Install directory does not exist: " << InstallLocation);
        }
    }

    void PortableInstaller::AddToPathVariable()
    {
        const std::filesystem::path& pathValue = GetInstallDirectoryForPathVariable();
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
        std::filesystem::path pathValue = GetInstallDirectoryForPathVariable();
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
            InstallLocation = GetPathValue(PortableValueName::InstallLocation);
            InstallDirectoryAddedToPath = GetBoolValue(PortableValueName::InstallDirectoryAddedToPath);
            InstallDirectoryCreated = GetBoolValue(PortableValueName::InstallDirectoryCreated);
        }
    }

    void PortableInstaller::InitializeRegistryEntry()
    {
        CommitToARPEntry(PortableValueName::WinGetPackageIdentifier, WinGetPackageIdentifier);
        CommitToARPEntry(PortableValueName::WinGetSourceIdentifier, WinGetSourceIdentifier);
        CommitToARPEntry(PortableValueName::UninstallString, "winget uninstall --product-code " + GetProductCode());
        CommitToARPEntry(PortableValueName::WinGetInstallerType, InstallerTypeToString(Manifest::InstallerTypeEnum::Portable));
        CommitToARPEntry(PortableValueName::SHA256, SHA256);
        CommitToARPEntry(PortableValueName::InstallLocation, InstallLocation);
    }

    void PortableInstaller::FinalizeRegistryEntry()
    {
        CommitToARPEntry(PortableValueName::DisplayName, DisplayName);
        CommitToARPEntry(PortableValueName::DisplayVersion, DisplayVersion);
        CommitToARPEntry(PortableValueName::Publisher, Publisher);
        CommitToARPEntry(PortableValueName::InstallDate, InstallDate);
        CommitToARPEntry(PortableValueName::URLInfoAbout, URLInfoAbout);
        CommitToARPEntry(PortableValueName::HelpLink, HelpLink);
    }

    std::filesystem::path PortableInstaller::GetPortableIndexPath()
    {
        return InstallLocation / c_PortableIndexFileName;
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