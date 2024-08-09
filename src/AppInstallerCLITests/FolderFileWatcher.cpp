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

    std::filesystem::path RemoveRoot(const std::filesystem::path& prefix, const std::filesystem::path& source)
    {
        auto prefixItr = prefix.begin();
        auto sourceItr = source.begin();

        while (prefixItr != prefix.end() && sourceItr != source.end())
        {
            if (*prefixItr != *sourceItr)
            {
                break;
            }

            ++prefixItr;
            ++sourceItr;
        }

        std::filesystem::path result{};
        if (prefixItr == prefix.end())
        {
            for (; sourceItr != source.end(); ++sourceItr)
            {
                if (result.empty())
                {
                    result = *sourceItr;
                }
                else
                {
                    result /= *sourceItr;
                }
            }
        }

        return result;
    }
}

TEST_CASE("FolderFileWatcher_CreateNewFiles", "[FolderFileWatcher]")
{
    TempDirectory dirToWatch("FolderFileWatcher_CreateNewFiles_", true);

    Utility::FolderFileWatcher folderFileWatcher(dirToWatch.GetPath());
    folderFileWatcher.Start();

    TempFile tempFile1(dirToWatch.GetPath(), "file1_", ".txt");
    WriteText(tempFile1.GetPath());

    TempFile tempFile2(dirToWatch.GetPath(), "file2_", ".txt");
    WriteText(tempFile2.GetPath());
    
    std::filesystem::path newTestDir = dirToWatch.GetPath();
    newTestDir /= "testDir";
    std::filesystem::create_directories(newTestDir);

    TempFile tempFile3(newTestDir, "file3_", ".txt");
    WriteText(tempFile3.GetPath());

    std::this_thread::sleep_for(100ms);
    folderFileWatcher.Stop();

    auto& watchedFiles = folderFileWatcher.Files();

    auto tempFile1RelativePath = RemoveRoot(dirToWatch, tempFile1.GetPath());
    auto foundTempFile1 = watchedFiles.find(tempFile1RelativePath);
    REQUIRE(foundTempFile1 != watchedFiles.cend());

    auto tempFile2RelativePath = RemoveRoot(dirToWatch, tempFile2.GetPath());
    auto foundTempFile2 = watchedFiles.find(tempFile2RelativePath);
    REQUIRE(foundTempFile2 != watchedFiles.cend());

    auto tempFile3RelativePath = RemoveRoot(dirToWatch, tempFile3.GetPath());
    auto foundTempFile3 = watchedFiles.find(tempFile3RelativePath);
    REQUIRE(foundTempFile3 != watchedFiles.cend());
}

TEST_CASE("FolderFileWatcher_CreateAfterStop", "[FolderFileWatcher]")
{
    TempDirectory dirToWatch("FolderFileWatcher_CreateAfterStop_", true);

    Utility::FolderFileWatcher folderFileWatcher(dirToWatch.GetPath());
    folderFileWatcher.Start();

    TempFile tempFile1(dirToWatch.GetPath(), "file1_", ".txt");
    WriteText(tempFile1.GetPath());

    std::this_thread::sleep_for(100ms);
    folderFileWatcher.Stop();

    TempFile tempFile2(dirToWatch.GetPath(), "file2_", ".txt");
    WriteText(tempFile2.GetPath());

    auto& watchedFiles = folderFileWatcher.Files();

    auto tempFile1RelativePath = RemoveRoot(dirToWatch, tempFile1.GetPath());
    auto foundTempFile1 = watchedFiles.find(tempFile1RelativePath);
    REQUIRE(foundTempFile1 != watchedFiles.cend());

    auto tempFile2RelativePath = RemoveRoot(dirToWatch, tempFile2.GetPath());
    auto foundTempFile2 = watchedFiles.find(tempFile2RelativePath);
    REQUIRE(foundTempFile2 == watchedFiles.cend());
}

TEST_CASE("FolderFileWatcher_CreateNewFilesAndRename", "[FolderFileWatcher]")
{
    TempDirectory dirToWatch("FolderFileWatcher_CreateNewFilesAndRename_", true);

    Utility::FolderFileWatcher folderFileWatcher(dirToWatch.GetPath());
    folderFileWatcher.Start();

    std::filesystem::path tempFile1Path = dirToWatch.GetPath() / "file1.txt";
    TempFile tempFile1(tempFile1Path);
    WriteText(tempFile1Path);

    std::filesystem::path newTestDir = dirToWatch.GetPath();
    newTestDir /= "testDir";
    std::filesystem::create_directories(newTestDir);

    std::filesystem::path tempFile2Path = newTestDir / "file2.txt";
    TempFile tempFile2(tempFile2Path);
    WriteText(tempFile2Path);

    std::filesystem::path tempFile1PathRenamed = dirToWatch.GetPath() / "file1_renamed.txt";
    std::filesystem::path tempFile2PathRenamed = newTestDir / "file2_renamed.txt";

    tempFile1.Rename(tempFile1PathRenamed);
    tempFile2.Rename(tempFile2PathRenamed);

    std::this_thread::sleep_for(100ms);
    folderFileWatcher.Stop();

    auto& watchedFiles = folderFileWatcher.Files();

    auto tempFile1RelativePath = RemoveRoot(dirToWatch, tempFile1Path);
    auto foundTempFile1 = watchedFiles.find(tempFile1RelativePath);
    REQUIRE(foundTempFile1 == watchedFiles.cend());

    auto tempFile1RenamedRelativePath = RemoveRoot(dirToWatch, tempFile1PathRenamed);
    auto foundTempFile1Renamed = watchedFiles.find(tempFile1RenamedRelativePath);
    REQUIRE(foundTempFile1Renamed != watchedFiles.cend());

    auto tempFile2RelativePath = RemoveRoot(dirToWatch, tempFile2Path);
    auto foundTempFile2 = watchedFiles.find(tempFile2RelativePath);
    REQUIRE(foundTempFile2 == watchedFiles.cend());

    auto tempFile2RenamedRelativePath = RemoveRoot(dirToWatch, tempFile2PathRenamed);
    auto foundTempFile2Renamed = watchedFiles.find(tempFile2RenamedRelativePath);
    REQUIRE(foundTempFile2Renamed != watchedFiles.cend());
}

TEST_CASE("FolderFileWatcher_CreateNewFilesAndDelete", "[FolderFileWatcher]")
{
    TempDirectory dirToWatch("FolderFileWatcher_CreateNewFilesAndDelete_", true);

    Utility::FolderFileWatcher folderFileWatcher(dirToWatch.GetPath());
    folderFileWatcher.Start();

    TempFile tempFile1(dirToWatch.GetPath(), "file1_", ".txt");
    WriteText(tempFile1.GetPath());

    std::filesystem::path newTestDir = dirToWatch.GetPath();
    newTestDir /= "testDir";
    std::filesystem::create_directories(newTestDir);

    TempFile tempFile2(newTestDir, "file2_", ".txt");
    WriteText(tempFile2.GetPath());

    // Create files and delete them.
    std::filesystem::path tempFile3Path;
    std::filesystem::path tempFile4Path;
    {
        TempFile tempFile3(dirToWatch.GetPath(), "file3_", ".txt");
        tempFile3Path = tempFile3.GetPath();
        WriteText(tempFile3Path);
        std::filesystem::remove(tempFile3Path);

        TempFile tempFile4(newTestDir, "file4_", ".txt");
        tempFile4Path = tempFile4.GetPath();
        WriteText(tempFile4Path);
        std::filesystem::remove(tempFile4Path);
    }

    std::this_thread::sleep_for(100ms);
    folderFileWatcher.Stop();

    auto& watchedFiles = folderFileWatcher.Files();

    auto tempFile1RelativePath = RemoveRoot(dirToWatch, tempFile1.GetPath());
    auto foundTempFile1 = watchedFiles.find(tempFile1RelativePath);
    REQUIRE(foundTempFile1 != watchedFiles.cend());

    auto tempFile2RelativePath = RemoveRoot(dirToWatch, tempFile2.GetPath());
    auto foundTempFile2 = watchedFiles.find(tempFile2RelativePath);
    REQUIRE(foundTempFile2 != watchedFiles.cend());

    auto tempFile3RelativePath = RemoveRoot(dirToWatch, tempFile3Path);
    auto foundTempFile3 = watchedFiles.find(tempFile3RelativePath);
    REQUIRE(foundTempFile3 == watchedFiles.cend());

    auto tempFile4RelativePath = RemoveRoot(dirToWatch, tempFile4Path);
    auto foundTempFile4 = watchedFiles.find(tempFile4RelativePath);
    REQUIRE(foundTempFile4 == watchedFiles.cend());
}

TEST_CASE("FolderFileWatcher_Extension_CreateNewFiles", "[FolderFileWatcher]")
{
    TempDirectory dirToWatch("FolderFileWatcher_Extension_CreateNewFiles", true);

    Utility::FolderFileWatcher folderFileExtensionWatcher(dirToWatch.GetPath(), ".yaml");
    folderFileExtensionWatcher.Start();

    TempFile tempFile1(dirToWatch.GetPath(), "file1_", ".txt");
    WriteText(tempFile1.GetPath());

    TempFile tempFile2(dirToWatch.GetPath(), "file2_", ".yaml");
    WriteText(tempFile2.GetPath());

    std::filesystem::path newTestDir = dirToWatch.GetPath();
    newTestDir /= "testDir";
    std::filesystem::create_directories(newTestDir);

    TempFile tempFile3(newTestDir, "file3_", ".txt");
    WriteText(tempFile3.GetPath());

    TempFile tempFile4(newTestDir, "file4_", ".yaml");
    WriteText(tempFile4.GetPath());

    std::this_thread::sleep_for(100ms);
    folderFileExtensionWatcher.Stop();

    auto& watchedFiles = folderFileExtensionWatcher.Files();

    auto tempFile1RelativePath = RemoveRoot(dirToWatch, tempFile1.GetPath());
    auto foundTempFile1 = watchedFiles.find(tempFile1RelativePath);
    REQUIRE(foundTempFile1 == watchedFiles.cend());

    auto tempFile2RelativePath = RemoveRoot(dirToWatch, tempFile2.GetPath());
    auto foundTempFile2 = watchedFiles.find(tempFile2RelativePath);
    REQUIRE(foundTempFile2 != watchedFiles.cend());

    auto tempFile3RelativePath = RemoveRoot(dirToWatch, tempFile3.GetPath());
    auto foundTempFile3 = watchedFiles.find(tempFile3RelativePath);
    REQUIRE(foundTempFile3 == watchedFiles.cend());

    auto tempFile4RelativePath = RemoveRoot(dirToWatch, tempFile4.GetPath());
    auto foundTempFile4 = watchedFiles.find(tempFile4RelativePath);
    REQUIRE(foundTempFile4 != watchedFiles.cend());
}

TEST_CASE("FolderFileWatcher_Extension_CreateAfterStop", "[FolderFileWatcher]")
{
    TempDirectory dirToWatch("FolderFileWatcher_Extension_CreateAfterStop", true);

    Utility::FolderFileWatcher folderFileExtensionWatcher(dirToWatch.GetPath(), ".txt");
    folderFileExtensionWatcher.Start();

    TempFile tempFile1(dirToWatch.GetPath(), "file1_", ".txt");
    WriteText(tempFile1.GetPath());

    std::this_thread::sleep_for(100ms);
    folderFileExtensionWatcher.Stop();

    TempFile tempFile2(dirToWatch.GetPath(), "file2_", ".txt");
    WriteText(tempFile2.GetPath());

    auto& watchedFiles = folderFileExtensionWatcher.Files();

    auto tempFile1RelativePath = RemoveRoot(dirToWatch, tempFile1.GetPath());
    auto foundTempFile1 = watchedFiles.find(tempFile1RelativePath);
    REQUIRE(foundTempFile1 != watchedFiles.cend());

    auto tempFile2RelativePath = RemoveRoot(dirToWatch, tempFile2.GetPath());
    auto foundTempFile2 = watchedFiles.find(tempFile2RelativePath);
    REQUIRE(foundTempFile2 == watchedFiles.cend());
}

TEST_CASE("FolderFileWatcher_Extension_CreateNewFilesAndRename", "[FolderFileWatcher]")
{
    TempDirectory dirToWatch("FolderFileWatcher_Extension_CreateNewFilesAndRename", true);

    Utility::FolderFileWatcher folderFileExtensionWatcher(dirToWatch.GetPath(), ".txt");
    folderFileExtensionWatcher.Start();

    std::filesystem::path tempFile1Path = dirToWatch.GetPath() / "file1.txt";
    TempFile tempFile1(tempFile1Path);
    WriteText(tempFile1Path);

    std::filesystem::path newTestDir = dirToWatch.GetPath();
    newTestDir /= "testDir";
    std::filesystem::create_directories(newTestDir);

    std::filesystem::path tempFile2Path = newTestDir / "file2.txt";
    TempFile tempFile2(tempFile2Path);
    WriteText(tempFile2Path);

    std::filesystem::path tempFile1PathRenamed = dirToWatch.GetPath() / "file1_renamed.txt";
    std::filesystem::path tempFile2PathRenamed = newTestDir / "file2_renamed.txt";

    tempFile1.Rename(tempFile1PathRenamed);
    tempFile2.Rename(tempFile2PathRenamed);

    std::this_thread::sleep_for(100ms);
    folderFileExtensionWatcher.Stop();

    auto& watchedFiles = folderFileExtensionWatcher.Files();

    auto tempFile1RelativePath = RemoveRoot(dirToWatch, tempFile1Path);
    auto foundTempFile1 = watchedFiles.find(tempFile1RelativePath);
    REQUIRE(foundTempFile1 == watchedFiles.cend());

    auto tempFile1RenamedRelativePath = RemoveRoot(dirToWatch, tempFile1PathRenamed);
    auto foundTempFile1Renamed = watchedFiles.find(tempFile1RenamedRelativePath);
    REQUIRE(foundTempFile1Renamed != watchedFiles.cend());

    auto tempFile2RelativePath = RemoveRoot(dirToWatch, tempFile2Path);
    auto foundTempFile2 = watchedFiles.find(tempFile2RelativePath);
    REQUIRE(foundTempFile2 == watchedFiles.cend());

    auto tempFile2RenamedRelativePath = RemoveRoot(dirToWatch, tempFile2PathRenamed);
    auto foundTempFile2Renamed = watchedFiles.find(tempFile2RenamedRelativePath);
    REQUIRE(foundTempFile2Renamed != watchedFiles.cend());
}

TEST_CASE("FolderFileWatcher_Extension_CreateNewFilesAndDelete", "[FolderFileWatcher]")
{
    TempDirectory dirToWatch("FolderFileWatcher_Extension_CreateNewFilesAndDelete", true);

    Utility::FolderFileWatcher folderFileExtensionWatcher(dirToWatch.GetPath(), ".txt");
    folderFileExtensionWatcher.Start();

    TempFile tempFile1(dirToWatch.GetPath(), "file1_", ".txt");
    WriteText(tempFile1.GetPath());

    std::filesystem::path newTestDir = dirToWatch.GetPath();
    newTestDir /= "testDir";
    std::filesystem::create_directories(newTestDir);

    TempFile tempFile2(newTestDir, "file2_", ".txt");
    WriteText(tempFile2.GetPath());

    // Create files and delete them.
    std::filesystem::path tempFile3Path;
    std::filesystem::path tempFile4Path;
    {
        TempFile tempFile3(dirToWatch.GetPath(), "file3_", ".txt");
        tempFile3Path = tempFile3.GetPath();
        WriteText(tempFile3Path);
        std::filesystem::remove(tempFile3Path);

        TempFile tempFile4(newTestDir, "file4_", ".txt");
        tempFile4Path = tempFile4.GetPath();
        WriteText(tempFile4Path);
        std::filesystem::remove(tempFile4Path);
    }

    std::this_thread::sleep_for(100ms);
    folderFileExtensionWatcher.Stop();

    auto& watchedFiles = folderFileExtensionWatcher.Files();

    auto tempFile1RelativePath = RemoveRoot(dirToWatch, tempFile1.GetPath());
    auto foundTempFile1 = watchedFiles.find(tempFile1RelativePath);
    REQUIRE(foundTempFile1 != watchedFiles.cend());

    auto tempFile2RelativePath = RemoveRoot(dirToWatch, tempFile2.GetPath());
    auto foundTempFile2 = watchedFiles.find(tempFile2RelativePath);
    REQUIRE(foundTempFile2 != watchedFiles.cend());

    auto tempFile3RelativePath = RemoveRoot(dirToWatch, tempFile3Path);
    auto foundTempFile3 = watchedFiles.find(tempFile3RelativePath);
    REQUIRE(foundTempFile3 == watchedFiles.cend());

    auto tempFile4RelativePath = RemoveRoot(dirToWatch, tempFile4Path);
    auto foundTempFile4 = watchedFiles.find(tempFile4RelativePath);
    REQUIRE(foundTempFile4 == watchedFiles.cend());
}
