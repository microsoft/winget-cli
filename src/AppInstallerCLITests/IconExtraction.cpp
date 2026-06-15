// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/IconExtraction.h>
#include <AppInstallerStrings.h>

using namespace AppInstaller::Repository;

TEST_CASE("ExtractIconFromBinaryFile", "IconExtraction")
{
    auto extracted = ExtractIconFromBinaryFile(TestCommon::TestDataFile{ "notepad.exe" }.GetPath());

    std::ifstream expectedIconFile{ TestCommon::TestDataFile{ "notepad.ico" }.GetPath(), std::ios::in | std::ios::binary};
    auto expected = AppInstaller::Utility::ReadEntireStreamAsByteArray(expectedIconFile);

    REQUIRE(expected == extracted);
}