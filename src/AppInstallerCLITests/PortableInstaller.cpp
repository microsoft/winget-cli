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
    const std::vector<NestedInstallerFile>& nestedInstallerFiles)
{
    {
        // Verify that files were added to index.
        Connection connection = Connection::Create(indexPath.u8string(), Connection::OpenDisposition::ReadWrite);
        REQUIRE(!Schema::Portable_V1_0::PortableTable::IsEmpty(connection));

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

TEST_CASE("PortableInstaller_InstallToRegistry", "[PortableInstaller]")
{
    TempDirectory tempDirectory = TestCommon::TempDirectory("TempDirectory", false);

    std::vector<PortableFileEntry> desiredTestState;

    TestCommon::TempFile testPortable("testPortable.txt");
    std::ofstream file(testPortable, std::ofstream::out);
    file.close();

    std::filesystem::path targetPath = tempDirectory.GetPath() / "testPortable.txt";
    std::filesystem::path symlinkPath = tempDirectory.GetPath() / "testSymlink.exe";

    desiredTestState.emplace_back(std::move(PortableFileEntry::CreateFileEntry(testPortable.GetPath(), targetPath, {})));
    desiredTestState.emplace_back(std::move(PortableFileEntry::CreateSymlinkEntry(symlinkPath, targetPath)));

    PortableInstaller portableInstaller = PortableInstaller(ScopeEnum::User, Architecture::X64, "testProductCode");
    portableInstaller.TargetInstallDirectory = tempDirectory.GetPath();
    portableInstaller.SetDesiredState(desiredTestState);
    portableInstaller.Install();

    PortableInstaller portableInstaller2 = PortableInstaller(ScopeEnum::User, Architecture::X64, "testProductCode");
    REQUIRE(portableInstaller2.ARPEntryExists());
    REQUIRE(std::filesystem::exists(portableInstaller2.PortableTargetFullPath));
    REQUIRE(AppInstaller::Filesystem::SymlinkExists(portableInstaller2.PortableSymlinkFullPath));

    portableInstaller2.Uninstall();
    REQUIRE_FALSE(std::filesystem::exists(portableInstaller2.PortableTargetFullPath));
    REQUIRE_FALSE(AppInstaller::Filesystem::SymlinkExists(portableInstaller2.PortableSymlinkFullPath));
    REQUIRE_FALSE(std::filesystem::exists(portableInstaller2.InstallLocation));
}

TEST_CASE("PortableInstaller_InstallToIndex", "[PortableInstaller]")
{
    TempDirectory installRootDirectory = TestCommon::TempDirectory("PortableInstallRoot", false);

    std::vector<PortableFileEntry> desiredTestState;

    TestCommon::TempFile testPortable("testPortable.txt");
    std::ofstream file1(testPortable, std::ofstream::out);
    file1.close();

    TestCommon::TempFile testPortable2("testPortable2.txt");
    std::ofstream file2(testPortable2, std::ofstream::out);
    file2.close();

    TestCommon::TempDirectory testDirectoryFolder("testDirectory", true);

    std::filesystem::path installRootPath = installRootDirectory.GetPath();
    std::filesystem::path targetPath = installRootPath / "testPortable.txt";
    std::filesystem::path targetPath2 = installRootPath / "testPortable2.txt";
    std::filesystem::path symlinkPath = installRootPath / "testSymlink.exe";
    std::filesystem::path symlinkPath2 = installRootPath / "testSymlink2.exe";
    std::filesystem::path directoryPath = installRootPath / "testDirectory";

    desiredTestState.emplace_back(std::move(PortableFileEntry::CreateFileEntry(testPortable.GetPath(), targetPath, {})));
    desiredTestState.emplace_back(std::move(PortableFileEntry::CreateFileEntry(testPortable2.GetPath(), targetPath2, {})));
    desiredTestState.emplace_back(std::move(PortableFileEntry::CreateSymlinkEntry(symlinkPath, targetPath)));
    desiredTestState.emplace_back(std::move(PortableFileEntry::CreateSymlinkEntry(symlinkPath2, targetPath2)));
    desiredTestState.emplace_back(std::move(PortableFileEntry::CreateDirectoryEntry(testDirectoryFolder.GetPath(), directoryPath)));

    PortableInstaller portableInstaller = PortableInstaller(ScopeEnum::User, Architecture::X64, "testProductCode");
    portableInstaller.TargetInstallDirectory = installRootDirectory.GetPath();
    portableInstaller.RecordToIndex = true;
    portableInstaller.SetDesiredState(desiredTestState);
    portableInstaller.Install();

    REQUIRE(std::filesystem::exists(targetPath));
    REQUIRE(std::filesystem::exists(targetPath2));
    REQUIRE(AppInstaller::Filesystem::SymlinkExists(symlinkPath));
    REQUIRE(AppInstaller::Filesystem::SymlinkExists(symlinkPath2));
    REQUIRE(std::filesystem::exists(directoryPath));

    PortableInstaller portableInstaller2 = PortableInstaller(ScopeEnum::User, Architecture::X64, "testProductCode");
    REQUIRE(portableInstaller2.ARPEntryExists());

    portableInstaller2.Uninstall();
    REQUIRE_FALSE(std::filesystem::exists(targetPath));
    REQUIRE_FALSE(std::filesystem::exists(targetPath2));
    REQUIRE_FALSE(AppInstaller::Filesystem::SymlinkExists(symlinkPath));
    REQUIRE_FALSE(AppInstaller::Filesystem::SymlinkExists(symlinkPath2));
    REQUIRE_FALSE(std::filesystem::exists(directoryPath));
}


