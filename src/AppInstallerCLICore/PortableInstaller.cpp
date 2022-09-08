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
                std::vector<IPortableIndex::PortableFile> portableFiles = portableIndex.GetAllPortableFiles();

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

    bool PortableInstaller::VerifyPortableFilesForUninstall()
    {
        const auto& indexPath = GetPortableIndexPath();
        if (std::filesystem::exists(indexPath))
        {
            PortableIndex portableIndex = PortableIndex::Open(indexPath.u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite);
            std::vector<IPortableIndex::PortableFile> portableFiles = portableIndex.GetAllPortableFiles();
            for (const auto& file : portableFiles)
            {
                IPortableIndex::PortableFileType fileType = file.FileType;
                const auto& filePath = file.GetFilePath();

                if (fileType == IPortableIndex::PortableFileType::File)
                {
                    if (std::filesystem::exists(filePath) && !SHA256::AreEqual(SHA256::ComputeHashFromFile(filePath), SHA256::ConvertToBytes(file.SHA256)))
                    {
                        return false;
                    }
                }
                else if (fileType == IPortableIndex::PortableFileType::Symlink)
                {
                    if (Filesystem::SymlinkExists(filePath) && !Filesystem::VerifySymlink(filePath, file.SymlinkTarget))
                    {
                        return false;
                    }
                }
            }

            return true;
        }
        else
        {
            if (std::filesystem::exists(PortableTargetFullPath) && !SHA256::AreEqual(SHA256::ComputeHashFromFile(PortableTargetFullPath), SHA256::ConvertToBytes(SHA256)))
            {
                return false;
            }
            else if (Filesystem::SymlinkExists(PortableSymlinkFullPath) && !Filesystem::VerifySymlink(PortableSymlinkFullPath, PortableTargetFullPath))
            {
                return false;
            }
            else
            {
                return true;
            }
        }
    }

    HRESULT PortableInstaller::InstallSingle(const std::filesystem::path& installerPath)
    {
        InitializeRegistryEntry();

        MovePortableExe(installerPath);

        if (!InstallDirectoryAddedToPath)
        {
            const std::filesystem::path& symlinkPath = PortableSymlinkFullPath;
            CommitToARPEntry(PortableValueName::PortableSymlinkFullPath, symlinkPath);

            std::filesystem::file_status status = std::filesystem::status(symlinkPath);
            if (std::filesystem::is_directory(status))
            {
                AICLI_LOG(CLI, Info, << "Unable to create symlink. '" << symlinkPath << "points to an existing directory.");
                return APPINSTALLER_CLI_ERROR_PORTABLE_SYMLINK_PATH_IS_DIRECTORY;
            }

            CreatePortableSymlink(PortableTargetFullPath, PortableSymlinkFullPath);
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Package directory was previously added to PATH. Skipping symlink creation.");
        }

        AddToPathVariable();

        FinalizeRegistryEntry();

        return ERROR_SUCCESS;
    }

    HRESULT PortableInstaller::InstallMultiple(const std::vector<Manifest::NestedInstallerFile>& nestedInstallerFiles, const std::vector<std::filesystem::path>& extractedItems)
    {
        InitializeRegistryEntry();

        const auto& indexPath = InstallLocation / c_PortableIndexFileName;
        if (!std::filesystem::exists(indexPath))
        {
            PortableIndex::CreateNew(indexPath.u8string(), Schema::Version::Latest());
        }

        PortableIndex portableIndex = PortableIndex::Open(indexPath.u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite);
        for (auto& item : extractedItems)
        {
            const auto& itemPath = InstallLocation / item;
            IPortableIndex::PortableFile portableFile = PortableIndex::CreatePortableFileFromPath(itemPath);
            portableIndex.AddOrUpdatePortableFile(portableFile);
        }

        for (const auto& nestedInstallerFile : nestedInstallerFiles)
        {
            const std::filesystem::path& portableTargetPath = InstallLocation / ConvertToUTF16(nestedInstallerFile.RelativeFilePath);
            if (!InstallDirectoryAddedToPath)
            {
                std::filesystem::path commandAlias;
                if (!nestedInstallerFile.PortableCommandAlias.empty())
                {
                    commandAlias = ConvertToUTF16(nestedInstallerFile.PortableCommandAlias);
                }
                else
                {
                    commandAlias = portableTargetPath.filename();
                }

                Filesystem::AppendExtension(commandAlias, ".exe");
                const std::filesystem::path& symlinkFullPath = GetPortableLinksLocation(GetScope()) / commandAlias;
                if (CreatePortableSymlink(portableTargetPath, symlinkFullPath))
                {
                    IPortableIndex::PortableFile symlinkPortableFile = PortableIndex::CreatePortableFileFromPath(symlinkFullPath);
                    portableIndex.AddOrUpdatePortableFile(symlinkPortableFile);
                }
            }
            else
            {
                AICLI_LOG(CLI, Info, << "Package directory was previously added to PATH. Skipping symlink creation.");
                break;
            }
        }

        AddToPathVariable();
        
        FinalizeRegistryEntry();

        return ERROR_SUCCESS;
    }

    HRESULT PortableInstaller::UninstallSingle(bool purge)
    {
        if (std::filesystem::exists(PortableTargetFullPath))
        {
            AICLI_LOG(CLI, Info, << "Successfully deleted portable exe:" << PortableTargetFullPath);
            std::filesystem::remove(PortableTargetFullPath);
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Portable exe not found; Unable to delete portable exe: " << PortableTargetFullPath);
        }

        RemoveInstallDirectory(purge);
         
        if (Filesystem::SymlinkExists(PortableSymlinkFullPath) && !std::filesystem::remove(PortableSymlinkFullPath))
        {
            AICLI_LOG(CLI, Info, << "Portable symlink not found; Unable to delete portable symlink: " << PortableSymlinkFullPath);
        }

        RemoveFromPathVariable();

        m_portableARPEntry.Delete();
        AICLI_LOG(CLI, Info, << "PortableARPEntry deleted.");
        
        return ERROR_SUCCESS;
    }

    HRESULT PortableInstaller::UninstallFromIndex(bool purge)
    {
        const auto& indexPath = GetPortableIndexPath();
        RemoveItemsFromIndex(indexPath);

        RemoveInstallDirectory(purge);

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