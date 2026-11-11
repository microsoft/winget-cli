// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestHooks.h"
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

TEST_CASE("PortableInstaller_InstallAndUpgradeReplaceInstalledState", "[PortableInstaller]")
{
    const bool recordToIndex = GENERATE(false, true);
    const bool targetAlreadyExists = GENERATE(false, true);
    const auto operation = GENERATE(OperationType::Install, OperationType::Upgrade);
    CAPTURE(recordToIndex, targetAlreadyExists, operation);
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

    PortableInstaller updater(ScopeEnum::User, Architecture::X64, productCode, operation);
    REQUIRE(updater.IsUpdate() == (operation == OperationType::Upgrade));
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

TEST_CASE("PortableInstaller_RollbackRemovesPartialInstall", "[PortableInstaller]")
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
    REQUIRE(std::filesystem::exists(indexPath));
    installer.PrepareForCleanUp();
    installer.Uninstall();
    REQUIRE_FALSE(std::filesystem::exists(targetDirectory.GetPath()));
    REQUIRE(std::filesystem::exists(blockedAlias.GetPath()));
}

TEST_CASE("PortableInstaller_ReinstallReplacesDirectoryTarget", "[PortableInstaller]")
{
    const bool trackedDirectory = GENERATE(false, true);
    CAPTURE(trackedDirectory);
    const std::string productCode = "PortableDirectoryReinstallTest";
    TempDirectory installRoot("PortableDirectoryReinstallRoot", true);
    TempDirectory oldSource("PortableDirectoryReinstallOld", true);
    TempDirectory newSource("PortableDirectoryReinstallNew", true);
    std::ofstream(oldSource.GetPath() / "old.txt") << "old";
    std::ofstream(newSource.GetPath() / "new.txt") << "new";
    const auto directoryPath = installRoot.GetPath() / "package";
    PortableARPEntry arpEntry(ScopeEnum::User, Architecture::X64, productCode);
    auto cleanup = wil::scope_exit([&]() { arpEntry.Delete(); });

    if (trackedDirectory)
    {
        PortableInstaller installer(ScopeEnum::User, Architecture::X64, productCode, OperationType::Install);
        installer.TargetInstallLocation = installRoot.GetPath();
        installer.RecordToIndex = true;
        installer.SetDesiredState({ PortableFileEntry::CreateDirectoryEntry(oldSource.GetPath(), directoryPath) });
        installer.Install();
        REQUIRE(std::filesystem::exists(directoryPath / "old.txt"));
    }
    else
    {
        std::filesystem::create_directory(directoryPath);
    }

    PortableInstaller reinstaller(ScopeEnum::User, Architecture::X64, productCode, OperationType::Install);
    reinstaller.TargetInstallLocation = installRoot.GetPath();
    reinstaller.RecordToIndex = true;
    reinstaller.SetDesiredState({ PortableFileEntry::CreateDirectoryEntry(newSource.GetPath(), directoryPath) });
    REQUIRE_FALSE(reinstaller.IsUpdate());
    reinstaller.Install();
    REQUIRE(std::filesystem::exists(directoryPath / "new.txt"));
    REQUIRE_FALSE(std::filesystem::exists(directoryPath / "old.txt"));

    PortableInstaller uninstaller(ScopeEnum::User, Architecture::X64, productCode, OperationType::Uninstall);
    uninstaller.Uninstall();
    REQUIRE_FALSE(std::filesystem::exists(directoryPath));
    REQUIRE(std::filesystem::exists(installRoot.GetPath()));
}

TEST_CASE("PortableInstaller_RollbackPreservesUnappliedDirectories", "[PortableInstaller]")
{
    const bool recordToIndex = GENERATE(false, true);
    const bool installFileFirst = GENERATE(false, true);
    CAPTURE(recordToIndex, installFileFirst);
    const std::string productCode = "PortableUnappliedDirectoriesTest";
    TempDirectory installRoot("PortableUnappliedDirectoriesRoot", true);
    TempDirectory directorySource("PortableUnappliedDirectorySource", true);
    TempFile fileSource("PortableAppliedFile.txt");
    std::ofstream(fileSource.GetPath()) << "new";
    const auto installedFile = installRoot.GetPath() / "new.txt";
    const auto failedDirectory = installRoot.GetPath() / "existing";
    const auto unattemptedDirectory = installRoot.GetPath() / "unattempted";
    std::filesystem::create_directory(failedDirectory);
    std::filesystem::create_directory(unattemptedDirectory);
    std::ofstream(failedDirectory / "keep.txt") << "existing";
    std::ofstream(unattemptedDirectory / "keep.txt") << "unattempted";

    PortableARPEntry arpEntry(ScopeEnum::User, Architecture::X64, productCode);
    auto cleanup = wil::scope_exit([&]() { arpEntry.Delete(); });
    PortableInstaller installer(ScopeEnum::User, Architecture::X64, productCode, OperationType::Install);
    installer.TargetInstallLocation = installRoot.GetPath();
    installer.RecordToIndex = recordToIndex;
    std::vector<PortableFileEntry> desiredEntries;
    if (installFileFirst)
    {
        desiredEntries.emplace_back(PortableFileEntry::CreateFileEntry(fileSource.GetPath(), installedFile, {}));
    }
    desiredEntries.emplace_back(PortableFileEntry::CreateDirectoryEntry(directorySource.GetPath(), failedDirectory));
    desiredEntries.emplace_back(PortableFileEntry::CreateDirectoryEntry(directorySource.GetPath(), unattemptedDirectory));
    installer.SetDesiredState(desiredEntries);
    REQUIRE_THROWS_HR(installer.Install(), HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS));
    REQUIRE(std::filesystem::exists(installedFile) == installFileFirst);
    REQUIRE(std::filesystem::exists(directorySource.GetPath()));

    const auto indexPath = installRoot.GetPath() / installer.GetPortableIndexFileName();
    if (recordToIndex)
    {
        auto index = PortableIndex::Open(indexPath.u8string(), SQLiteStorageBase::OpenDisposition::Read);
        REQUIRE(index.GetAllPortableFiles().size() == static_cast<size_t>(installFileFirst ? 1 : 0));
    }

    installer.PrepareForCleanUp();
    installer.Uninstall();
    REQUIRE_FALSE(std::filesystem::exists(installedFile));
    REQUIRE_FALSE(std::filesystem::exists(indexPath));
    for (const auto& directory : { failedDirectory, unattemptedDirectory })
    {
        std::ifstream file(directory / "keep.txt");
        std::string contents;
        std::getline(file, contents);
        REQUIRE(contents == (directory == failedDirectory ? "existing" : "unattempted"));
    }
}

TEST_CASE("PortableInstaller_RollbackHandlesFailedReplacement", "[PortableInstaller]")
{
    const std::string productCode = "PortableUnindexedReplacementTest";
    TempDirectory installRoot("PortableUnindexedReplacementRoot", false);
    TempFile source("PortableUnindexedReplacementSource.txt");
    std::ofstream(source.GetPath()) << "source";
    const auto targetPath = installRoot.GetPath() / "missing.txt";
    auto desiredEntry = PortableFileEntry::CreateFileEntry(source.GetPath(), targetPath, {});
    std::filesystem::remove(source.GetPath());

    PortableARPEntry arpEntry(ScopeEnum::User, Architecture::X64, productCode);
    auto cleanup = wil::scope_exit([&]() { arpEntry.Delete(); });
    PortableInstaller installer(ScopeEnum::User, Architecture::X64, productCode, OperationType::Install);
    installer.TargetInstallLocation = installRoot.GetPath();
    installer.RecordToIndex = true;
    installer.SetDesiredState({ desiredEntry });
    REQUIRE_THROWS(installer.Install());

    {
        const auto indexPath = installRoot.GetPath() / installer.GetPortableIndexFileName();
        auto index = PortableIndex::Open(indexPath.u8string(), SQLiteStorageBase::OpenDisposition::Read);
        REQUIRE(index.Exists(desiredEntry));
    }
    installer.PrepareForCleanUp();
    REQUIRE_NOTHROW(installer.Uninstall());
    REQUIRE_FALSE(std::filesystem::exists(installRoot.GetPath()));
}

TEST_CASE("PortableInstaller_FailedDirectoryUpgradeRetainsOwnership", "[PortableInstaller]")
{
    const bool retryUpgrade = GENERATE(false, true);
    CAPTURE(retryUpgrade);
    const std::string productCode = "PortablePartialDirectoryUpgradeTest";
    TempDirectory installRoot("PortablePartialDirectoryUpgradeRoot", true);
    TempDirectory source("PortablePartialDirectoryUpgradeSource", true);
    const auto sourceSubdirectory = source.GetPath() / "nested";
    std::filesystem::create_directory(sourceSubdirectory);
    const auto sourceFile = sourceSubdirectory / "locked.txt";
    std::ofstream(sourceFile) << "replacement";
    const auto targetDirectory = installRoot.GetPath() / "package";
    const auto untouchedDirectory = installRoot.GetPath() / "untouched";
    std::filesystem::create_directory(untouchedDirectory);
    std::ofstream(untouchedDirectory / "keep.txt") << "untouched";
    const auto desiredEntry = PortableFileEntry::CreateDirectoryEntry(source.GetPath(), targetDirectory);
    const auto untouchedEntry = PortableFileEntry::CreateDirectoryEntry(source.GetPath(), untouchedDirectory);
    PortableARPEntry arpEntry(ScopeEnum::User, Architecture::X64, productCode);
    auto cleanup = wil::scope_exit([&]() { arpEntry.Delete(); });

    PortableInstaller installer(ScopeEnum::User, Architecture::X64, productCode, OperationType::Install);
    installer.TargetInstallLocation = installRoot.GetPath();
    installer.RecordToIndex = true;
    installer.Install();

    bool sameVolume = false;
    AppInstaller::Filesystem::TestHook_SetIsSameVolume_Override(&sameVolume);
    auto restoreVolume = wil::scope_exit([]()
        {
            AppInstaller::Filesystem::TestHook_SetIsSameVolume_Override(nullptr);
        });
    wil::unique_hfile lockedFile{ CreateFileW(sourceFile.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr) };
    REQUIRE(lockedFile);

    PortableInstaller updater(ScopeEnum::User, Architecture::X64, productCode, OperationType::Upgrade);
    updater.TargetInstallLocation = installRoot.GetPath();
    updater.RecordToIndex = true;
    updater.SetDesiredState({ desiredEntry, untouchedEntry });
    REQUIRE_THROWS_AS(updater.Install(), std::filesystem::filesystem_error);
    REQUIRE(std::filesystem::is_directory(targetDirectory / "nested"));
    PortableARPEntry currentARPEntry(ScopeEnum::User, Architecture::X64, productCode);
    REQUIRE(currentARPEntry.Exists());

    {
        const auto indexPath = installRoot.GetPath() / updater.GetPortableIndexFileName();
        auto index = PortableIndex::Open(indexPath.u8string(), SQLiteStorageBase::OpenDisposition::Read);
        REQUIRE(index.GetAllPortableFiles().size() == 1);
        REQUIRE(index.Exists(desiredEntry));
        REQUIRE_FALSE(index.Exists(untouchedEntry));
    }

    lockedFile.reset();
    if (retryUpgrade)
    {
        PortableInstaller retry(ScopeEnum::User, Architecture::X64, productCode, OperationType::Upgrade);
        retry.TargetInstallLocation = installRoot.GetPath();
        retry.RecordToIndex = true;
        retry.SetDesiredState({ desiredEntry });
        REQUIRE(retry.VerifyExpectedState());
        REQUIRE_NOTHROW(retry.Install());
        REQUIRE(std::filesystem::exists(targetDirectory / "nested" / "locked.txt"));
    }

    PortableInstaller uninstaller(ScopeEnum::User, Architecture::X64, productCode, OperationType::Uninstall);
    REQUIRE_FALSE(uninstaller.Purge);
    REQUIRE(uninstaller.VerifyExpectedState());
    REQUIRE_NOTHROW(uninstaller.Uninstall());
    REQUIRE_FALSE(std::filesystem::exists(targetDirectory));
    std::ifstream untouchedFile(untouchedDirectory / "keep.txt");
    std::string contents;
    std::getline(untouchedFile, contents);
    REQUIRE(contents == "untouched");
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
