// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/FolderFileWatcher.h>

using namespace TestCommon;
using namespace AppInstaller;

namespace
{
    void WriteText(const std::filesystem::path& path)
    {
        std::ofstream fileStream{ path };
        fileStream << "text";
    }
}

TEST_CASE("FolderFileWatcher_CreateNewFiles", "[FolderFileWatcher]")
{
    TempDirectory dirToWatch("FolderFileWatcher_CreateNewFiles_", true);

    Utility::FolderFileWatcher folderFileWatcher(dirToWatch.GetPath());
    folderFileWatcher.start();

    TempFile tmpFile1(dirToWatch.GetPath(), "file1_", ".txt");
    WriteText(tmpFile1.GetPath());

    TempFile tmpFile2(dirToWatch.GetPath(), "file2_", ".txt");
    WriteText(tmpFile2.GetPath());
    
    std::filesystem::path newTestDir = dirToWatch.GetPath();
    newTestDir /= "testDir";
    std::filesystem::create_directories(newTestDir);

    TempFile tmpFile3(newTestDir, "file3_", ".txt");
    WriteText(tmpFile3.GetPath());

    std::this_thread::sleep_for(100ms);
    folderFileWatcher.stop();

    auto& watchedFiles = folderFileWatcher.files();

    auto foundTmpFile1 = watchedFiles.find(tmpFile1.GetPath().string());
    REQUIRE(foundTmpFile1 != watchedFiles.cend());

    auto foundTmpFile2 = watchedFiles.find(tmpFile2.GetPath().string());
    REQUIRE(foundTmpFile2 != watchedFiles.cend());

    auto foundTmpFile3 = watchedFiles.find(tmpFile3.GetPath().string());
    REQUIRE(foundTmpFile3 != watchedFiles.cend());
}

TEST_CASE("FolderFileWatcher_CreateAfterStop", "[FolderFileWatcher]")
{
    TempDirectory dirToWatch("FolderFileWatcher_CreateAfterStop_", true);

    Utility::FolderFileWatcher folderFileWatcher(dirToWatch.GetPath());
    folderFileWatcher.start();

    TempFile tmpFile1(dirToWatch.GetPath(), "file1_", ".txt");
    WriteText(tmpFile1.GetPath());

    std::this_thread::sleep_for(100ms);
    folderFileWatcher.stop();

    TempFile tmpFile2(dirToWatch.GetPath(), "file2_", ".txt");
    WriteText(tmpFile2.GetPath());

    auto& watchedFiles = folderFileWatcher.files();

    auto foundTmpFile1 = watchedFiles.find(tmpFile1.GetPath().string());
    REQUIRE(foundTmpFile1 != watchedFiles.cend());

    auto foundTmpFile2 = watchedFiles.find(tmpFile2.GetPath().string());
    REQUIRE(foundTmpFile2 == watchedFiles.cend());
}

TEST_CASE("FolderFileWatcher_CreateNewFilesAndRename", "[FolderFileWatcher]")
{
    TempDirectory dirToWatch("FolderFileWatcher_CreateNewFilesAndRename_", true);

    Utility::FolderFileWatcher folderFileWatcher(dirToWatch.GetPath());
    folderFileWatcher.start();

    std::filesystem::path tmpFile1Path = dirToWatch.GetPath() / "file1.txt";
    TempFile tmpFile1(tmpFile1Path);
    WriteText(tmpFile1Path);

    std::filesystem::path newTestDir = dirToWatch.GetPath();
    newTestDir /= "testDir";
    std::filesystem::create_directories(newTestDir);

    std::filesystem::path tmpFile2Path = newTestDir / "file2.txt";
    TempFile tmpFile2(tmpFile2Path);
    WriteText(tmpFile2Path);

    std::filesystem::path tmpFile1PathRenamed = dirToWatch.GetPath() / "file1_renamed.txt";
    std::filesystem::path tmpFile2PathRenamed = newTestDir / "file2_renamed.txt";

    tmpFile1.Rename(tmpFile1PathRenamed);
    tmpFile2.Rename(tmpFile2PathRenamed);

    std::this_thread::sleep_for(100ms);
    folderFileWatcher.stop();

    auto& watchedFiles = folderFileWatcher.files();

    auto foundTmpFile1 = watchedFiles.find(tmpFile1Path.string());
    REQUIRE(foundTmpFile1 == watchedFiles.cend());

    auto foundTmpFile1Renamed = watchedFiles.find(tmpFile1PathRenamed.string());
    REQUIRE(foundTmpFile1Renamed != watchedFiles.cend());

    auto foundTmpFile2 = watchedFiles.find(tmpFile2Path.string());
    REQUIRE(foundTmpFile2 == watchedFiles.cend());

    auto foundTmpFile2Renamed = watchedFiles.find(tmpFile2PathRenamed.string());
    REQUIRE(foundTmpFile2Renamed != watchedFiles.cend());
}

TEST_CASE("FolderFileWatcher_CreateNewFilesAndDelete", "[FolderFileWatcher]")
{
    TempDirectory dirToWatch("FolderFileWatcher_CreateNewFilesAndDelete_", true);

    Utility::FolderFileWatcher folderFileWatcher(dirToWatch.GetPath());
    folderFileWatcher.start();

    TempFile tmpFile1(dirToWatch.GetPath(), "file1_", ".txt");
    WriteText(tmpFile1.GetPath());

    std::filesystem::path newTestDir = dirToWatch.GetPath();
    newTestDir /= "testDir";
    std::filesystem::create_directories(newTestDir);

    TempFile tmpFile2(newTestDir, "file2_", ".txt");
    WriteText(tmpFile2.GetPath());

    // Create files and delete them.
    std::filesystem::path tmpFile3Path;
    std::filesystem::path tmpFile4Path;
    {
        TempFile tmpFile3(dirToWatch.GetPath(), "file3_", ".txt");
        tmpFile3Path = tmpFile3.GetPath();
        WriteText(tmpFile3Path);

        TempFile tmpFile4(newTestDir, "file4_", ".txt");
        tmpFile4Path = tmpFile4.GetPath();
        WriteText(tmpFile4Path);
    }

    std::this_thread::sleep_for(100ms);
    folderFileWatcher.stop();

    auto& watchedFiles = folderFileWatcher.files();

    auto foundTmpFile1 = watchedFiles.find(tmpFile1.GetPath().string());
    REQUIRE(foundTmpFile1 != watchedFiles.cend());

    auto foundTmpFile2 = watchedFiles.find(tmpFile2.GetPath().string());
    REQUIRE(foundTmpFile2 != watchedFiles.cend());

    auto foundTmpFile3 = watchedFiles.find(tmpFile3Path.string());
    REQUIRE(foundTmpFile3 == watchedFiles.cend());

    auto foundTmpFile4 = watchedFiles.find(tmpFile4Path.string());
    REQUIRE(foundTmpFile4 == watchedFiles.cend());
}

TEST_CASE("FolderFileExtensionWatcher_CreateNewFiles", "[FolderFileWatcher]")
{
    TempDirectory dirToWatch("FolderFileExtensionWatcher_CreateNewFiles_", true);

    Utility::FolderFileExtensionWatcher folderFileExtensionWatcher(dirToWatch.GetPath(), ".yaml");
    folderFileExtensionWatcher.start();

    TempFile tmpFile1(dirToWatch.GetPath(), "file1_", ".txt");
    WriteText(tmpFile1.GetPath());

    TempFile tmpFile2(dirToWatch.GetPath(), "file2_", ".yaml");
    WriteText(tmpFile2.GetPath());

    std::filesystem::path newTestDir = dirToWatch.GetPath();
    newTestDir /= "testDir";
    std::filesystem::create_directories(newTestDir);

    TempFile tmpFile3(newTestDir, "file3_", ".txt");
    WriteText(tmpFile3.GetPath());

    TempFile tmpFile4(newTestDir, "file4_", ".yaml");
    WriteText(tmpFile4.GetPath());

    std::this_thread::sleep_for(100ms);
    folderFileExtensionWatcher.stop();

    auto& watchedFiles = folderFileExtensionWatcher.files();

    auto foundTmpFile1 = watchedFiles.find(tmpFile1.GetPath().string());
    REQUIRE(foundTmpFile1 == watchedFiles.cend());

    auto foundTmpFile2 = watchedFiles.find(tmpFile2.GetPath().string());
    REQUIRE(foundTmpFile2 != watchedFiles.cend());

    auto foundTmpFile3 = watchedFiles.find(tmpFile3.GetPath().string());
    REQUIRE(foundTmpFile3 == watchedFiles.cend());

    auto foundTmpFile4 = watchedFiles.find(tmpFile4.GetPath().string());
    REQUIRE(foundTmpFile4 != watchedFiles.cend());
}

TEST_CASE("FolderFileExtensionWatcher_CreateAfterStop", "[FolderFileWatcher]")
{
    TempDirectory dirToWatch("FolderFileExtensionWatcher_CreateAfterStop_", true);

    Utility::FolderFileExtensionWatcher folderFileExtensionWatcher(dirToWatch.GetPath(), ".txt");
    folderFileExtensionWatcher.start();

    TempFile tmpFile1(dirToWatch.GetPath(), "file1_", ".txt");
    WriteText(tmpFile1.GetPath());

    std::this_thread::sleep_for(100ms);
    folderFileExtensionWatcher.stop();

    TempFile tmpFile2(dirToWatch.GetPath(), "file2_", ".txt");
    WriteText(tmpFile2.GetPath());

    auto& watchedFiles = folderFileExtensionWatcher.files();

    auto foundTmpFile1 = watchedFiles.find(tmpFile1.GetPath().string());
    REQUIRE(foundTmpFile1 != watchedFiles.cend());

    auto foundTmpFile2 = watchedFiles.find(tmpFile2.GetPath().string());
    REQUIRE(foundTmpFile2 == watchedFiles.cend());
}

TEST_CASE("FolderFileExtensionWatcher_CreateNewFilesAndRename", "[FolderFileWatcher]")
{
    TempDirectory dirToWatch("FolderFileExtensionWatcher_CreateNewFilesAndRename_", true);

    Utility::FolderFileExtensionWatcher folderFileExtensionWatcher(dirToWatch.GetPath(), ".txt");
    folderFileExtensionWatcher.start();

    std::filesystem::path tmpFile1Path = dirToWatch.GetPath() / "file1.txt";
    TempFile tmpFile1(tmpFile1Path);
    WriteText(tmpFile1Path);

    std::filesystem::path newTestDir = dirToWatch.GetPath();
    newTestDir /= "testDir";
    std::filesystem::create_directories(newTestDir);

    std::filesystem::path tmpFile2Path = newTestDir / "file2.txt";
    TempFile tmpFile2(tmpFile2Path);
    WriteText(tmpFile2Path);

    std::filesystem::path tmpFile1PathRenamed = dirToWatch.GetPath() / "file1_renamed.txt";
    std::filesystem::path tmpFile2PathRenamed = newTestDir / "file2_renamed.txt";

    tmpFile1.Rename(tmpFile1PathRenamed);
    tmpFile2.Rename(tmpFile2PathRenamed);

    std::this_thread::sleep_for(100ms);
    folderFileExtensionWatcher.stop();

    auto& watchedFiles = folderFileExtensionWatcher.files();

    auto foundTmpFile1 = watchedFiles.find(tmpFile1Path.string());
    REQUIRE(foundTmpFile1 == watchedFiles.cend());

    auto foundTmpFile1Renamed = watchedFiles.find(tmpFile1PathRenamed.string());
    REQUIRE(foundTmpFile1Renamed != watchedFiles.cend());

    auto foundTmpFile2 = watchedFiles.find(tmpFile2Path.string());
    REQUIRE(foundTmpFile2 == watchedFiles.cend());

    auto foundTmpFile2Renamed = watchedFiles.find(tmpFile2PathRenamed.string());
    REQUIRE(foundTmpFile2Renamed != watchedFiles.cend());
}

TEST_CASE("FolderFileExtensionWatcher_CreateNewFilesAndDelete", "[FolderFileWatcher]")
{
    TempDirectory dirToWatch("FolderFileExtensionWatcher_CreateNewFilesAndDelete_", true);

    Utility::FolderFileExtensionWatcher folderFileExtensionWatcher(dirToWatch.GetPath(), ".txt");
    folderFileExtensionWatcher.start();

    TempFile tmpFile1(dirToWatch.GetPath(), "file1_", ".txt");
    WriteText(tmpFile1.GetPath());

    std::filesystem::path newTestDir = dirToWatch.GetPath();
    newTestDir /= "testDir";
    std::filesystem::create_directories(newTestDir);

    TempFile tmpFile2(newTestDir, "file2_", ".txt");
    WriteText(tmpFile2.GetPath());

    // Create files and delete them.
    std::filesystem::path tmpFile3Path;
    std::filesystem::path tmpFile4Path;
    {
        TempFile tmpFile3(dirToWatch.GetPath(), "file3_", ".txt");
        tmpFile3Path = tmpFile3.GetPath();
        WriteText(tmpFile3Path);

        TempFile tmpFile4(newTestDir, "file4_", ".txt");
        tmpFile4Path = tmpFile4.GetPath();
        WriteText(tmpFile4Path);
    }

    std::this_thread::sleep_for(100ms);
    folderFileExtensionWatcher.stop();

    auto& watchedFiles = folderFileExtensionWatcher.files();

    auto foundTmpFile1 = watchedFiles.find(tmpFile1.GetPath().string());
    REQUIRE(foundTmpFile1 != watchedFiles.cend());

    auto foundTmpFile2 = watchedFiles.find(tmpFile2.GetPath().string());
    REQUIRE(foundTmpFile2 != watchedFiles.cend());

    auto foundTmpFile3 = watchedFiles.find(tmpFile3Path.string());
    REQUIRE(foundTmpFile3 == watchedFiles.cend());

    auto foundTmpFile4 = watchedFiles.find(tmpFile4Path.string());
    REQUIRE(foundTmpFile4 == watchedFiles.cend());
}
