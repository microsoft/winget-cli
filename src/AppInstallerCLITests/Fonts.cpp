// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerRuntime.h>
#include <winget/Fonts.h>

using namespace AppInstaller::Fonts;
using namespace AppInstaller::Manifest;
using namespace TestCommon;

constexpr std::wstring_view s_testFontName = L"Times New Roman";
constexpr std::string_view s_FontFile = "TestFont.ttf";
constexpr std::string_view s_FontFileTemp = "TestFont_temp.ttf";
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

TEST_CASE("GetFontRegistryPath", "[fonts]")
{
    auto context = FontContext();
    context.PackageName = L"TestPackage";

    context.InstallerSource = InstallerSource::WinGet;
    auto fontPath = GetFontRegistryPath(context);
    REQUIRE(fontPath == L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts\\winget_v1\\TestPackage");

    context.InstallerSource = InstallerSource::UWP;
    fontPath = GetFontRegistryPath(context);
    REQUIRE(fontPath == L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts\\TestPackage");

    context.InstallerSource = InstallerSource::Unknown;
    fontPath = GetFontRegistryPath(context);
    REQUIRE(fontPath == L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts");
}

TEST_CASE("GetInstalledFontFiles", "[fonts]")
{
    const auto& fontFiles = GetInstalledFontFiles();
    REQUIRE(fontFiles.size() > 0);
}

TEST_CASE("InstallInvalidFontFile", "[fonts]")
{
    TestDataFile testFont(s_InvalidFontFile);

    auto context = FontContext();
    context.FilePath = testFont.GetPath();
    context.PackageName = L"TestPackage";
    context.InstallerSource = InstallerSource::WinGet;
    context.Scope = ScopeEnum::User;

    const auto& result = InstallFontFile(context, true);

    REQUIRE(result.HResult == APPINSTALLER_CLI_ERROR_FONT_FILE_NOT_SUPPORTED);
}

TEST_CASE("RemoveFontPackage", "[fonts]")
{
    // Calling remove should always be successful when font doesn't exist.
    auto context = FontContext();
    context.PackageName = L"TestPackage";
    context.InstallerSource = InstallerSource::WinGet;
    context.Scope = ScopeEnum::User;

    const auto& result = UninstallFontPackage(context);

    REQUIRE(result.HResult == S_OK);
}

TEST_CASE("InstallValidFontFile", "[fonts]")
{
    // The test will move the file, so for idempotency we need
    // to use a file that can be replaced.
    TestDataFile testFontBase(s_FontFile);
    TestCommon::TempDirectory tempDirectory("TempDirectory");
    std::filesystem::path testFontCopyPath = tempDirectory.GetPath() / s_FontFileTemp;
    if (!std::filesystem::exists(testFontCopyPath))
    {
        std::filesystem::copy(testFontBase.GetPath(), testFontCopyPath);
    }

    // Use the copied font for the test
    TestDataFile testFont(testFontCopyPath);

    auto context = FontContext();
    context.FilePath = testFont.GetPath();
    context.PackageName = L"TestPackage";
    context.InstallerSource = InstallerSource::WinGet;
    context.Scope = ScopeEnum::User;

    auto result = InstallFontFile(context, true, true);
    REQUIRE(result.HResult == S_OK);

    result = UninstallFontPackage(context);
    REQUIRE(result.HResult == S_OK);
}

TEST_CASE("RemoveFontPackageMachine", "[fonts]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    // Calling remove should always be successful when font doesn't exist.
    auto context = FontContext();
    context.PackageName = L"TestPackage";
    context.InstallerSource = InstallerSource::WinGet;
    context.Scope = ScopeEnum::Machine;

    const auto& result = UninstallFontPackage(context);

    REQUIRE(result.HResult == S_OK);
}

TEST_CASE("InstallValidFontFileMachine", "[fonts]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    // The test will move the file, so for idempotency we need
    // to use a file that can be replaced.
    TestDataFile testFontBase(s_FontFile);
    TestCommon::TempDirectory tempDirectory("TempDirectory");
    std::filesystem::path testFontCopyPath = tempDirectory.GetPath() / s_FontFileTemp;
    if (!std::filesystem::exists(testFontCopyPath))
    {
        std::filesystem::copy(testFontBase.GetPath(), testFontCopyPath);
    }

    // Use the copied font for the test
    TestDataFile testFont(testFontCopyPath);

    auto context = FontContext();
    context.FilePath = testFont.GetPath();
    context.PackageName = L"TestPackage";
    context.InstallerSource = InstallerSource::WinGet;
    context.Scope = ScopeEnum::Machine;

    auto result = InstallFontFile(context, true, true);
    REQUIRE(result.HResult == S_OK);

    result = UninstallFontPackage(context);
    REQUIRE(result.HResult == S_OK);
}

