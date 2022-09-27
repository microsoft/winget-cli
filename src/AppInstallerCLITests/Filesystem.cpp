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