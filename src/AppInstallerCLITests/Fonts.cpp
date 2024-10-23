// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/Fonts.h>

using namespace AppInstaller::Fonts;
using namespace TestCommon;

constexpr std::wstring_view s_testFontName = L"Times New Roman";

TEST_CASE("GetInstalledFonts", "[fonts]")
{
    std::vector<FontFamily> installedFontFamilies;
    REQUIRE_NOTHROW(installedFontFamilies = GetInstalledFontFamilies());
    REQUIRE(installedFontFamilies.size() > 0);
}

TEST_CASE("GetSingleFontFamily", "[fonts]")
{
    std::vector<FontFamily> fontFamily;
    REQUIRE_NOTHROW(fontFamily = GetInstalledFontFamilies(std::wstring(s_testFontName)));
    REQUIRE_FALSE(fontFamily.empty());
    FontFamily singleFontFamily = fontFamily[0];
    REQUIRE(AppInstaller::Utility::CaseInsensitiveEquals(singleFontFamily.Name, s_testFontName));
    REQUIRE(singleFontFamily.Faces.size() > 0);
}

TEST_CASE("GetInvalidFontFamily", "[fonts]")
{
    std::vector<FontFamily> fontFamily;
    REQUIRE_NOTHROW(fontFamily = GetInstalledFontFamilies(L"Invalid Font"));
    REQUIRE(fontFamily.empty());
}
