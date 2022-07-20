// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/Filesystem.h>

using namespace AppInstaller::Filesystem;
using namespace TestCommon;

TEST_CASE("PathEscapesDirectory", "[filesystem]")
{
    TestCommon::TempDirectory tempDirectory("TempDirectory");
    const std::filesystem::path& basePath = tempDirectory.GetPath();
    std::filesystem::path badPath = basePath / "../../badPath";
    std::filesystem::path goodPath = basePath / "goodPath";
    REQUIRE(PathEscapesBaseDirectory(badPath, basePath));
    REQUIRE_FALSE(PathEscapesBaseDirectory(goodPath, basePath));
}