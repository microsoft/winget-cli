// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/PathVariable.h>
#include <winget/PortableEntry.h>
#include <winget/PortableARPEntry.h>
#include <Public/AppInstallerArchitecture.h>

using namespace AppInstaller::Portable;
using namespace AppInstaller::Utility;
using namespace TestCommon;

TEST_CASE("VerifyPortableMove", "[PortableEntry]")
{
    PortableARPEntry testARPEntry = PortableARPEntry(
        AppInstaller::Manifest::ScopeEnum::User,
        Architecture::X64,
        "testProductCode");

    PortableEntry testEntry = PortableEntry(testARPEntry);
    TestCommon::TempDirectory tempDirectory("TempDirectory", false);
    testEntry.InstallLocation = tempDirectory.GetPath();
    testEntry.PortableTargetFullPath = tempDirectory.GetPath() / "output.txt";

    TestCommon::TempFile testFile("input.txt");
    std::ofstream file(testFile.GetPath(), std::ofstream::out);
    file.close();

    testEntry.MovePortableExe(testFile.GetPath());
    REQUIRE(std::filesystem::exists(testEntry.PortableTargetFullPath));
    REQUIRE(testEntry.InstallDirectoryCreated);

    // Create a second PortableEntry instance to emulate installing for a second time. (ARP entry should already exist)
    PortableARPEntry testARPEntry2 = PortableARPEntry(
        AppInstaller::Manifest::ScopeEnum::User,
        Architecture::X64,
        "testProductCode");

    PortableEntry testEntry2 = PortableEntry(testARPEntry2);
    REQUIRE(testEntry2.InstallDirectoryCreated); // InstallDirectoryCreated should already be initialized as true.

    testEntry2.InstallLocation = tempDirectory.GetPath();
    testEntry2.PortableTargetFullPath = tempDirectory.GetPath() / "output2.txt";

    TestCommon::TempFile testFile2("input2.txt");
    std::ofstream file2(testFile2, std::ofstream::out);
    file2.close();

    testEntry2.MovePortableExe(testFile2.GetPath());
    REQUIRE(std::filesystem::exists(testEntry2.PortableTargetFullPath));
    // InstallDirectoryCreated value should be preserved even though the directory was not created; 
    REQUIRE(testEntry2.InstallDirectoryCreated);
    testEntry2.RemoveARPEntry();
}

TEST_CASE("VerifySymlinkCheck", "[PortableEntry]")
{
    PortableARPEntry testARPEntry = PortableARPEntry(
        AppInstaller::Manifest::ScopeEnum::User,
        Architecture::X64,
        "testProductCode");

    PortableEntry testEntry = PortableEntry(testARPEntry);

    TestCommon::TempFile testFile("target.txt");
    std::ofstream file(testFile.GetPath(), std::ofstream::out);
    file.close();

    TestCommon::TempDirectory tempDirectory("TempDirectory", true);
    testEntry.PortableTargetFullPath = testFile.GetPath();
    testEntry.PortableSymlinkFullPath = tempDirectory.GetPath() / "symlink.exe";

    testEntry.CreatePortableSymlink();

    REQUIRE(testEntry.VerifySymlinkTarget());

    // Modify with incorrect target full path.
    testEntry.PortableTargetFullPath = tempDirectory.GetPath() / "invalidTarget.txt";
    REQUIRE_FALSE(testEntry.VerifySymlinkTarget());
    testEntry.RemoveARPEntry();
}

TEST_CASE("VerifyPathVariableModified", "[PortableEntry]")
{
    PortableARPEntry testARPEntry = PortableARPEntry(
        AppInstaller::Manifest::ScopeEnum::User,
        Architecture::X64,
        "testProductCode");

    PortableEntry testEntry = PortableEntry(testARPEntry);
    testEntry.InstallDirectoryAddedToPath = true;
    TestCommon::TempDirectory tempDirectory("TempDirectory", false);
    const std::filesystem::path& pathValue = tempDirectory.GetPath();
    testEntry.InstallLocation = pathValue;
    testEntry.AddToPathVariable();

    AppInstaller::Registry::Environment::PathVariable pathVariable(AppInstaller::Manifest::ScopeEnum::User);
    REQUIRE(pathVariable.Contains(pathValue));

    testEntry.RemoveFromPathVariable();
    REQUIRE_FALSE(pathVariable.Contains(pathValue));
    testEntry.RemoveARPEntry();
}