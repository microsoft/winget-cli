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
            if (entry.IsInstallDirectory)
            {
                CommitToARPEntry(PortableValueName::InstallLocation, filePath);

                if (std::filesystem::create_directories(filePath))
                {
                    AICLI_LOG(Core, Info, << "Created target install directory: " << filePath);
                    CommitToARPEntry(PortableValueName::InstallDirectoryCreated, true);
                }
            }
            else
            {
                AICLI_LOG(Core, Info, << "Moving directory to: " << filePath);
                Filesystem::RenameFile(entry.CurrentPath, filePath);
            }
        }
        else if (entry.FileType == PortableFileType::Symlink)
        {
            const std::filesystem::path& symlinkPath = entry.SymlinkTarget;

            std::filesystem::file_status status = std::filesystem::status(symlinkPath);
            if (std::filesystem::is_directory(status))
            {
                AICLI_LOG(CLI, Info, << "Unable to create symlink. '" << symlinkPath << "points to an existing directory.");
                throw APPINSTALLER_CLI_ERROR_PORTABLE_SYMLINK_PATH_IS_DIRECTORY;
            }

            if (!RecordToIndex)
            {
                CommitToARPEntry(PortableValueName::PortableSymlinkFullPath, filePath);
            }

            PortableInstaller::CreatePortableSymlink(entry.SymlinkTarget, filePath);
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
            if (entry.IsInstallDirectory)
            {
                if (Purge)
                {
                    m_stream << Resource::String::PurgeInstallDirectory << std::endl;
                    const auto& removedFilesCount = std::filesystem::remove_all(filePath);
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
                            break;
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
                AICLI_LOG(CLI, Info, << "Removing directory at " << filePath);
                std::filesystem::remove_all(filePath);
            }
        }
    }

    void PortableInstaller::ApplyDesiredState()
    {
        // TODO: Record to index if applicable.

        for (auto expectedEntry : m_expectedEntries)
        {
            RemoveFile(expectedEntry);
        }

        for (auto desiredEntry : m_desiredEntries)
        {
            InstallFile(desiredEntry);
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

    HRESULT PortableInstaller::Install()
    {
        InitializeRegistryEntry();
        
        ApplyDesiredState();

        AddToPathVariable();

        return ERROR_SUCCESS;
    }

    HRESULT PortableInstaller::Uninstall()
    {
        ApplyDesiredState();

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