// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/Fonts.h>

using namespace AppInstaller::Fonts;
using namespace TestCommon;

constexpr std::wstring_view s_testFontName = L"Times New Roman";
constexpr std::string_view s_FontFile = "TestFont.ttf";
constexpr std::string_view s_InvalidFontFile = "Installer-Good.msix";

TEST_CASE("GetInstalledFonts", "[fonts]")
{
    FontCatalog fontCatalog;
    std::vector<FontFamily> installedFontFamilies;
    REQUIRE_NOTHROW(installedFontFamilies = fontCatalog.GetInstalledFontFamilies());
    REQUIRE(installedFontFamilies.size() > 0);
}

TEST_CASE("GetSingleFontFamily", "[fonts]")
{
    FontCatalog fontCatalog;
    std::vector<FontFamily> fontFamily;
    REQUIRE_NOTHROW(fontFamily = fontCatalog.GetInstalledFontFamilies(std::wstring(s_testFontName)));
    REQUIRE_FALSE(fontFamily.empty());
    FontFamily singleFontFamily = fontFamily[0];
    REQUIRE(AppInstaller::Utility::CaseInsensitiveEquals(singleFontFamily.Name, s_testFontName));
    REQUIRE(singleFontFamily.Faces.size() > 0);
}

TEST_CASE("GetInvalidFontFamily", "[fonts]")
{
    FontCatalog fontCatalog;
    std::vector<FontFamily> fontFamily;
    REQUIRE_NOTHROW(fontFamily = fontCatalog.GetInstalledFontFamilies(L"Invalid Font"));
    REQUIRE(fontFamily.empty());
}

TEST_CASE("ValidFontFile", "[fonts]")
{
    TestDataFile testFont(s_FontFile);
    const auto& testFontPath = testFont.GetPath();

    FontCatalog fontCatalog;
    DWRITE_FONT_FILE_TYPE fontFileType;
    REQUIRE(fontCatalog.IsFontFileSupported(testFontPath, fontFileType));
    REQUIRE(fontFileType == DWRITE_FONT_FILE_TYPE::DWRITE_FONT_FILE_TYPE_TRUETYPE);
}

TEST_CASE("InvalidFontFile", "[fonts]")
{
    TestDataFile testFont(s_InvalidFontFile);
    const auto& testFontPath = testFont.GetPath();

    FontCatalog fontCatalog;
    DWRITE_FONT_FILE_TYPE fontFileType;
    REQUIRE_FALSE(fontCatalog.IsFontFileSupported(testFontPath, fontFileType));
    REQUIRE(fontFileType == DWRITE_FONT_FILE_TYPE::DWRITE_FONT_FILE_TYPE_UNKNOWN);
}
