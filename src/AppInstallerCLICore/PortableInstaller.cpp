// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PortableInstaller.h"
#include "winget/PortableARPEntry.h"
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
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Repository::Microsoft::Schema;

namespace AppInstaller::CLI::Portable
{
    namespace
    {
        constexpr std::string_view c_PortableIndexFileName = "portable.db";
        constexpr std::string_view s_DefaultSource = "*DefaultSource"sv;

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

    HRESULT PortableInstaller::MultipleInstall(const std::vector<Manifest::NestedInstallerFile>& nestedInstallerFiles, const std::vector<std::filesystem::path>& extractedItems)
    {
        const auto& indexPath = InstallLocation / Utility::ConvertToUTF16(c_PortableIndexFileName);

        if (!std::filesystem::exists(indexPath))
        {
            PortableIndex::CreateNew(indexPath.u8string(), Schema::Version::Latest());
        }

        PortableIndex portableIndex = PortableIndex::Open(indexPath.u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite);

        for (auto& item : extractedItems)
        {
            const auto& itemPath = InstallLocation / item;
            // This method should be able to create any portable file based on the extension.
            IPortableIndex::PortableFile portableFile = CreatePortableFileFromPath(itemPath);

            if (portableIndex.Exists(portableFile))
            {
                portableIndex.UpdatePortableFile(portableFile);
            }
            else
            {
                portableIndex.AddPortableFile(portableFile);
            }
        }

        for (const auto& nestedInstallerFile : nestedInstallerFiles)
        {
            const std::filesystem::path& portableTargetPath = InstallLocation / ConvertToUTF16(nestedInstallerFile.RelativeFilePath);

            if (!InstallDirectoryAddedToPath)
            {
                // append .exe if needed for portable command alias if it exists, otherwise just use portalbe command alias.
                const std::filesystem::path& symlinkFullPath = GetPortableLinksLocation(GetScope()) / ConvertToUTF16(nestedInstallerFile.PortableCommandAlias);
                CreatePortableSymlink(portableTargetPath, symlinkFullPath);

                // add portable file only if created symlink is true.
                IPortableIndex::PortableFile symlinkPortableFile = CreatePortableFileFromPath(symlinkFullPath);
                if (portableIndex.Exists(symlinkPortableFile))
                {
                    portableIndex.UpdatePortableFile(symlinkPortableFile);
                }
                else
                {
                    portableIndex.AddPortableFile(symlinkPortableFile);
                }
            }
            else
            {
                AICLI_LOG(CLI, Info, << "Package directory was previously added to PATH. Skipping symlink creation.");
            }
        }

        AddToPathVariable();
        return ERROR_SUCCESS;
    }

    // providing a path and the alias
    // providing a list of extracted files and the NestedInstallerFiles, 

    HRESULT PortableInstaller::SingleInstall(const std::filesystem::path& installerPath)
    {
        // Initial registration.
        Commit(PortableValueName::WinGetPackageIdentifier, WinGetPackageIdentifier);
        Commit(PortableValueName::WinGetSourceIdentifier, WinGetSourceIdentifier);
        Commit(PortableValueName::UninstallString, "winget uninstall --product-code " + GetProductCode());
        Commit(PortableValueName::WinGetInstallerType, InstallerTypeToString(Manifest::InstallerTypeEnum::Portable));
        Commit(PortableValueName::SHA256, SHA256);


        Commit(PortableValueName::PortableTargetFullPath, PortableTargetFullPath);
        Commit(PortableValueName::InstallLocation, InstallLocation);
        MovePortableExe(installerPath);

        // Create symlink
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
        return ERROR_SUCCESS;
    }

    HRESULT PortableInstaller::Uninstall(bool purge)
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

    HRESULT PortableInstaller::UninstallFromIndex(bool purge)
    {
        if (purge)
        {
            std::filesystem::remove(InstallLocation);
            return ERROR_SUCCESS;
        }

        const auto& indexPath = InstallLocation / Utility::ConvertToUTF16(c_PortableIndexFileName);
        if (!std::filesystem::exists(indexPath))
        {
            // portable index not found;
            return ERROR_NOT_SUPPORTED;
        }

        PortableIndex portableIndex = PortableIndex::Open(indexPath.u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite);

        IPortableIndex::PortableFile file;
        int i = 0;
        while (portableIndex.GetPortableFileById(i).has_value())
        {
            file = portableIndex.GetPortableFileById(i).value();

            if (file.FileType == IPortableIndex::PortableFileType::File)
            {
                if (VerifyPortableExeHash(file.GetFilePath(), file.SHA256))
                {
                    std::filesystem::remove(file.GetFilePath());
                    portableIndex.RemovePortableFile(file);
                }
                else
                {
                    i++;
                }
            }
            else if (file.FileType == IPortableIndex::PortableFileType::Directory)
            {
                std::filesystem::remove(file.GetFilePath());
                portableIndex.RemovePortableFile(file);
            }
            else if (file.FileType == IPortableIndex::PortableFileType::Symlink)
            {
                if (VerifySymlinkTarget(file.GetFilePath(), file.SymlinkTarget))
                {
                    std::filesystem::remove(file.GetFilePath());
                    portableIndex.RemovePortableFile(file);
                }
                else
                {
                    i++;
                }
            }
        }

        // remove install directory
        RemovePortableDirectory(InstallLocation, purge, InstallDirectoryCreated);

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

    void PortableInstaller::SetAppsAndFeaturesMetadata(const Manifest::AppsAndFeaturesEntry& entry, const Manifest::Manifest& manifest)
    {
        Commit(PortableValueName::DisplayName, DisplayName = entry.DisplayName);
        Commit(PortableValueName::DisplayVersion, DisplayVersion = entry.DisplayVersion);
        Commit(PortableValueName::Publisher, Publisher = entry.Publisher);
        Commit(PortableValueName::InstallDate, InstallDate = AppInstaller::Utility::GetCurrentDateForARP());
        Commit(PortableValueName::URLInfoAbout, URLInfoAbout = manifest.CurrentLocalization.Get<Manifest::Localization::PackageUrl>());
        Commit(PortableValueName::HelpLink, HelpLink = manifest.CurrentLocalization.Get < Manifest::Localization::PublisherSupportUrl>());
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

    bool PortableInstaller::VerifyPortableExeHash(const std::filesystem::path& targetPath, const std::string& hashValue)
    {
        std::ifstream inStream{ targetPath, std::ifstream::binary };
        const Utility::SHA256::HashBuffer& targetFileHash = Utility::SHA256::ComputeHash(inStream);
        inStream.close();

        return Utility::SHA256::AreEqual(Utility::SHA256::ConvertToBytes(hashValue), targetFileHash);
    }

    void PortableInstaller::AddToPathVariable()
    {
        const std::filesystem::path& pathValue = GetPathDirectory();
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

    void PortableInstaller::CreatePortableSymlink(const std::filesystem::path& targetPath, const std::filesystem::path& symlinkPath)
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
        }
        else
        {
            // Symlink creation should only fail if the user executes in user mode and non-admin.
            // Resort to adding install directory to PATH directly.
            AICLI_LOG(Core, Info, << "Portable install executed in user mode. Adding package directory to PATH.");
            Commit(PortableValueName::InstallDirectoryAddedToPath, InstallDirectoryAddedToPath = true);
        }
    }

    bool PortableInstaller::VerifySymlinkTarget(const std::filesystem::path& targetPath, const std::filesystem::path& symlinkPath)
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

    void PortableInstaller::RemovePortableExe(const std::filesystem::path& targetPath, const std::string& hash)
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

    void PortableInstaller::RemovePortableSymlink(const std::filesystem::path& targetPath, const std::filesystem::path& symlinkPath)
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

    void PortableInstaller::RemovePortableDirectory(const std::filesystem::path& directoryPath, bool purge, bool isCreated)
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
                //m_stream << Resource::String::FilesRemainInInstallDirectory << directoryPath << std::endl;
            }
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Install directory does not exist: " << directoryPath);
        }
    }

    bool PortableInstaller::RemoveFromPathVariable()
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

    PortableInstaller::PortableInstaller(Manifest::ScopeEnum scope, Utility::Architecture arch, const std::string& productCode) :
        m_portableARPEntry(PortableARPEntry(scope, arch, productCode))
    {
        Initialize();
    }

    void PortableInstaller::Initialize()
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