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
    std::optional<FontFamily> fontFamily;
    REQUIRE_NOTHROW(fontFamily = GetInstalledFontFamily(std::wstring(s_testFontName)));
    REQUIRE(fontFamily.has_value());
    REQUIRE(AppInstaller::Utility::CaseInsensitiveEquals(fontFamily->Name, s_testFontName));
    REQUIRE(fontFamily->Faces.size() > 0);
}

TEST_CASE("GetInvalidFontFamily", "[fonts]")
{
    std::optional<FontFamily> fontFamily;
    REQUIRE_NOTHROW(fontFamily = GetInstalledFontFamily(L"Invalid Font"));
    REQUIRE_FALSE(fontFamily.has_value());
}
