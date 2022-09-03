// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PortableInstaller.h"
#include "winget/Manifest.h"
#include "winget/ManifestCommon.h"
#include "winget/Filesystem.h"
#include "winget/PathVariable.h"
#include "Microsoft/PortableIndex.h"
#include "Microsoft/Schema/IPortableIndex.h"
#include <AppInstallerErrors.h>
#include <AppInstallerDateTime.h>
#include "ExecutionContext.h"

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

        IPortableIndex::PortableFile CreatePortableFileFromPath(const std::filesystem::path& path)
        {
            IPortableIndex::PortableFile portableFile;
            portableFile.SetFilePath(path);

            if (std::filesystem::is_directory(path))
            {
                portableFile.FileType = IPortableIndex::PortableFileType::Directory;
            }
            else if (std::filesystem::is_symlink(path))
            {
                portableFile.FileType = IPortableIndex::PortableFileType::Symlink;
                portableFile.SymlinkTarget = std::filesystem::read_symlink(path).u8string();
            }
            else
            {
                portableFile.FileType = IPortableIndex::PortableFileType::File;

                std::ifstream inStream{ path, std::ifstream::binary };
                const Utility::SHA256::HashBuffer& targetFileHash = Utility::SHA256::ComputeHash(inStream);
                inStream.close();
                portableFile.SHA256 = Utility::SHA256::ConvertToString(targetFileHash);
            }

            return portableFile;
        }

        bool VerifyPortableExeHash(const std::filesystem::path& targetPath, const std::string& hashValue)
        {
            std::ifstream inStream{ targetPath, std::ifstream::binary };
            const Utility::SHA256::HashBuffer& targetFileHash = Utility::SHA256::ComputeHash(inStream);
            inStream.close();

            return Utility::SHA256::AreEqual(Utility::SHA256::ConvertToBytes(hashValue), targetFileHash);
        }

        void RemoveItemsFromIndex(const std::filesystem::path& indexPath, bool purge)
        {
            bool deleteIndex = false;
            {
                PortableIndex portableIndex = PortableIndex::Open(indexPath.u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite);
                std::vector<IPortableIndex::PortableFile> portableFiles = portableIndex.GetAllPortableFiles();

                for (const auto& file : portableFiles)
                {
                    IPortableIndex::PortableFileType fileType = file.FileType;
                    const auto& filePath = file.GetFilePath();

                    if (purge || (fileType == IPortableIndex::PortableFileType::File && VerifyPortableExeHash(filePath, file.SHA256)) ||
                        (fileType == IPortableIndex::PortableFileType::Symlink && Filesystem::VerifySymlink(filePath, file.SymlinkTarget)))
                    {
                        std::filesystem::remove_all(filePath);
                        portableIndex.RemovePortableFile(file);
                    }
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
        namespace fs = std::filesystem;
        const auto& indexPath = GetPortableIndexPath();
        if (fs:: exists(indexPath))
        {
            PortableIndex portableIndex = PortableIndex::Open(indexPath.u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite);
            std::vector<IPortableIndex::PortableFile> portableFiles = portableIndex.GetAllPortableFiles();
            for (const auto& file : portableFiles)
            {
                IPortableIndex::PortableFileType fileType = file.FileType;
                const auto& filePath = file.GetFilePath();

                if (fileType == IPortableIndex::PortableFileType::File)
                {
                    if (std::filesystem::exists(filePath) && !VerifyPortableExeHash(filePath, file.SHA256))
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
            if (std::filesystem::exists(PortableTargetFullPath) && !VerifyPortableExeHash(PortableTargetFullPath, SHA256))
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

    HRESULT PortableInstaller::SingleInstall(const std::filesystem::path& installerPath)
    {
        InitializeRegistryEntry();

        MovePortableExe(installerPath);

        if (!InstallDirectoryAddedToPath)
        {
            const std::filesystem::path& symlinkPath = PortableSymlinkFullPath;
            Commit(PortableValueName::PortableSymlinkFullPath, symlinkPath);
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

    HRESULT PortableInstaller::MultipleInstall(const std::vector<Manifest::NestedInstallerFile>& nestedInstallerFiles, const std::vector<std::filesystem::path>& extractedItems)
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
            IPortableIndex::PortableFile portableFile = CreatePortableFileFromPath(itemPath);
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
                    IPortableIndex::PortableFile symlinkPortableFile = CreatePortableFileFromPath(symlinkFullPath);
                    portableIndex.AddOrUpdatePortableFile(symlinkPortableFile);
                }
            }
            else
            {
                AICLI_LOG(CLI, Info, << "Package directory was previously added to PATH. Skipping symlink creation.");
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
            std::filesystem::remove(PortableTargetFullPath);
            AICLI_LOG(CLI, Info, << "Successfully deleted portable exe:" << PortableTargetFullPath);
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Portable exe not found; Unable to delete portable exe: " << PortableTargetFullPath);
        }

        RemoveInstallDirectory(purge);
         
        if (!std::filesystem::remove(PortableSymlinkFullPath))
        {
            AICLI_LOG(CLI, Info, << "Portable symlink not found; Unable to delete portable symlink: " << PortableSymlinkFullPath);
        }

        RemoveFromPathVariable();

        AICLI_LOG(CLI, Info, << "PortableARPEntry deleted.");
        m_portableARPEntry.Delete();
        
        return ERROR_SUCCESS;
    }

    HRESULT PortableInstaller::UninstallFromIndex(bool purge)
    {
        const auto& indexPath = GetPortableIndexPath();
        RemoveItemsFromIndex(indexPath, purge);

        RemoveInstallDirectory(purge);

        RemoveFromPathVariable();

        AICLI_LOG(CLI, Info, << "PortableARPEntry deleted.");
        m_portableARPEntry.Delete();

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

        Commit(PortableValueName::PortableTargetFullPath, PortableTargetFullPath);

        Filesystem::RenameFile(installerPath, PortableTargetFullPath);
    }

    bool PortableInstaller::CreatePortableSymlink(const std::filesystem::path& targetPath, const std::filesystem::path& symlinkPath)
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
            Commit(PortableValueName::InstallDirectoryAddedToPath, InstallDirectoryAddedToPath = true);
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
            InstallDirectoryAddedToPath = GetBoolValue(PortableValueName::InstallDirectoryAddedToPath);
        }
    }

    void PortableInstaller::InitializeRegistryEntry()
    {
        Commit(PortableValueName::WinGetPackageIdentifier, WinGetPackageIdentifier);
        Commit(PortableValueName::WinGetSourceIdentifier, WinGetSourceIdentifier);
        Commit(PortableValueName::UninstallString, "winget uninstall --product-code " + GetProductCode());
        Commit(PortableValueName::WinGetInstallerType, InstallerTypeToString(Manifest::InstallerTypeEnum::Portable));
        Commit(PortableValueName::SHA256, SHA256);
        Commit(PortableValueName::InstallLocation, InstallLocation);
    }

    void PortableInstaller::FinalizeRegistryEntry()
    {
        Commit(PortableValueName::DisplayName, DisplayName);
        Commit(PortableValueName::DisplayVersion, DisplayVersion);
        Commit(PortableValueName::Publisher, Publisher);
        Commit(PortableValueName::InstallDate, InstallDate);
        Commit(PortableValueName::URLInfoAbout, URLInfoAbout);
        Commit(PortableValueName::HelpLink, HelpLink);
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