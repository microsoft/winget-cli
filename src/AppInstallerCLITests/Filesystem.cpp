// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/Filesystem.h>
#include <AppInstallerStrings.h>

using namespace AppInstaller::Utility;
using namespace AppInstaller::Filesystem;
using namespace TestCommon;

TEST_CASE("PathEscapesDirectory", "[filesystem]")
{
    TestCommon::TempDirectory tempDirectory("TempDirectory");
    const std::filesystem::path& basePath = tempDirectory.GetPath();

    std::string badRelativePath = "../../target.exe";
    std::string badRelativePath2 = "test/../../target.exe";
    std::string goodRelativePath = "target.exe";
    std::string goodRelativePath2 = "test/../test1/target.exe";

    std::filesystem::path badPath = basePath / badRelativePath;
    std::filesystem::path badPath2 = basePath / badRelativePath2;
    std::filesystem::path goodPath = basePath / goodRelativePath;
    std::filesystem::path goodPath2 = basePath / goodRelativePath2;

    REQUIRE(PathEscapesBaseDirectory(badPath, basePath));
    REQUIRE(PathEscapesBaseDirectory(badPath2, basePath));
    REQUIRE_FALSE(PathEscapesBaseDirectory(goodPath, basePath));
    REQUIRE_FALSE(PathEscapesBaseDirectory(goodPath2, basePath));
}

TEST_CASE("VerifySymlink", "[filesystem]")
{
    TestCommon::TempDirectory tempDirectory("TempDirectory");
    const std::filesystem::path& basePath = tempDirectory.GetPath();

    std::filesystem::path testFilePath = basePath / "testFile.txt";
    std::filesystem::path symlinkPath = basePath / "symlink.exe";

    TestCommon::TempFile testFile(testFilePath);
    std::ofstream file2(testFile, std::ofstream::out);
    file2.close();

    std::filesystem::create_symlink(testFile.GetPath(), symlinkPath);

    REQUIRE(SymlinkExists(symlinkPath));
    REQUIRE(VerifySymlink(symlinkPath, testFilePath));
    REQUIRE_FALSE(VerifySymlink(symlinkPath, "badPath"));

    std::filesystem::remove(testFilePath);

    // Ensure that symlink existence does not check the target
    REQUIRE(SymlinkExists(symlinkPath));

    std::filesystem::remove(symlinkPath);

    REQUIRE_FALSE(SymlinkExists(symlinkPath));
}

TEST_CASE("VerifyIsSameVolume", "[filesystem]")
{
    // Note: Pipeline build machine uses 'D:\' as the volume.
    std::filesystem::path path1 = L"C:\\Program Files\\WinGet\\Packages";
    std::filesystem::path path2 = L"c:\\Users\\testUser\\AppData\\Local\\Microsoft\\WinGet\\Packages";
    std::filesystem::path path3 = L"localPath\\test\\folder";
    std::filesystem::path path4 = L"test\\folder";
    std::filesystem::path path5 = L"D:\\test\\folder";
    std::filesystem::path path6 = L"F:\\test\\folder";
    std::filesystem::path path7 = L"d:\\randomFolder";
    std::filesystem::path path8 = L"f:\\randomFolder";
    std::filesystem::path path9 = L"a";
    std::filesystem::path path10 = L"b";

    REQUIRE(IsSameVolume(path1, path2));
    if (IsSameVolume(path5, path5))
    {
        REQUIRE(IsSameVolume(path5, path7));
    }
    REQUIRE(IsSameVolume(path3, path4));
    REQUIRE(IsSameVolume(path9, path10));

    REQUIRE_FALSE(IsSameVolume(path1, path5));
    REQUIRE_FALSE(IsSameVolume(path1, path6));
    REQUIRE_FALSE(IsSameVolume(path2, path5));
    REQUIRE_FALSE(IsSameVolume(path2, path6));
    REQUIRE_FALSE(IsSameVolume(path3, path6));
    REQUIRE_FALSE(IsSameVolume(path5, path6));
    REQUIRE_FALSE(IsSameVolume(path4, path6));
    REQUIRE_FALSE(IsSameVolume(path6, path8));
}

TEST_CASE("ReplaceCommonPathPrefix", "[filesystem]")
{
    std::filesystem::path prefix = "C:\\test1\\test2";
    std::string replacement = "%TEST%";

    std::filesystem::path shouldReplace = "C:\\test1\\test2\\subdir1\\subdir2";
    REQUIRE(ReplaceCommonPathPrefix(shouldReplace, prefix, replacement));
    REQUIRE(shouldReplace.u8string() == (replacement + "\\subdir1\\subdir2"));

    std::filesystem::path shouldNotReplace = "C:\\test1\\test3\\subdir1\\subdir2";
    REQUIRE(!ReplaceCommonPathPrefix(shouldNotReplace, prefix, replacement));
    REQUIRE(shouldNotReplace.u8string() == "C:\\test1\\test3\\subdir1\\subdir2");
}

TEST_CASE("GetExecutablePathForProcess", "[filesystem]")
{
    std::filesystem::path thisExecutable = GetExecutablePathForProcess(GetCurrentProcess());
    REQUIRE(!thisExecutable.empty());
    REQUIRE(thisExecutable.is_absolute());
    REQUIRE(thisExecutable.has_filename());
    REQUIRE(thisExecutable.has_extension());
    REQUIRE(thisExecutable.filename() == L"AppInstallerCLITests.exe");
}

TEST_CASE("GetFileInfoFor", "[filesystem]")
{
    TestCommon::TempDirectory tempDirectory{ "GetFileInfoFor" };

    auto now = std::filesystem::file_time_type::clock::now();

    std::this_thread::sleep_for(1s);
    auto file1 = tempDirectory.CreateTempFile("c.txt");
    std::string file1Content = "File 1 Content!";
    std::ofstream{ file1 } << file1Content;
    std::this_thread::sleep_for(1s);
    auto file2 = tempDirectory.CreateTempFile("b.txt");
    std::string file2Content = "More Content! Better Content!";
    std::ofstream{ file2 } << file2Content;
    std::this_thread::sleep_for(1s);
    auto file3 = tempDirectory.CreateTempFile("a.txt");
    std::string file3Content = "Maybe less is better?";
    std::ofstream{ file3 } << file3Content;

    auto fileInfo = GetFileInfoFor(tempDirectory);
    REQUIRE(3 == fileInfo.size());

    // Sort with oldest first
    std::sort(fileInfo.begin(), fileInfo.end(), [](const FileInfo& a, const FileInfo& b) { return a.LastWriteTime < b.LastWriteTime; });

    REQUIRE(fileInfo[0].Path == file1.GetPath());
    REQUIRE(fileInfo[0].LastWriteTime > now);
    REQUIRE(fileInfo[0].Size == file1Content.size());

    REQUIRE(fileInfo[1].Path == file2.GetPath());
    REQUIRE(fileInfo[1].LastWriteTime > fileInfo[0].LastWriteTime);
    REQUIRE(fileInfo[1].Size == file2Content.size());

    REQUIRE(fileInfo[2].Path == file3.GetPath());
    REQUIRE(fileInfo[2].LastWriteTime > fileInfo[1].LastWriteTime);
    REQUIRE(fileInfo[2].Size == file3Content.size());
}

void RequireFilePaths(const std::vector<FileInfo>& files, std::initializer_list<const char*> paths)
{
    REQUIRE(paths.size() == files.size());
    size_t i = 0;
    for (const char* val : paths)
    {
        REQUIRE(val == files[i++].Path.u8string());
    }
}

TEST_CASE("FilterToFilesExceedingLimits", "[filesystem]")
{
    auto now = std::filesystem::file_time_type::clock::now();

    std::vector<FileInfo> files
    {
        { "a", now, 42 },
        { "b", now - 32min, 45321 },
        { "c", now - 84min, 24567 },
        { "d", now - 4h, 876312 },
        { "e", now - 18h, 2908534 },
        { "f", now - 47h, 312 },
        { "g", now - 132h, 74321 },
        { "h", now - 4567h, 6573423 },
    };

    // Give the sort inside FilterToFilesExceedingLimits something to do
    std::shuffle(files.begin(), files.end(), std::mt19937{});
    FileLimits limits{};

    SECTION("No limits")
    {
        FilterToFilesExceedingLimits(files, limits);
        // Without limits, nothing exceeds them
        REQUIRE(files.empty());
    }
    SECTION("Age - 1h")
    {
        limits.Age = 1h;
        FilterToFilesExceedingLimits(files, limits);
        RequireFilePaths(files, { "h", "g", "f", "e", "d", "c" });
    }
    SECTION("Age - 2h")
    {
        limits.Age = 2h;
        FilterToFilesExceedingLimits(files, limits);
        RequireFilePaths(files, { "h", "g", "f", "e", "d" });
    }
    SECTION("Age - 24h")
    {
        limits.Age = 24h;
        FilterToFilesExceedingLimits(files, limits);
        RequireFilePaths(files, { "h", "g", "f" });
    }
    SECTION("Age - 7d")
    {
        limits.Age = 7 * 24h;
        FilterToFilesExceedingLimits(files, limits);
        RequireFilePaths(files, { "h" });
    }
    SECTION("Age - 365d")
    {
        limits.Age = 365 * 24h;
        FilterToFilesExceedingLimits(files, limits);
        REQUIRE(files.empty());
    }
    SECTION("Size - 1MB")
    {
        limits.TotalSizeInMB = 1;
        FilterToFilesExceedingLimits(files, limits);
        RequireFilePaths(files, { "h", "g", "f", "e" });
    }
    SECTION("Size - 2MB")
    {
        limits.TotalSizeInMB = 2;
        FilterToFilesExceedingLimits(files, limits);
        RequireFilePaths(files, { "h", "g", "f", "e" });
    }
    SECTION("Size - 3MB")
    {
        limits.TotalSizeInMB = 3;
        FilterToFilesExceedingLimits(files, limits);
        RequireFilePaths(files, { "h", "g", "f", "e" });
    }
    SECTION("Size - 4MB")
    {
        limits.TotalSizeInMB = 4;
        FilterToFilesExceedingLimits(files, limits);
        RequireFilePaths(files, { "h" });
    }
    SECTION("Size - 100MB")
    {
        limits.TotalSizeInMB = 100;
        FilterToFilesExceedingLimits(files, limits);
        REQUIRE(files.empty());
    }
    SECTION("Count - 1")
    {
        limits.Count = 1;
        FilterToFilesExceedingLimits(files, limits);
        RequireFilePaths(files, { "h", "g", "f", "e", "d", "c", "b" });
    }
    SECTION("Count - 2")
    {
        limits.Count = 2;
        FilterToFilesExceedingLimits(files, limits);
        RequireFilePaths(files, { "h", "g", "f", "e", "d", "c" });
    }
    SECTION("Count - 4")
    {
        limits.Count = 4;
        FilterToFilesExceedingLimits(files, limits);
        RequireFilePaths(files, { "h", "g", "f", "e" });
    }
    SECTION("Count - 7")
    {
        limits.Count = 7;
        FilterToFilesExceedingLimits(files, limits);
        RequireFilePaths(files, { "h" });
    }
    SECTION("Count - 8")
    {
        limits.Count = 8;
        FilterToFilesExceedingLimits(files, limits);
        REQUIRE(files.empty());
    }
    SECTION("Count - 100")
    {
        limits.Count = 100;
        FilterToFilesExceedingLimits(files, limits);
        REQUIRE(files.empty());
    }
    SECTION("Mix - 24h - 2MB - 4")
    {
        limits.Age = 24h;
        limits.TotalSizeInMB = 2;
        limits.Count = 4;
        FilterToFilesExceedingLimits(files, limits);
        RequireFilePaths(files, { "h", "g", "f", "e" });
    }
    SECTION("Mix - 2h - 2MB - 4")
    {
        limits.Age = 2h;
        limits.TotalSizeInMB = 2;
        limits.Count = 4;
        FilterToFilesExceedingLimits(files, limits);
        RequireFilePaths(files, { "h", "g", "f", "e", "d" });
    }
    SECTION("Mix - 24h - 1MB - 4")
    {
        limits.Age = 24h;
        limits.TotalSizeInMB = 1;
        limits.Count = 4;
        FilterToFilesExceedingLimits(files, limits);
        RequireFilePaths(files, { "h", "g", "f", "e" });
    }
    SECTION("Mix - 24h - 2MB - 2")
    {
        limits.Age = 24h;
        limits.TotalSizeInMB = 2;
        limits.Count = 2;
        FilterToFilesExceedingLimits(files, limits);
        RequireFilePaths(files, { "h", "g", "f", "e", "d", "c" });
    }
}
