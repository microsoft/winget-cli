// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerErrors.h>
#include <PortableInstaller.h>
#include <Public/AppInstallerArchitecture.h>
#include <winget/Filesystem.h>
#include <winget/Manifest.h>
#include <winget/PathVariable.h>
#include <winget/PortableARPEntry.h>
#include <winget/SQLiteStorageBase.h>
#include <Microsoft/Schema/IPortableIndex.h>
#include <winget/PortableIndex.h>

using namespace std::string_literals;
using namespace AppInstaller::CLI::Portable;
using namespace AppInstaller::Filesystem;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Registry::Environment;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::SQLite;
using namespace AppInstaller::Repository::Microsoft::Schema;
using namespace AppInstaller::Utility;
using namespace AppInstaller::CLI::Workflow;
using namespace TestCommon;

TEST_CASE("PortableInstaller_FreshInstallIgnoresExistingState", "[PortableInstaller]")
{
    const bool recordToIndex = GENERATE(false, true);
    CAPTURE(recordToIndex);
    const std::string productCode = "PortableFreshStateTest";
    TempDirectory oldDirectory("PortableOldState", true);
    TempDirectory targetDirectory("PortableFreshState", true);
    TempFile source("PortableFreshSource.exe");
    std::ofstream(source.GetPath()) << "fresh";
    const auto oldFile = oldDirectory.GetPath() / "untouched.exe";
    std::ofstream(oldFile) << "old";
    const auto oldIndexPath = oldDirectory.GetPath() / (productCode + ".db");
    std::ofstream(oldIndexPath) << "not a database";
    const auto targetPath = targetDirectory.GetPath() / "fresh.exe";
    std::ofstream(targetPath) << "replace me";
    const auto targetIndexPath = targetDirectory.GetPath() / (productCode + ".db");
    {
        auto index = PortableIndex::CreateNew(targetIndexPath.u8string());
        index.AddPortableFile(PortableFileEntry::CreateFileEntry(oldFile, oldFile, {}));
    }
    for (std::wstring_view suffix : { L"-journal", L"-wal", L"-shm" })
    {
        auto sidecarPath = targetIndexPath;
        sidecarPath += suffix;
        std::ofstream(sidecarPath) << "old journal";
    }

    PortableARPEntry arpEntry(ScopeEnum::User, Architecture::X64, productCode);
    auto cleanup = wil::scope_exit([&]() { arpEntry.Delete(); });
    arpEntry.SetValue(PortableValueName::InstallLocation, oldDirectory.GetPath());
    arpEntry.SetValue(PortableValueName::PortableTargetFullPath, oldFile);
    arpEntry.SetValue(PortableValueName::PortableSymlinkFullPath, oldDirectory.GetPath() / "old-link.exe");
    arpEntry.SetValue(PortableValueName::WinGetPackageIdentifier, "old-package"s);
    arpEntry.SetValue(PortableValueName::WinGetSourceIdentifier, "old-source"s);
    bool oldFlag = true;
    arpEntry.SetValue(PortableValueName::InstallDirectoryCreated, oldFlag);
    arpEntry.SetValue(PortableValueName::InstallDirectoryAddedToPath, oldFlag);

    PortableInstaller installer(ScopeEnum::User, Architecture::X64, productCode, OperationType::Install);
    REQUIRE(installer.InstallLocation.empty());
    REQUIRE(installer.WinGetPackageIdentifier.empty());
    REQUIRE(installer.VerifyExpectedState());
    installer.TargetInstallLocation = targetDirectory.GetPath();
    installer.WinGetPackageIdentifier = "new-package";
    installer.WinGetSourceIdentifier = "new-source";
    installer.RecordToIndex = recordToIndex;
    installer.SetDesiredState({ PortableFileEntry::CreateFileEntry(source.GetPath(), targetPath, {}) });
    installer.Install();

    REQUIRE(std::filesystem::exists(oldFile));
    REQUIRE(std::filesystem::exists(oldIndexPath));
    REQUIRE(std::filesystem::exists(targetPath));
    {
        std::ifstream installedFile(targetPath);
        std::string contents;
        std::getline(installedFile, contents);
        REQUIRE(contents == "fresh");
    }
    for (std::wstring_view suffix : { L"-journal", L"-wal", L"-shm" })
    {
        auto sidecarPath = targetIndexPath;
        sidecarPath += suffix;
        REQUIRE_FALSE(std::filesystem::exists(sidecarPath));
    }
    if (recordToIndex)
    {
        auto index = PortableIndex::Open(targetIndexPath.u8string(), SQLiteStorageBase::OpenDisposition::Read);
        auto files = index.GetAllPortableFiles();
        REQUIRE(files.size() == 1);
        REQUIRE(std::filesystem::equivalent(files.at(0).GetFilePath(), targetPath));
    }
    else
    {
        REQUIRE_FALSE(std::filesystem::exists(targetIndexPath));
    }

    PortableInstaller uninstaller(ScopeEnum::User, Architecture::X64, productCode, OperationType::Uninstall);
    REQUIRE(uninstaller.InstallLocation == targetDirectory.GetPath());
    REQUIRE_FALSE(uninstaller.InstallDirectoryCreated);
    REQUIRE_FALSE(uninstaller.InstallDirectoryAddedToPath);
    REQUIRE(uninstaller.WinGetPackageIdentifier == "new-package");
    REQUIRE(uninstaller.WinGetSourceIdentifier == "new-source");
    if (recordToIndex)
    {
        REQUIRE(uninstaller.PortableTargetFullPath.empty());
        REQUIRE(uninstaller.PortableSymlinkFullPath.empty());
    }
    uninstaller.Uninstall();
    REQUIRE_FALSE(std::filesystem::exists(targetPath));
    REQUIRE(std::filesystem::exists(targetDirectory.GetPath()));
    REQUIRE(std::filesystem::exists(oldFile));
    REQUIRE(std::filesystem::exists(oldIndexPath));
}

TEST_CASE("PortableInstaller_UpgradeLoadsAndReplacesInstalledState", "[PortableInstaller]")
{
    const bool recordToIndex = GENERATE(false, true);
    const bool targetAlreadyExists = GENERATE(false, true);
    CAPTURE(recordToIndex, targetAlreadyExists);
    const std::string productCode = "PortableUpgradeStateTest";
    TempDirectory oldDirectory("PortableBeforeUpgrade", false);
    TempDirectory newDirectory("PortableAfterUpgrade", targetAlreadyExists);
    TempFile oldSource("PortableBeforeUpgrade.exe");
    TempFile newSource("PortableAfterUpgrade.exe");
    std::ofstream(oldSource.GetPath()) << "before";
    std::ofstream(newSource.GetPath()) << "after";
    const auto oldPath = oldDirectory.GetPath() / "before.exe";
    const auto newPath = newDirectory.GetPath() / "after.exe";
    PortableARPEntry arpEntry(ScopeEnum::User, Architecture::X64, productCode);
    auto cleanup = wil::scope_exit([&]() { arpEntry.Delete(); });

    PortableInstaller installer(ScopeEnum::User, Architecture::X64, productCode, OperationType::Install);
    installer.TargetInstallLocation = oldDirectory.GetPath();
    installer.RecordToIndex = recordToIndex;
    installer.SetDesiredState({ PortableFileEntry::CreateFileEntry(oldSource.GetPath(), oldPath, std::string(64, '0')) });
    installer.Install();

    PortableInstaller updater(ScopeEnum::User, Architecture::X64, productCode, OperationType::Upgrade);
    REQUIRE_FALSE(updater.VerifyExpectedState());
    updater.TargetInstallLocation = newDirectory.GetPath();
    updater.RecordToIndex = recordToIndex;
    updater.SetDesiredState({ PortableFileEntry::CreateFileEntry(newSource.GetPath(), newPath, {}) });
    updater.Install();
    REQUIRE_FALSE(std::filesystem::exists(oldDirectory.GetPath()));
    REQUIRE(std::filesystem::exists(newPath));

    PortableInstaller uninstaller(ScopeEnum::User, Architecture::X64, productCode, OperationType::Uninstall);
    REQUIRE(uninstaller.InstallLocation == newDirectory.GetPath());
    uninstaller.Uninstall();
    REQUIRE(std::filesystem::exists(newDirectory.GetPath()) == targetAlreadyExists);
}

TEST_CASE("PortableInstaller_FreshRollbackUsesInMemoryState", "[PortableInstaller]")
{
    const std::string productCode = "PortableRollbackStateTest";
    TempDirectory targetDirectory("PortableRollbackState", false);
    TempDirectory blockedAlias("PortableBlockedAlias", true);
    TempFile source("PortableRollbackSource.exe");
    std::ofstream(source.GetPath()) << "fresh";
    const auto targetPath = targetDirectory.GetPath() / "fresh.exe";
    PortableARPEntry arpEntry(ScopeEnum::User, Architecture::X64, productCode);
    auto cleanup = wil::scope_exit([&]() { arpEntry.Delete(); });

    PortableInstaller installer(ScopeEnum::User, Architecture::X64, productCode, OperationType::Install);
    installer.TargetInstallLocation = targetDirectory.GetPath();
    installer.RecordToIndex = true;
    installer.SetDesiredState({
        PortableFileEntry::CreateFileEntry(source.GetPath(), targetPath, {}),
        PortableFileEntry::CreateSymlinkEntry(blockedAlias.GetPath(), targetPath),
    });
    REQUIRE_THROWS_HR(installer.Install(), APPINSTALLER_CLI_ERROR_PORTABLE_SYMLINK_PATH_IS_DIRECTORY);
    REQUIRE(std::filesystem::exists(targetPath));

    const auto indexPath = targetDirectory.GetPath() / installer.GetPortableIndexFileName();
    std::filesystem::remove(indexPath);
    {
        std::ofstream corruptIndex(indexPath);
        corruptIndex << "not a database";
        REQUIRE(corruptIndex.good());
    }
    installer.PrepareForCleanUp();
    installer.Uninstall();
    REQUIRE_FALSE(std::filesystem::exists(targetDirectory.GetPath()));
    REQUIRE(std::filesystem::exists(blockedAlias.GetPath()));
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

    PortableInstaller portableInstaller = PortableInstaller(ScopeEnum::User, Architecture::X64, "testProductCode", OperationType::Install);
    portableInstaller.TargetInstallLocation = tempDirectory.GetPath();
    portableInstaller.SetDesiredState(desiredTestState);
    REQUIRE(portableInstaller.VerifyExpectedState());

    portableInstaller.Install();

    auto entry = portableInstaller.GetAppsAndFeaturesEntry();
    REQUIRE(entry.ProductCode == portableInstaller.GetProductCode());

    PortableInstaller portableInstaller2 = PortableInstaller(ScopeEnum::User, Architecture::X64, "testProductCode", OperationType::Uninstall);
    REQUIRE(portableInstaller2.ARPEntryExists());
    REQUIRE(std::filesystem::exists(portableInstaller2.PortableTargetFullPath));
    REQUIRE(AppInstaller::Filesystem::SymlinkExists(portableInstaller2.PortableSymlinkFullPath));

    portableInstaller2.Uninstall();
    REQUIRE_FALSE(std::filesystem::exists(portableInstaller2.PortableTargetFullPath));
    REQUIRE_FALSE(AppInstaller::Filesystem::SymlinkExists(portableInstaller2.PortableSymlinkFullPath));
    REQUIRE_FALSE(std::filesystem::exists(portableInstaller2.InstallLocation));
}

TEST_CASE("PortableInstaller_InstallToIndex_CreateInstallRoot", "[PortableInstaller]")
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

    PortableInstaller portableInstaller = PortableInstaller(ScopeEnum::User, Architecture::X64, "testProductCode", OperationType::Install);
    portableInstaller.TargetInstallLocation = installRootDirectory.GetPath();
    portableInstaller.RecordToIndex = true;
    portableInstaller.SetDesiredState(desiredTestState);
    REQUIRE(portableInstaller.VerifyExpectedState());

    portableInstaller.Install();

    REQUIRE(std::filesystem::exists(installRootPath / portableInstaller.GetPortableIndexFileName()));
    REQUIRE(std::filesystem::exists(targetPath));
    REQUIRE(std::filesystem::exists(targetPath2));
    REQUIRE(AppInstaller::Filesystem::SymlinkExists(symlinkPath));
    REQUIRE(AppInstaller::Filesystem::SymlinkExists(symlinkPath2));
    REQUIRE(std::filesystem::exists(directoryPath));

    PortableInstaller portableInstaller2 = PortableInstaller(ScopeEnum::User, Architecture::X64, "testProductCode", OperationType::Uninstall);
    REQUIRE(portableInstaller2.ARPEntryExists());

    portableInstaller2.Uninstall();

    // Install root directory should be removed since it was created.
    REQUIRE_FALSE(std::filesystem::exists(installRootPath));
    REQUIRE_FALSE(std::filesystem::exists(targetPath));
    REQUIRE_FALSE(std::filesystem::exists(targetPath2));
    REQUIRE_FALSE(AppInstaller::Filesystem::SymlinkExists(symlinkPath));
    REQUIRE_FALSE(AppInstaller::Filesystem::SymlinkExists(symlinkPath2));
    REQUIRE_FALSE(std::filesystem::exists(directoryPath));
}

TEST_CASE("PortableInstaller_InstallToIndex_ExistingInstallRoot", "[PortableInstaller]")
{
    TempDirectory installRootDirectory = TestCommon::TempDirectory("PortableInstallRoot", true);

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

    PortableInstaller portableInstaller = PortableInstaller(ScopeEnum::User, Architecture::X64, "testProductCode", OperationType::Install);
    portableInstaller.TargetInstallLocation = installRootDirectory.GetPath();
    portableInstaller.RecordToIndex = true;
    portableInstaller.SetDesiredState(desiredTestState);
    REQUIRE(portableInstaller.VerifyExpectedState());

    portableInstaller.Install();

    REQUIRE(std::filesystem::exists(installRootPath / portableInstaller.GetPortableIndexFileName()));
    REQUIRE(std::filesystem::exists(targetPath));
    REQUIRE(std::filesystem::exists(targetPath2));
    REQUIRE(AppInstaller::Filesystem::SymlinkExists(symlinkPath));
    REQUIRE(AppInstaller::Filesystem::SymlinkExists(symlinkPath2));
    REQUIRE(std::filesystem::exists(directoryPath));

    PortableInstaller portableInstaller2 = PortableInstaller(ScopeEnum::User, Architecture::X64, "testProductCode", OperationType::Uninstall);
    REQUIRE(portableInstaller2.ARPEntryExists());

    portableInstaller2.Uninstall();

    // Install root directory should still exist since it was created previously.
    REQUIRE(std::filesystem::exists(installRootPath));
    REQUIRE_FALSE(std::filesystem::exists(targetPath));
    REQUIRE_FALSE(std::filesystem::exists(targetPath2));
    REQUIRE_FALSE(AppInstaller::Filesystem::SymlinkExists(symlinkPath));
    REQUIRE_FALSE(AppInstaller::Filesystem::SymlinkExists(symlinkPath2));
    REQUIRE_FALSE(std::filesystem::exists(directoryPath));
}

TEST_CASE("PortableInstaller_UnicodeSymlinkPath", "[PortableInstaller]")
{
    TempDirectory tempDirectory = TestCommon::TempDirectory("TempDirectory", false);

    // Modify install location path to include unicode characters.
    std::filesystem::path testInstallLocation = tempDirectory.GetPath() / std::filesystem::path{ ConvertToUTF16("романтический") };

    std::vector<PortableFileEntry> desiredTestState;

    TestCommon::TempFile testPortable("testPortable.txt");
    std::ofstream file(testPortable, std::ofstream::out);
    file.close();

    std::filesystem::path targetPath = testInstallLocation / "testPortable.txt";
    std::filesystem::path symlinkPath = tempDirectory.GetPath() / "testSymlink.exe";

    desiredTestState.emplace_back(std::move(PortableFileEntry::CreateFileEntry(testPortable.GetPath(), targetPath, {})));
    desiredTestState.emplace_back(std::move(PortableFileEntry::CreateSymlinkEntry(symlinkPath, targetPath)));

    PortableInstaller portableInstaller = PortableInstaller(ScopeEnum::User, Architecture::X64, "testProductCode", OperationType::Install);
    portableInstaller.TargetInstallLocation = testInstallLocation;
    portableInstaller.SetDesiredState(desiredTestState);
    REQUIRE(portableInstaller.VerifyExpectedState());

    portableInstaller.Install();

    PortableInstaller portableInstaller2 = PortableInstaller(ScopeEnum::User, Architecture::X64, "testProductCode", OperationType::Uninstall);
    REQUIRE(portableInstaller2.ARPEntryExists());
    REQUIRE(std::filesystem::exists(portableInstaller2.PortableTargetFullPath));
    REQUIRE(AppInstaller::Filesystem::SymlinkExists(portableInstaller2.PortableSymlinkFullPath));

    portableInstaller2.Uninstall();
    REQUIRE_FALSE(std::filesystem::exists(portableInstaller2.PortableTargetFullPath));
    REQUIRE_FALSE(AppInstaller::Filesystem::SymlinkExists(portableInstaller2.PortableSymlinkFullPath));
    REQUIRE_FALSE(std::filesystem::exists(portableInstaller2.InstallLocation));
}
