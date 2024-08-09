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
