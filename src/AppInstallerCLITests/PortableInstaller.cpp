// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/PathVariable.h>
#include <winget/Manifest.h>
#include <PortableInstaller.h>
#include <Public/AppInstallerArchitecture.h>
#include <winget/PortableARPEntry.h>
#include <winget/Filesystem.h>
#include <Microsoft/SQLiteStorageBase.h>
#include <Microsoft/Schema/IPortableIndex.h>
#include <Microsoft/PortableIndex.h>

using namespace std::string_literals;
using namespace AppInstaller::CLI::Portable;
using namespace AppInstaller::Filesystem;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Registry::Environment;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Repository::SQLite;
using namespace AppInstaller::Repository::Microsoft::Schema;
using namespace AppInstaller::Utility;
using namespace TestCommon;

PortableInstaller CreateTestPortableInstaller(const std::filesystem::path& installLocation)
{
    PortableInstaller portableInstaller = PortableInstaller(
        ScopeEnum::User,
        Architecture::X64,
        "testProductCode");

    const auto& filename = "testPortable.txt";
    const auto& alias = "testSymlink.exe";

    portableInstaller.InstallLocation = installLocation;
    portableInstaller.PortableTargetFullPath = installLocation / filename;
    portableInstaller.PortableSymlinkFullPath = GetPortableLinksLocation(ScopeEnum::User) / alias;
    return portableInstaller;
}

std::vector<std::filesystem::path> CreateExtractedItemsFromArchive(const std::filesystem::path& installLocation)
{
    std::vector<std::filesystem::path> extractedItems;
    const auto& itemPath = installLocation / "testPortable.txt";
    const auto& itemPath2 = installLocation / "testPortable2.txt";
    extractedItems.emplace_back(itemPath);
    extractedItems.emplace_back(itemPath2);

    for (const auto& item : extractedItems)
    {
        std::ofstream file(item, std::ofstream::out);
        file.close();
    }

    return extractedItems;
}

std::vector<NestedInstallerFile> CreateTestNestedInstallerFiles()
{
    std::vector<NestedInstallerFile> nestedInstallerFiles;
    NestedInstallerFile file;
    file.RelativeFilePath = "testPortable.txt";
    nestedInstallerFiles.emplace_back(file);

    NestedInstallerFile file2;
    file2.RelativeFilePath = "testPortable2.txt";
    file2.PortableCommandAlias = "testSymlink.exe";
    nestedInstallerFiles.emplace_back(file2);

    return nestedInstallerFiles;
}

// Ensures that the portable exes and symlinks all got recorded in the index.
void VerifyPortableFilesTrackedByIndex(
    const std::filesystem::path& indexPath,
    const std::vector<std::filesystem::path>& extractedItems,
    const std::vector<NestedInstallerFile>& nestedInstallerFiles)
{
    {
        // Verify that files were added to index.
        Connection connection = Connection::Create(indexPath.u8string(), Connection::OpenDisposition::ReadWrite);
        REQUIRE(!Schema::Portable_V1_0::PortableTable::IsEmpty(connection));

        for (const auto& item : extractedItems)
        {
            REQUIRE(Schema::Portable_V1_0::PortableTable::SelectByFilePath(connection, item).has_value());
        }

        // Verify that all symlinks were added to the index.
        for (const auto& item : nestedInstallerFiles)
        {
            std::filesystem::path symlinkPath = GetPortableLinksLocation(ScopeEnum::User);
            if (!item.PortableCommandAlias.empty())
            {
                symlinkPath /= ConvertToUTF16(item.PortableCommandAlias);
            }
            else
            {
                symlinkPath /= ConvertToUTF16(item.RelativeFilePath);
            }

            AppendExtension(symlinkPath, ".exe");

            REQUIRE(Schema::Portable_V1_0::PortableTable::SelectByFilePath(connection, symlinkPath).has_value());
        }
    }
}

TEST_CASE("PortableInstaller_SingleInstall", "[PortableInstaller]")
{
    TempDirectory tempDirectory = TestCommon::TempDirectory("TempDirectory", false);

    {
        PortableInstaller portableInstaller = CreateTestPortableInstaller(tempDirectory.GetPath());

        TestCommon::TempFile testPortable("testPortable.txt");
        std::ofstream file2(testPortable, std::ofstream::out);
        file2.close();

        HRESULT installResult = portableInstaller.InstallSingle(testPortable.GetPath());
        REQUIRE(SUCCEEDED(installResult));
    }

    {
        // Create a new portable installer instance and verify that values from ARP are loaded correctly.
        PortableInstaller portableInstaller = PortableInstaller(ScopeEnum::User, Architecture::X64, "testProductCode");

        REQUIRE(std::filesystem::exists(portableInstaller.PortableTargetFullPath));
        REQUIRE(SymlinkExists(portableInstaller.PortableSymlinkFullPath));
        REQUIRE(VerifySymlink(portableInstaller.PortableSymlinkFullPath, portableInstaller.PortableTargetFullPath));
        REQUIRE(portableInstaller.ARPEntryExists());

        HRESULT uninstallResult = portableInstaller.Uninstall();

        REQUIRE(SUCCEEDED(uninstallResult));
        REQUIRE_FALSE(SymlinkExists(portableInstaller.PortableSymlinkFullPath));
        REQUIRE_FALSE(std::filesystem::exists(portableInstaller.PortableTargetFullPath));
        REQUIRE_FALSE(std::filesystem::exists(portableInstaller.InstallLocation));
    }
}

TEST_CASE("PortableInstaller_InstallDirectoryAddedToPath", "[PortableInstaller]")
{
    TempDirectory tempDirectory = TestCommon::TempDirectory("TempDirectory", false);

    {
        PortableInstaller portableInstaller = CreateTestPortableInstaller(tempDirectory.GetPath());

        TestCommon::TempFile testPortable("testPortable.txt");
        std::ofstream file2(testPortable, std::ofstream::out);
        file2.close();

        // This value is only set to true if we fail to create a symlink.
        // If true no symlink should be created and InstallDirectory is added to PATH variable.
        portableInstaller.CommitToARPEntry(PortableValueName::InstallDirectoryAddedToPath, portableInstaller.InstallDirectoryAddedToPath = true);

        HRESULT installResult = portableInstaller.InstallSingle(testPortable.GetPath());
        REQUIRE(SUCCEEDED(installResult));
    }

    {
        PortableInstaller portableInstaller = PortableInstaller(ScopeEnum::User, Architecture::X64, "testProductCode");

        REQUIRE(std::filesystem::exists(portableInstaller.PortableTargetFullPath));
        REQUIRE_FALSE(SymlinkExists(portableInstaller.PortableSymlinkFullPath));
        REQUIRE(PathVariable(ScopeEnum::User).Contains(portableInstaller.InstallLocation));
        REQUIRE(portableInstaller.ARPEntryExists());

        HRESULT uninstallResult = portableInstaller.Uninstall();

        REQUIRE(SUCCEEDED(uninstallResult));
        REQUIRE(portableInstaller.InstallDirectoryAddedToPath);
        REQUIRE_FALSE(std::filesystem::exists(portableInstaller.PortableTargetFullPath));
        REQUIRE_FALSE(std::filesystem::exists(portableInstaller.InstallLocation));
        REQUIRE_FALSE(PathVariable(ScopeEnum::User).Contains(portableInstaller.InstallLocation));
    }
}

TEST_CASE("PortableInstaller_MultipleInstall", "[PortableInstaller]")
{
    PortableInstaller portableInstaller = PortableInstaller(
        ScopeEnum::User,
        Architecture::X64,
        "testProductCode");

    TempDirectory tempDirectory = TestCommon::TempDirectory("TempDirectory", true);
    const auto& tempDirectoryPath = tempDirectory.GetPath();
    portableInstaller.InstallLocation = tempDirectoryPath;

    std::vector<NestedInstallerFile> nestedInstallerFiles = CreateTestNestedInstallerFiles();
    std::vector<std::filesystem::path> extractedItems = CreateExtractedItemsFromArchive(tempDirectoryPath);

    HRESULT installResult = portableInstaller.InstallMultiple(nestedInstallerFiles, extractedItems);
    REQUIRE(SUCCEEDED(installResult));

    const auto& indexPath = portableInstaller.GetPortableIndexPath();
    REQUIRE(std::filesystem::exists(indexPath));

    VerifyPortableFilesTrackedByIndex(indexPath, extractedItems, nestedInstallerFiles);

    REQUIRE(portableInstaller.VerifyPortableFilesForUninstall());

    // Perform uninstall
    HRESULT uninstallResult = portableInstaller.Uninstall();
    REQUIRE(SUCCEEDED(uninstallResult));

    REQUIRE_FALSE(std::filesystem::exists(indexPath));
    REQUIRE_FALSE(std::filesystem::exists(portableInstaller.InstallLocation));
}

TEST_CASE("PortableInstaller_VerifyFilesFromIndex", "[PortableInstaller]")
{
    // Create installer and set install location to temp directory
    // Create portable index and add files
    PortableInstaller portableInstaller = PortableInstaller(
        ScopeEnum::User,
        Architecture::X64,
        "testProductCode");

    TempDirectory tempDirectory = TestCommon::TempDirectory("TempDirectory", true);
    const auto& tempDirectoryPath = tempDirectory.GetPath();
    portableInstaller.InstallLocation = tempDirectoryPath;
    
    std::filesystem::path indexPath = tempDirectoryPath / "portable.db";

    PortableIndex index = PortableIndex::CreateNew(indexPath.u8string(), Schema::Version::Latest());
    
    std::filesystem::path symlinkPath = tempDirectoryPath / "symlink.exe";
    std::filesystem::path exePath = tempDirectoryPath / "testExe.txt";

    // Create exe and symlink
    TestCommon::TempFile testFile(exePath);
    std::ofstream file2(testFile, std::ofstream::out);
    file2.close();

    std::filesystem::create_symlink(exePath, symlinkPath);

    // Add files to index
    IPortableIndex::PortableFile exeFile = PortableIndex::CreatePortableFileFromPath(exePath);
    IPortableIndex::PortableFile symlinkFile = PortableIndex::CreatePortableFileFromPath(symlinkPath);
    
    index.AddPortableFile(exeFile);
    index.AddPortableFile(symlinkFile);

    REQUIRE(portableInstaller.VerifyPortableFilesForUninstall());

    // Modify symlink target
    std::filesystem::remove(symlinkPath);
    std::filesystem::create_symlink("badPath", symlinkPath);

    REQUIRE_FALSE(portableInstaller.VerifyPortableFilesForUninstall());

    std::filesystem::remove(symlinkPath);

    // Modify exe hash in index
    exeFile.SHA256 = "2413fb3709b05939f04cf2e92f7d0897fc2596f9ad0b8a9ea855c7bfebaae892";
    index.UpdatePortableFile(exeFile);

    REQUIRE_FALSE(portableInstaller.VerifyPortableFilesForUninstall());

    std::filesystem::remove(exePath);

    // Files that do not exist should still pass as they have already been removed.
    REQUIRE(portableInstaller.VerifyPortableFilesForUninstall());
}

TEST_CASE("PortableInstaller_VerifyFiles", "[PortableInstaller]")
{
    PortableInstaller portableInstaller = PortableInstaller(
        ScopeEnum::User,
        Architecture::X64,
        "testProductCode");

    TempDirectory tempDirectory = TestCommon::TempDirectory("TempDirectory", true);
    const auto& tempDirectoryPath = tempDirectory.GetPath();
    portableInstaller.InstallLocation = tempDirectoryPath;

    std::filesystem::path symlinkPath = tempDirectoryPath / "symlink.exe";
    std::filesystem::path exePath = tempDirectoryPath / "testExe.txt";

    // Create and set test file, symlink, and hash
    TestCommon::TempFile testFile(exePath);
    std::ofstream file(testFile, std::ofstream::out);
    file.close();

    std::filesystem::create_symlink(exePath, symlinkPath);

    std::ifstream inStream{ exePath, std::ifstream::binary };
    const SHA256::HashBuffer& targetFileHash = SHA256::ComputeHash(inStream);
    inStream.close();
    portableInstaller.SHA256 = SHA256::ConvertToString(targetFileHash);
    portableInstaller.PortableTargetFullPath = exePath;
    portableInstaller.PortableSymlinkFullPath = symlinkPath;

    REQUIRE(portableInstaller.VerifyPortableFilesForUninstall());

    // Modify symlink target
    std::filesystem::remove(symlinkPath);
    std::filesystem::create_symlink("badPath", symlinkPath);

    REQUIRE_FALSE(portableInstaller.VerifyPortableFilesForUninstall());

    // Modify exe hash
    std::filesystem::remove(symlinkPath);
    portableInstaller.SHA256 = "2413fb3709b05939f04cf2e92f7d0897fc2596f9ad0b8a9ea855c7bfebaae892";

    REQUIRE_FALSE(portableInstaller.VerifyPortableFilesForUninstall());

    std::filesystem::remove(exePath);

    // Files that do not exist should still pass as they have already been removed.
    REQUIRE(portableInstaller.VerifyPortableFilesForUninstall());
}