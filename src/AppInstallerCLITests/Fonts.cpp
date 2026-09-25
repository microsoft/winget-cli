// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerErrors.h>
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

TEST_CASE("GetFontFileInfo", "[fonts]")
{
    TestDataFile testFont(s_FontFile);
    auto context = FontContext();
    context.Scope = ScopeEnum::User;
    context.PackageId = L"TestPackage";
    context.PackageVersion = L"1.0.0.0";
    context.InstallerSource = InstallerSource::WinGet;
    const auto& fontFileInfo = CreateFontFileInfo(context, testFont.GetPath());
    REQUIRE(fontFileInfo.Status == FontStatus::Absent);
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

TEST_CASE("GetInstalledFontFiles", "[fonts]")
{
    const auto& fontFiles = GetInstalledFontFiles();

    // There should be at least one font installed on the system.
    REQUIRE(fontFiles.size() > 0);
}

TEST_CASE("ValidateInvalidFontPackage", "[fonts]")
{
    TestDataFile testFont(s_InvalidFontFile);
    auto context = FontContext();
    context.Scope = ScopeEnum::User;
    context.PackageId = L"TestPackage";
    context.PackageVersion = L"1.0.0.0";
    context.InstallerSource = InstallerSource::WinGet;
    context.AddPackageFile(testFont.GetPath());
    const auto& fontValidationResult = ValidateFontPackage(context);
    REQUIRE(fontValidationResult.HResult == S_OK);
    REQUIRE(fontValidationResult.Result == FontResult::Success);
    REQUIRE(fontValidationResult.HasUnsupportedFonts == true);
    REQUIRE(fontValidationResult.Status == FontStatus::Absent);
}

TEST_CASE("ValidateValidFontPackage", "[fonts]")
{
    TestDataFile testFont(s_FontFile);
    auto context = FontContext();
    context.Scope = ScopeEnum::User;
    context.PackageId = L"TestPackage";
    context.PackageVersion = L"1.0.0.0";
    context.InstallerSource = InstallerSource::WinGet;
    context.AddPackageFile(testFont.GetPath());
    const auto& fontValidationResult = ValidateFontPackage(context);
    REQUIRE(fontValidationResult.HResult == S_OK);
    REQUIRE(fontValidationResult.Result == FontResult::Success);
    REQUIRE(fontValidationResult.HasUnsupportedFonts == false);
    REQUIRE(fontValidationResult.Status == FontStatus::Absent);
}

TEST_CASE("PackageInformationUsedInPathsIsValidated", "[fonts]")
{
    TestDataFile testFont(s_FontFile);

    // The package id and version are used to construct both file system and registry paths. They are not
    // always supplied by a validated manifest; an uninstall without a manifest uses the moniker and version
    // of the installed package, which are read back from the font registry keys. Those keys are writable by
    // the user for a per-user scope, so the values are checked at their point of use.
    auto makeContext = [&](std::wstring_view packageId, std::wstring_view packageVersion)
        {
            auto context = FontContext();
            context.Scope = ScopeEnum::User;
            context.InstallerSource = InstallerSource::WinGet;
            context.PackageId = packageId;
            context.PackageVersion = packageVersion;
            context.AddPackageFile(testFont.GetPath());
            return context;
        };

    // A well formed package is unaffected.
    auto goodContext = makeContext(L"TestPackage", L"1.0.0.0");
    REQUIRE(ValidateFontPackage(goodContext).Result == FontResult::Success);

    // Values that escape the directory they are used to construct.
    for (const auto& value : { L"..", L"..\\..", L"C:" })
    {
        auto idContext = makeContext(value, L"1.0.0.0");
        REQUIRE_THROWS_HR(ValidateFontPackage(idContext), APPINSTALLER_CLI_ERROR_INVALID_MANIFEST);

        auto versionContext = makeContext(L"TestPackage", value);
        REQUIRE_THROWS_HR(ValidateFontPackage(versionContext), APPINSTALLER_CLI_ERROR_INVALID_MANIFEST);
    }

    // Values containing a path separator or another character that is not valid in a path. A registry key
    // name cannot contain a separator, but the value can reach here from other sources.
    for (const auto& value : { L"Test\\Package", L"Test/Package", L"Test*Package", L"Test|Package" })
    {
        auto idContext = makeContext(value, L"1.0.0.0");
        REQUIRE_THROWS_HR(ValidateFontPackage(idContext), APPINSTALLER_CLI_ERROR_INVALID_MANIFEST);

        auto versionContext = makeContext(L"TestPackage", value);
        REQUIRE_THROWS_HR(ValidateFontPackage(versionContext), APPINSTALLER_CLI_ERROR_INVALID_MANIFEST);
    }

    // Reserved device names, which a registry key name is free to use.
    for (const auto& value : { L"CON", L"NUL.txt", L"COM1" })
    {
        auto idContext = makeContext(value, L"1.0.0.0");
        REQUIRE_THROWS_HR(ValidateFontPackage(idContext), APPINSTALLER_CLI_ERROR_INVALID_MANIFEST);

        auto versionContext = makeContext(L"TestPackage", value);
        REQUIRE_THROWS_HR(ValidateFontPackage(versionContext), APPINSTALLER_CLI_ERROR_INVALID_MANIFEST);
    }

    // Whitespace is not a path safety concern, so it must not be rejected here. The values have to keep
    // matching the paths and registry keys that were persisted at install time, so they are never altered.
    auto whitespaceContext = makeContext(L"Test Package", L"1.0 beta");
    REQUIRE(ValidateFontPackage(whitespaceContext).Result == FontResult::Success);
}

TEST_CASE("InstallInvalidFontPackageUser", "[fonts]")
{
    TestDataFile testFont(s_InvalidFontFile);
    auto context = FontContext();
    context.PackageId = L"TestPackage";
    context.PackageVersion = L"1.0.0.0";
    context.InstallerSource = InstallerSource::WinGet;
    context.Scope = ScopeEnum::User;
    context.AddPackageFile(testFont.GetPath());
    const auto& result = InstallFontPackage(context);
    REQUIRE(result.HResult == APPINSTALLER_CLI_ERROR_FONT_FILE_NOT_SUPPORTED);
}

TEST_CASE("RemoveFontPackageUser", "[fonts]")
{
    TestDataFile testFont(s_FontFile);

    // Calling remove should always be successful when font doesn't exist.
    auto context = FontContext();
    context.PackageId = L"TestPackage";
    context.PackageVersion = L"1.0.0.0";
    context.InstallerSource = InstallerSource::WinGet;
    context.Scope = ScopeEnum::User;
    context.AddPackageFile(testFont.GetPath());

    const auto& result = UninstallFontPackage(context);
    REQUIRE(result.HResult == S_OK);

    auto fontValidationResult = ValidateFontPackage(context);
    REQUIRE(fontValidationResult.Status == FontStatus::Absent);
}

TEST_CASE("InstallValidFontPackageUser", "[fonts]")
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
    context.PackageId = L"TestPackage";
    context.PackageVersion = L"1.0.0.0";
    context.InstallerSource = InstallerSource::WinGet;
    context.Scope = ScopeEnum::User;
    context.Force = true;
    context.AddPackageFile(testFont.GetPath());

    auto preinstallValidation = ValidateFontPackage(context);
    REQUIRE(preinstallValidation.Status == FontStatus::Absent);

    auto installResult = InstallFontPackage(context);
    REQUIRE(installResult.HResult == S_OK);

    auto postinstallValidation = ValidateFontPackage(context);
    REQUIRE(postinstallValidation.Status == FontStatus::OK);

    auto uninstallResult = UninstallFontPackage(context);
    REQUIRE(uninstallResult.HResult == S_OK);

    auto postuninstallValidation = ValidateFontPackage(context);
    REQUIRE(postuninstallValidation.Status == FontStatus::Absent);
}

TEST_CASE("RemoveFontPackageMachine", "[fonts]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    TestDataFile testFont(s_FontFile);

    // Calling remove should always be successful when font doesn't exist.
    auto context = FontContext();
    context.PackageId = L"TestPackage";
    context.PackageVersion = L"1.0.0.0";
    context.InstallerSource = InstallerSource::WinGet;
    context.Scope = ScopeEnum::Machine;
    context.AddPackageFile(testFont.GetPath());

    const auto& result = UninstallFontPackage(context);
    REQUIRE(result.HResult == S_OK);

    auto fontValidationResult = ValidateFontPackage(context);
    REQUIRE(fontValidationResult.Status == FontStatus::Absent);
}

TEST_CASE("InstallValidFontPackageMachine", "[fonts]")
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
    context.PackageId = L"TestPackage";
    context.PackageVersion = L"1.0.0.0";
    context.InstallerSource = InstallerSource::WinGet;
    context.Scope = ScopeEnum::Machine;
    context.Force = true;
    context.AddPackageFile(testFont.GetPath());

    auto preinstallValidation = ValidateFontPackage(context);
    REQUIRE(preinstallValidation.Status == FontStatus::Absent);

    auto installResult = InstallFontPackage(context);
    REQUIRE(installResult.HResult == S_OK);

    auto postinstallValidation = ValidateFontPackage(context);
    REQUIRE(postinstallValidation.Status == FontStatus::OK);

    auto uninstallResult = UninstallFontPackage(context);
    REQUIRE(uninstallResult.HResult == S_OK);

    auto postuninstallValidation = ValidateFontPackage(context);
    REQUIRE(postuninstallValidation.Status == FontStatus::Absent);
}
