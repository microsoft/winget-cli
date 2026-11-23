// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestHooks.h"
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

namespace
{
    constexpr std::wstring_view s_WinGetFontPrefix = L"Microsoft.DesktopAppInstaller_8wekyb3d8bbwe";

    struct FontLayoutTest
    {
        FontLayoutTest() :
            FontLayoutTest(AppInstaller::Registry::Key::OpenIfExists(HKEY_CURRENT_USER, std::wstring{}, 0, KEY_ALL_ACCESS))
        {
        }

        explicit FontLayoutTest(AppInstaller::Registry::Key registryRoot) :
            RegistryRoot(std::move(registryRoot)),
            // Font registration keys require non-volatile ancestors.
            RegistryPath(L"WinGetFontLayoutTests_" + Directory.GetPath().filename().wstring()),
            RegistryHive(AppInstaller::Registry::Key::Create(RegistryRoot, RegistryPath)),
            UserPath(AppInstaller::Runtime::GetPathDetailsFor(AppInstaller::Runtime::PathName::FontsUserInstallLocation)),
            MachinePath(AppInstaller::Runtime::GetPathDetailsFor(AppInstaller::Runtime::PathName::FontsMachineInstallLocation))
        {
            AppInstaller::Runtime::TestHook_SetPathOverride(AppInstaller::Runtime::PathName::FontsUserInstallLocation, RootPath(ScopeEnum::User));
            AppInstaller::Runtime::TestHook_SetPathOverride(AppInstaller::Runtime::PathName::FontsMachineInstallLocation, RootPath(ScopeEnum::Machine));
            TestHook_SetGetFontRegistryHive_Override([this](ScopeEnum scope) { return Hive(scope); });
            TestHook_SetFontSessionChangesDisabled(true);
        }

        ~FontLayoutTest()
        {
            TestHook_SetGetFontRegistryHive_Override({});
            TestHook_SetFontSessionChangesDisabled(false);
            TestHook_SetPackageFontFilesReadCallback({});
            TestHook_SetFontRegistrationsReadCallback({});
            AppInstaller::Runtime::TestHook_SetPathOverride(AppInstaller::Runtime::PathName::FontsUserInstallLocation, UserPath);
            AppInstaller::Runtime::TestHook_SetPathOverride(AppInstaller::Runtime::PathName::FontsMachineInstallLocation, MachinePath);
            AppInstaller::Registry::Key::DeleteTree(RegistryRoot, RegistryPath);
        }

        std::filesystem::path RootPath(ScopeEnum scope) const
        {
            return Directory.GetPath() / AppInstaller::Manifest::ScopeToString(scope);
        }

        AppInstaller::Registry::Key Hive(ScopeEnum scope) const
        {
            return AppInstaller::Registry::Key::Create(RegistryHive, AppInstaller::Utility::ConvertToUTF16(AppInstaller::Manifest::ScopeToString(scope)));
        }

        AppInstaller::Registry::Key Root(ScopeEnum scope) const
        {
            return AppInstaller::Registry::Key::Create(Hive(scope), GetFontRegistryRoot());
        }

        FontContext Context(ScopeEnum scope) const
        {
            FontContext result;
            result.Scope = scope;
            result.InstallerSource = InstallerSource::WinGet;
            result.PackageId = L"TestPackage";
            result.PackageVersion = L"1.0.0.0";
            return result;
        }

        std::wstring PackagePath(const FontContext& context) const
        {
            return GetFontRegistryRoot() + L"\\" + std::wstring(s_WinGetFontPrefix) + L"\\" + context.PackageId + L"\\" + context.PackageVersion;
        }

        AppInstaller::Registry::Key Package(const FontContext& context) const
        {
            return AppInstaller::Registry::Key::Create(Hive(context.Scope), PackagePath(context));
        }

        std::filesystem::path AddPackageDirectoryFont(const FontContext& context)
        {
            const auto path = RootPath(context.Scope) / s_WinGetFontPrefix / context.PackageId / context.PackageVersion / s_FontFile;
            CopyFont(path);
            Package(context).SetValue(L"Font title", path.wstring());
            return path;
        }

        void CopyFont(const std::filesystem::path& path) const
        {
            std::filesystem::create_directories(path.parent_path());
            std::filesystem::copy_file(TestDataFile(s_FontFile).GetPath(), path);
        }

        TestCommon::TempDirectory Directory{ "FontLayout" };
        AppInstaller::Registry::Key RegistryRoot;
        std::wstring RegistryPath;
        AppInstaller::Registry::Key RegistryHive;
        AppInstaller::Filesystem::PathDetails UserPath;
        AppInstaller::Filesystem::PathDetails MachinePath;
    };

    std::wstring RegistryString(const AppInstaller::Registry::Key& key, const std::wstring& name)
    {
        auto value = key[name];
        REQUIRE(value.has_value());
        REQUIRE(value->GetType() == AppInstaller::Registry::Value::Type::String);
        return value->GetValue<AppInstaller::Registry::Value::Type::UTF16String>();
    }

    std::vector<std::string> RegistryValueNames(const AppInstaller::Registry::Key& key)
    {
        std::vector<std::string> result;
        for (const auto& value : key.Values())
        {
            result.push_back(value.Name());
        }
        return result;
    }

    std::filesystem::path ExtendedLengthPath(const std::filesystem::path& path)
    {
        return L"\\\\?\\" + path.wstring();
    }
}

TEST_CASE("FontLayout_InstallWithVolatileSharedTestTree", "[fonts][fontlayout]")
{
    const auto scope = GENERATE(ScopeEnum::User, ScopeEnum::Machine);
    const TempDirectory directory{ "FontRegistry" };
    const auto registryPath = directory.GetPath().filename().wstring();
    auto cleanup = wil::scope_exit([&]()
        {
            AppInstaller::Registry::Key::DeleteTree(HKEY_CURRENT_USER, registryPath);
        });
    auto registryRoot = AppInstaller::Registry::Key::Create(HKEY_CURRENT_USER, registryPath);
    AppInstaller::Registry::Key::Create(registryRoot, L"Software\\Microsoft\\WinGet", REG_OPTION_VOLATILE);
    REQUIRE_THROWS_HR(
        AppInstaller::Registry::Key::Create(registryRoot, L"Software\\Microsoft\\WinGet\\FontLayoutTests"),
        HRESULT_FROM_WIN32(ERROR_CHILD_MUST_BE_VOLATILE));

    FontLayoutTest layout{ std::move(registryRoot) };
    auto context = layout.Context(scope);
    const auto source = layout.Directory.GetPath() / "Source" / s_FontFile;
    const auto destination = layout.RootPath(scope) / s_FontFile;
    layout.CopyFont(source);
    context.AddPackageFile(source);
    REQUIRE(InstallFontPackage(context).HResult == S_OK);
    REQUIRE(RegistryString(layout.Package(context), L"TestFont.ttf") == destination.wstring());
    REQUIRE(RegistryString(layout.Root(scope), L"TestFont.ttf") ==
        (scope == ScopeEnum::Machine ? destination.filename().wstring() : destination.wstring()));
    REQUIRE(UninstallFontPackage(context).HResult == S_OK);
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_InstallRepairAndUninstall", "[fonts][fontlayout]")
{
    const auto scope = GENERATE(ScopeEnum::User, ScopeEnum::Machine);
    const auto otherScope = scope == ScopeEnum::User ? ScopeEnum::Machine : ScopeEnum::User;
    auto context = Context(scope);
    const auto source = Directory.GetPath() / "Source" / s_FontFile;
    const auto destination = RootPath(scope) / s_FontFile;
    const std::vector<std::string> expectedNames{ std::string{ s_FontFile } };
    CopyFont(source);
    context.AddPackageFile(source);

    REQUIRE(InstallFontPackage(context).HResult == S_OK);
    REQUIRE(GetFontRegistryRoot() == L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts");
    REQUIRE(std::filesystem::exists(destination));
    REQUIRE_FALSE(std::filesystem::exists(source));
    REQUIRE_FALSE(std::filesystem::exists(RootPath(scope) / s_WinGetFontPrefix));
    REQUIRE(RegistryValueNames(Package(context)) == expectedNames);
    REQUIRE(RegistryString(Package(context), L"TestFont.ttf") == destination.wstring());
    REQUIRE(RegistryValueNames(Root(scope)) == expectedNames);
    REQUIRE(RegistryString(Root(scope), L"TestFont.ttf") ==
        (scope == ScopeEnum::Machine ? destination.filename().wstring() : destination.wstring()));
    REQUIRE_FALSE(AppInstaller::Registry::Key::OpenIfExists(Hive(otherScope), GetFontRegistryRoot()));
    REQUIRE(ValidateFontPackage(context).Status == FontStatus::OK);
    auto inventory = GetInstalledFontFiles();
    REQUIRE(inventory.size() == 1);
    REQUIRE(inventory[0].InstallerSource == InstallerSource::WinGet);
    REQUIRE(inventory[0].Scope == scope);

    Root(scope).DeleteValue(L"TestFont.ttf");
    REQUIRE(ValidateFontPackage(context).Status == FontStatus::Corrupt);
    REQUIRE(RepairFontPackage(context).HResult == S_OK);
    REQUIRE(RepairFontPackage(context).HResult == S_OK);
    REQUIRE(RegistryValueNames(Package(context)) == expectedNames);
    REQUIRE(RegistryString(Package(context), L"TestFont.ttf") == destination.wstring());
    REQUIRE(RegistryValueNames(Root(scope)) == expectedNames);
    REQUIRE(RegistryString(Root(scope), L"TestFont.ttf") ==
        (scope == ScopeEnum::Machine ? destination.filename().wstring() : destination.wstring()));
    REQUIRE(ValidateFontPackage(context).Status == FontStatus::OK);
    REQUIRE(GetInstalledFontFiles().size() == 1);

    const auto unrelated = RootPath(scope) / "Unrelated.ttf";
    CopyFont(unrelated);
    Root(scope).SetValue(L"Unrelated.ttf", unrelated.wstring());
    REQUIRE(UninstallFontPackage(context).HResult == S_OK);
    REQUIRE_FALSE(std::filesystem::exists(destination));
    REQUIRE_FALSE(Root(scope)[std::wstring{ L"TestFont.ttf" }].has_value());
    REQUIRE_FALSE(AppInstaller::Registry::Key::OpenIfExists(Hive(scope), PackagePath(context)));
    REQUIRE(std::filesystem::exists(unrelated));
    REQUIRE(RegistryString(Root(scope), L"Unrelated.ttf") == unrelated.wstring());
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_FileAndRegistryCollisions", "[fonts][fontlayout]")
{
    const auto scope = GENERATE(ScopeEnum::User, ScopeEnum::Machine);
    auto first = Context(scope);
    const auto source = Directory.GetPath() / "Source" / s_FontFile;
    CopyFont(source);
    CopyFont(RootPath(scope) / s_FontFile);
    Root(scope).SetValue(L"TestFont0.ttf", L"Unrelated.ttf");
    first.AddPackageFile(source);
    REQUIRE(InstallFontPackage(first).HResult == S_OK);
    const auto firstDestination = RootPath(scope) / "TestFont1.ttf";
    const std::vector<std::string> expectedNames{ std::string{ s_FontFile } };
    REQUIRE(RegistryValueNames(Package(first)) == expectedNames);
    REQUIRE(RegistryString(Package(first), L"TestFont.ttf") == firstDestination.wstring());
    REQUIRE(RegistryString(Root(scope), L"TestFont1.ttf") ==
        (scope == ScopeEnum::Machine ? firstDestination.filename().wstring() : firstDestination.wstring()));
    REQUIRE(ValidateFontPackage(first).Status == FontStatus::OK);

    auto second = Context(scope);
    second.PackageId = L"OtherPackage";
    const auto otherSource = Directory.GetPath() / "OtherSource" / s_FontFile;
    CopyFont(otherSource);
    second.AddPackageFile(otherSource);
    REQUIRE(InstallFontPackage(second).HResult == S_OK);
    const auto secondDestination = RootPath(scope) / "TestFont2.ttf";
    REQUIRE(RegistryValueNames(Package(second)) == expectedNames);
    REQUIRE(RegistryString(Package(second), L"TestFont.ttf") == secondDestination.wstring());
    REQUIRE(RegistryString(Root(scope), L"TestFont2.ttf") ==
        (scope == ScopeEnum::Machine ? secondDestination.filename().wstring() : secondDestination.wstring()));
    REQUIRE(UninstallFontPackage(first).HResult == S_OK);
    REQUIRE_FALSE(Root(scope)[std::wstring{ L"TestFont1.ttf" }].has_value());
    REQUIRE_FALSE(AppInstaller::Registry::Key::OpenIfExists(Hive(scope), PackagePath(first)));
    REQUIRE(std::filesystem::exists(secondDestination));
    REQUIRE(RegistryString(Root(scope), L"TestFont2.ttf") ==
        (scope == ScopeEnum::Machine ? secondDestination.filename().wstring() : secondDestination.wstring()));
    REQUIRE(ValidateFontPackage(second).Status == FontStatus::OK);
    REQUIRE(RegistryString(Root(scope), L"TestFont0.ttf") == L"Unrelated.ttf");
    REQUIRE(std::filesystem::exists(RootPath(scope) / s_FontFile));
    REQUIRE(UninstallFontPackage(second).HResult == S_OK);
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_DistinctSourceNamesShareDestinationCandidate", "[fonts][fontlayout]")
{
    const auto scope = GENERATE(ScopeEnum::User, ScopeEnum::Machine);
    auto context = Context(scope);
    const auto existing = RootPath(scope) / s_FontFile;
    const auto firstSource = Directory.GetPath() / "Source" / s_FontFile;
    const auto secondSource = Directory.GetPath() / "Source" / "TestFont0.ttf";
    const auto firstDestination = RootPath(scope) / "TestFont0.ttf";
    const auto secondDestination = RootPath(scope) / "TestFont00.ttf";
    CopyFont(existing);
    CopyFont(firstSource);
    CopyFont(secondSource);
    context.AddPackageFile(firstSource);
    context.AddPackageFile(secondSource);

    REQUIRE(InstallFontPackage(context).HResult == S_OK);
    REQUIRE(std::filesystem::exists(firstDestination));
    REQUIRE(std::filesystem::exists(secondDestination));
    REQUIRE(RegistryString(Package(context), L"TestFont.ttf") == firstDestination.wstring());
    REQUIRE(RegistryString(Package(context), L"TestFont0.ttf") == secondDestination.wstring());
    REQUIRE(ValidateFontPackage(context).Status == FontStatus::OK);
    REQUIRE(RepairFontPackage(context).HResult == S_OK);
    REQUIRE(RegistryString(Package(context), L"TestFont.ttf") == firstDestination.wstring());
    REQUIRE(RegistryString(Package(context), L"TestFont0.ttf") == secondDestination.wstring());
    REQUIRE(UninstallFontPackage(context).HResult == S_OK);
    REQUIRE_FALSE(std::filesystem::exists(firstDestination));
    REQUIRE_FALSE(std::filesystem::exists(secondDestination));
    REQUIRE(std::filesystem::exists(existing));
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_LongFilenamesWithCollisions", "[fonts][fontlayout]")
{
    const auto scope = GENERATE(ScopeEnum::User, ScopeEnum::Machine);
    const auto nameLength = GENERATE(size_t{ 128 }, size_t{ 129 });
    auto context = Context(scope);
    const auto stem = std::string(nameLength - 4, 'f');
    const std::filesystem::path name = stem + ".ttf";
    const auto existing = RootPath(scope) / name;
    const auto source = Directory.GetPath() / "Source" / name;
    const auto destination = RootPath(scope) / (stem + "0.ttf");
    CopyFont(existing);
    CopyFont(source);
    context.AddPackageFile(source);

    REQUIRE(InstallFontPackage(context).HResult == S_OK);
    REQUIRE(std::filesystem::exists(destination));
    REQUIRE(RegistryString(Package(context), name.wstring()) == destination.wstring());
    REQUIRE(ValidateFontPackage(context).Status == FontStatus::OK);
    Root(scope).DeleteValue(destination.filename().wstring());
    REQUIRE(RepairFontPackage(context).HResult == S_OK);
    REQUIRE(RegistryString(Package(context), name.wstring()) == destination.wstring());
    REQUIRE(RegistryString(Root(scope), destination.filename().wstring()) ==
        (scope == ScopeEnum::Machine ? destination.filename().wstring() : destination.wstring()));
    REQUIRE(GetInstalledFontFiles().size() == 1);
    REQUIRE(UninstallFontPackage(context).HResult == S_OK);
    REQUIRE_FALSE(std::filesystem::exists(destination));
    REQUIRE_FALSE(Root(scope)[destination.filename().wstring()].has_value());
    REQUIRE(std::filesystem::exists(existing));
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_RejectsUnsafeFilenames", "[fonts][fontlayout]")
{
    const auto scope = GENERATE(ScopeEnum::User, ScopeEnum::Machine);
    const std::string unsafeName = GENERATE("", "..\\Font.ttf", "NUL.ttf", "Font.ttf:stream", "Font.ttf. ");
    const bool invalidPath = GENERATE(false, true);
    auto context = Context(scope);
    const auto installed = RootPath(scope) / s_FontFile;
    CopyFont(installed);
    const auto registryName = invalidPath ? std::wstring{ L"TestFont.ttf" } : AppInstaller::Utility::ConvertToUTF16(unsafeName);
    const auto filePath = invalidPath ? RootPath(scope) / unsafeName : installed;
    Package(context).SetValue(registryName, filePath.wstring());

    REQUIRE(RepairFontPackage(context).HResult == APPINSTALLER_CLI_ERROR_FONT_VALIDATION_FAILED);
    REQUIRE(UninstallFontPackage(context).HResult == APPINSTALLER_CLI_ERROR_FONT_UNINSTALL_FAILED);
    REQUIRE(RegistryString(Package(context), registryName) == filePath.wstring());
    REQUIRE(std::filesystem::exists(installed));
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_MaximumFilenameCollisions", "[fonts][fontlayout]")
{
    const auto scope = GENERATE(ScopeEnum::User, ScopeEnum::Machine);
    const bool supplementaryCharacter = GENERATE(false, true);
    const int suffix = GENERATE(0, 10);
    auto context = Context(scope);
    // Extended-length paths isolate the filename component limit from MAX_PATH.
    const auto root = ExtendedLengthPath(RootPath(scope));
    AppInstaller::Runtime::TestHook_SetPathOverride(scope == ScopeEnum::Machine ?
        AppInstaller::Runtime::PathName::FontsMachineInstallLocation : AppInstaller::Runtime::PathName::FontsUserInstallLocation, root);
    auto cleanup = wil::scope_exit([&]() { std::filesystem::remove_all(ExtendedLengthPath(Directory.GetPath())); });
    const auto stem = std::wstring(249, L'f') + (supplementaryCharacter ? L"\U0001F600" : L"ff");
    const std::filesystem::path name = stem + L".ttf";
    const auto source = ExtendedLengthPath(Directory.GetPath()) / "Source" / name;
    const auto existing = root / name;
    const auto destination = root /
        (std::wstring(suffix == 0 && !supplementaryCharacter ? 250 : 249, L'f') + std::to_wstring(suffix) + L".ttf");
    REQUIRE(name.wstring().size() == 255);
    CopyFont(source);
    CopyFont(existing);
    for (int i = 0; i < suffix; ++i)
    {
        Root(scope).SetValue(std::wstring(supplementaryCharacter ? 249 : 250, L'f') + std::to_wstring(i) + L".ttf", L"Unrelated.ttf");
    }
    context.AddPackageFile(source);

    REQUIRE(InstallFontPackage(context).HResult == S_OK);
    REQUIRE(std::filesystem::exists(destination));
    REQUIRE(RegistryString(Package(context), name.wstring()) == destination.wstring());
    REQUIRE(ValidateFontPackage(context).Status == FontStatus::OK);
    REQUIRE(RepairFontPackage(context).HResult == S_OK);
    const auto inventory = GetInstalledFontFiles();
    REQUIRE(inventory.size() == static_cast<size_t>(suffix + 1));
    REQUIRE(std::count_if(inventory.begin(), inventory.end(), [&](const auto& font)
        {
            return font.InstallerSource == InstallerSource::WinGet && font.FilePath == destination && font.Status == FontStatus::OK;
        }) == 1);
    REQUIRE(UninstallFontPackage(context).HResult == S_OK);
    REQUIRE_FALSE(std::filesystem::exists(destination));
    REQUIRE(std::filesystem::exists(existing));
    REQUIRE(RegistryValueNames(Root(scope)).size() == static_cast<size_t>(suffix));
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_RepairMigratesMaximumFilename", "[fonts][fontlayout]")
{
    auto context = Context(ScopeEnum::User);
    const auto root = ExtendedLengthPath(RootPath(context.Scope));
    AppInstaller::Runtime::TestHook_SetPathOverride(AppInstaller::Runtime::PathName::FontsUserInstallLocation, root);
    auto cleanup = wil::scope_exit([&]() { std::filesystem::remove_all(ExtendedLengthPath(Directory.GetPath())); });
    const std::filesystem::path name = std::wstring(251, L'f') + L".ttf";
    const auto source = root / s_WinGetFontPrefix / context.PackageId / context.PackageVersion / name;
    const auto existing = root / name;
    const auto destination = root / (std::wstring(250, L'f') + L"0.ttf");
    CopyFont(source);
    CopyFont(existing);
    Package(context).SetValue(L"Font title", source.wstring());
    Root(context.Scope).SetValue(L"Font title (TrueType)", source.wstring());

    REQUIRE(RepairFontPackage(context).HResult == S_OK);
    REQUIRE(std::filesystem::exists(destination));
    REQUIRE_FALSE(std::filesystem::exists(source));
    REQUIRE_FALSE(Package(context)[std::wstring{ L"Font title" }].has_value());
    REQUIRE_FALSE(Root(context.Scope)[std::wstring{ L"Font title (TrueType)" }].has_value());
    REQUIRE(RegistryString(Package(context), name.wstring()) == destination.wstring());
    REQUIRE(RegistryString(Root(context.Scope), destination.filename().wstring()) == destination.wstring());
    REQUIRE(UninstallFontPackage(context).HResult == S_OK);
    REQUIRE_FALSE(std::filesystem::exists(destination));
    REQUIRE(std::filesystem::exists(existing));
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_InventoryReadsMappingsOncePerVersion", "[fonts][fontlayout]")
{
    const auto scope = GENERATE(ScopeEnum::User, ScopeEnum::Machine);
    auto context = Context(scope);
    for (const auto& version : { L"1.0", L"2.0" })
    {
        context.PackageVersion = version;
        for (int i = 0; i < 3; ++i)
        {
            const auto name = L"Font" + std::to_wstring(i) + L".ttf";
            const auto installed = RootPath(scope) / (context.PackageVersion + name);
            CopyFont(installed);
            Package(context).SetValue(name, installed.wstring());
            Root(scope).SetValue(installed.filename().wstring(),
                scope == ScopeEnum::Machine ? installed.filename().wstring() : installed.wstring());
        }
    }

    size_t mappingReads = 0;
    size_t registrationReads = 0;
    TestHook_SetPackageFontFilesReadCallback([&]() { ++mappingReads; });
    TestHook_SetFontRegistrationsReadCallback([&]() { ++registrationReads; });
    const auto inventory = GetInstalledFontFiles();
    REQUIRE(inventory.size() == 6);
    REQUIRE(mappingReads == 2);
    REQUIRE(registrationReads == 1);
    for (const auto& font : inventory)
    {
        REQUIRE(font.InstallerSource == InstallerSource::WinGet);
        REQUIRE(font.Status == FontStatus::OK);
        REQUIRE(font.InstallPath == font.FilePath);
    }

    mappingReads = 0;
    registrationReads = 0;
    const auto packages = GetInstalledFontPackages(scope);
    REQUIRE(packages.size() == 2);
    REQUIRE(mappingReads == 2);
    REQUIRE(registrationReads == 1);
    for (const auto& package : packages)
    {
        REQUIRE(package.Status == FontStatus::OK);
    }

    const auto added = RootPath(scope) / L"Added.ttf";
    CopyFont(added);
    Package(context).SetValue(L"Added.ttf", added.wstring());
    Root(scope).SetValue(L"Added.ttf", scope == ScopeEnum::Machine ? added.filename().wstring() : added.wstring());
    mappingReads = 0;
    REQUIRE(GetInstalledFontFiles().size() == 7);
    REQUIRE(mappingReads == 2);

    Root(scope).DeleteValue(L"1.0Font0.ttf");
    mappingReads = 0;
    registrationReads = 0;
    const auto changedPackages = GetInstalledFontPackages(scope);
    REQUIRE(changedPackages.size() == 2);
    REQUIRE(mappingReads == 2);
    REQUIRE(registrationReads == 1);
    REQUIRE(std::count_if(changedPackages.begin(), changedPackages.end(), [&](const auto& package)
        {
            return package.PackageVersion == L"1.0" && package.Status == FontStatus::Corrupt;
        }) == 1);

    Package(context).SetValue(L"Invalid.ttf", DWORD{ 1 });
    mappingReads = 0;
    registrationReads = 0;
    const auto updatedPackages = GetInstalledFontPackages(scope);
    REQUIRE(updatedPackages.size() == 2);
    REQUIRE(mappingReads == 2);
    REQUIRE(registrationReads == 1);
    REQUIRE(std::count_if(updatedPackages.begin(), updatedPackages.end(), [&](const auto& package)
        {
            return package.PackageVersion == context.PackageVersion && package.Status == FontStatus::Unknown;
        }) == 1);
    REQUIRE(RepairFontPackage(context).HResult == APPINSTALLER_CLI_ERROR_FONT_VALIDATION_FAILED);
    REQUIRE(UninstallFontPackage(context).HResult == APPINSTALLER_CLI_ERROR_FONT_UNINSTALL_FAILED);
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_InventoryReadsRegistrationsOncePerScope", "[fonts][fontlayout]")
{
    const auto scope = GENERATE(ScopeEnum::User, ScopeEnum::Machine);
    const auto fontCount = GENERATE(3, 10);
    auto context = Context(scope);
    for (int i = 0; i < fontCount; ++i)
    {
        const auto name = L"Font" + std::to_wstring(i) + L".ttf";
        const auto installed = RootPath(scope) / name;
        CopyFont(installed);
        Package(context).SetValue(name, installed.wstring());
        if (i % 2 == 0)
        {
            const auto registeredName = L"FONT" + std::to_wstring(i) + L".TTF";
            const auto registeredPath = scope == ScopeEnum::Machine ?
                std::filesystem::path{ L"." } / registeredName : RootPath(scope) / L"." / registeredName;
            Root(scope).SetValue(name, registeredPath.wstring());
        }
    }
    Root(scope).SetValue(L"Alias", (RootPath(scope) / L"FONT0.TTF").wstring());
    Root(scope).SetValue(L"Invalid", DWORD{ 1 });
    const auto external = RootPath(scope) / L"External.ttf";
    CopyFont(external);
    Root(scope).SetValue(L"External font", scope == ScopeEnum::Machine ? external.filename().wstring() : external.wstring());
    const auto missing = RootPath(scope) / L"Missing.ttf";
    Root(scope).SetValue(L"Missing font", missing.wstring());

    size_t registrationReads = 0;
    TestHook_SetFontRegistrationsReadCallback([&]() { ++registrationReads; });
    const auto inventory = GetInstalledFontFiles();
    REQUIRE(registrationReads == 1);
    REQUIRE(inventory.size() == static_cast<size_t>(fontCount + 2));
    for (int i = 0; i < fontCount; ++i)
    {
        const auto installed = RootPath(scope) / (L"Font" + std::to_wstring(i) + L".ttf");
        REQUIRE(std::count_if(inventory.begin(), inventory.end(), [&](const auto& font)
            {
                return font.Scope == scope && font.InstallerSource == InstallerSource::WinGet &&
                    font.FilePath == installed && font.InstallPath == installed &&
                    font.Status == (i % 2 == 0 ? FontStatus::OK : FontStatus::Corrupt);
            }) == 1);
    }
    REQUIRE(std::count_if(inventory.begin(), inventory.end(), [&](const auto& font)
        {
            return font.Scope == scope && font.InstallerSource == InstallerSource::Unknown &&
                font.FilePath == external && font.Title == L"External font" && font.Status == FontStatus::OK;
        }) == 1);
    REQUIRE(std::count_if(inventory.begin(), inventory.end(), [&](const auto& font)
        {
            return font.Scope == scope && font.InstallerSource == InstallerSource::Unknown &&
                font.FilePath == missing && font.Status == FontStatus::Corrupt;
        }) == 1);

    const auto addedRegistration = RootPath(scope) / L"Font1.ttf";
    Root(scope).SetValue(L"Font1.ttf", addedRegistration.wstring());
    registrationReads = 0;
    const auto updated = GetInstalledFontFiles();
    REQUIRE(registrationReads == 1);
    REQUIRE(updated.size() == inventory.size());
    REQUIRE(std::count_if(updated.begin(), updated.end(), [&](const auto& font)
        {
            return font.Scope == scope && font.InstallerSource == InstallerSource::WinGet &&
                font.FilePath == addedRegistration && font.Status == FontStatus::OK;
        }) == 1);
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_InventoryContinuesAfterInvalidPackage", "[fonts][fontlayout]")
{
    const bool nonStringValue = GENERATE(false, true);
    auto invalidContext = Context(ScopeEnum::Machine);
    invalidContext.PackageId = L"InvalidPackage";
    const auto invalidPackageFont = RootPath(invalidContext.Scope) / "InvalidPackage.ttf";
    CopyFont(invalidPackageFont);
    Package(invalidContext).SetValue(L"InvalidPackage.ttf", invalidPackageFont.wstring());
    if (nonStringValue)
    {
        Package(invalidContext).SetValue(L"Invalid.ttf", DWORD{ 1 });
    }
    else
    {
        Package(invalidContext).SetValue(L"Invalid.ttf", (Directory.GetPath() / "Outside.ttf").wstring());
    }

    for (const auto scope : { ScopeEnum::Machine, ScopeEnum::User })
    {
        auto context = Context(scope);
        const auto installed = RootPath(scope) / s_FontFile;
        CopyFont(installed);
        Package(context).SetValue(L"TestFont.ttf", installed.wstring());
        Root(scope).SetValue(L"TestFont.ttf",
            scope == ScopeEnum::Machine ? installed.filename().wstring() : installed.wstring());
    }
    const auto external = RootPath(ScopeEnum::Machine) / "External.ttf";
    CopyFont(external);
    Root(ScopeEnum::Machine).SetValue(L"External.ttf", external.filename().wstring());

    const auto inventory = GetInstalledFontFiles();
    REQUIRE(inventory.size() == 3);
    for (const auto scope : { ScopeEnum::Machine, ScopeEnum::User })
    {
        REQUIRE(std::count_if(inventory.begin(), inventory.end(), [&](const auto& font)
            {
                return font.Scope == scope && font.PackageId == L"TestPackage" &&
                    font.InstallerSource == InstallerSource::WinGet &&
                    font.FilePath == RootPath(scope) / s_FontFile;
            }) == 1);
    }
    REQUIRE(std::count_if(inventory.begin(), inventory.end(), [&](const auto& font)
        {
            return font.Scope == ScopeEnum::Machine && font.InstallerSource == InstallerSource::Unknown &&
                font.FilePath == external;
        }) == 1);
    REQUIRE(RepairFontPackage(invalidContext).HResult == APPINSTALLER_CLI_ERROR_FONT_VALIDATION_FAILED);
    REQUIRE(UninstallFontPackage(invalidContext).HResult == APPINSTALLER_CLI_ERROR_FONT_UNINSTALL_FAILED);
    REQUIRE(std::filesystem::exists(invalidPackageFont));
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_UnicodeCaseInsensitiveRegistrations", "[fonts][fontlayout]")
{
    const auto scope = GENERATE(ScopeEnum::User, ScopeEnum::Machine);
    const auto names = GENERATE(
        std::make_pair(L"\u03C3.ttf", L"\u03A3.TTF"),
        std::make_pair(L"\u00E9.ttf", L"\u00C9.TTF"));
    auto context = Context(scope);
    const auto installed = RootPath(scope) / names.first;
    const auto registered = RootPath(scope) / names.second;
    CopyFont(installed);
    Package(context).SetValue(installed.filename().wstring(), installed.wstring());
    Root(scope).SetValue(registered.filename().wstring(),
        scope == ScopeEnum::Machine ? registered.filename().wstring() : registered.wstring());
    context.AddPackageFile(installed);

    const auto inventory = GetInstalledFontFiles();
    REQUIRE(inventory.size() == 1);
    REQUIRE(inventory[0].InstallerSource == InstallerSource::WinGet);
    REQUIRE(inventory[0].Status == FontStatus::OK);
    const auto packages = GetInstalledFontPackages(scope);
    REQUIRE(packages.size() == 1);
    REQUIRE(packages[0].Status == FontStatus::OK);
    REQUIRE(ValidateFontPackage(context).Status == FontStatus::OK);
    REQUIRE(RepairFontPackage(context).HResult == S_OK);
    REQUIRE(UninstallFontPackage(context).HResult == S_OK);
    REQUIRE_FALSE(Root(scope)[registered.filename().wstring()].has_value());
    REQUIRE_FALSE(std::filesystem::exists(installed));
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_InventoryContinuesAfterInvalidVersion", "[fonts][fontlayout]")
{
    const bool nonStringValue = GENERATE(false, true);
    auto invalidContext = Context(ScopeEnum::User);
    invalidContext.PackageVersion = GENERATE(L"1.0", L"2.0");
    AddPackageDirectoryFont(invalidContext);
    if (nonStringValue)
    {
        Package(invalidContext).SetValue(L"Invalid.ttf", DWORD{ 1 });
    }
    else
    {
        Package(invalidContext).SetValue(L"Invalid.ttf", (Directory.GetPath() / "Outside.ttf").wstring());
    }

    auto context = Context(ScopeEnum::User);
    context.PackageVersion = invalidContext.PackageVersion == L"1.0" ? L"2.0" : L"1.0";
    const auto installed = AddPackageDirectoryFont(context);

    const auto inventory = GetInstalledFontFiles();
    REQUIRE(inventory.size() == 1);
    REQUIRE(inventory[0].PackageId == context.PackageId);
    REQUIRE(inventory[0].PackageVersion == context.PackageVersion);
    REQUIRE(inventory[0].Scope == context.Scope);
    REQUIRE(inventory[0].InstallerSource == InstallerSource::WinGet);
    REQUIRE(inventory[0].FilePath == installed);
    REQUIRE(inventory[0].InstallPath == installed);
    REQUIRE(inventory[0].Status == FontStatus::Corrupt);
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_RepairMigratesPackageDirectory", "[fonts][fontlayout]")
{
    const bool keepDirectory = GENERATE(false, true);
    auto context = Context(ScopeEnum::User);
    const auto source = AddPackageDirectoryFont(context);
    const auto sentinel = source.parent_path() / "keep.txt";
    if (keepDirectory)
    {
        std::ofstream{ sentinel } << "Keep this file";
    }
    CopyFont(RootPath(context.Scope) / s_FontFile);
    context.AddPackageFile(source);
    REQUIRE(ValidateFontPackage(context).Status == FontStatus::Corrupt);
    const auto packages = GetInstalledFontPackages(context.Scope);
    REQUIRE(packages.size() == 1);
    REQUIRE(packages[0].Status == FontStatus::Corrupt);

    REQUIRE(RepairFontPackage(context).HResult == S_OK);
    const auto destination = RootPath(context.Scope) / "TestFont0.ttf";
    REQUIRE(std::filesystem::exists(destination));
    REQUIRE_FALSE(std::filesystem::exists(source));
    REQUIRE(std::filesystem::exists(source.parent_path()) == keepDirectory);
    REQUIRE_FALSE(Package(context)[std::wstring{ L"Font title" }].has_value());
    REQUIRE(RegistryString(Package(context), L"TestFont.ttf") == destination.wstring());
    REQUIRE(RegistryString(Root(context.Scope), L"TestFont0.ttf") == destination.wstring());
    REQUIRE(RepairFontPackage(context).HResult == S_OK);
    REQUIRE(RegistryString(Package(context), L"TestFont.ttf") == destination.wstring());
    REQUIRE(GetInstalledFontFiles().size() == 1);
    REQUIRE(UninstallFontPackage(context).HResult == S_OK);
    REQUIRE(std::filesystem::exists(RootPath(context.Scope) / s_FontFile));
    REQUIRE(std::filesystem::exists(sentinel) == keepDirectory);
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_InstalledPathTakesPrecedenceOverSourceName", "[fonts][fontlayout]")
{
    const auto scope = GENERATE(ScopeEnum::User, ScopeEnum::Machine);
    auto context = Context(scope);
    const auto first = RootPath(scope) / "TestFont0.ttf";
    const auto second = RootPath(scope) / "TestFont00.ttf";
    CopyFont(first);
    CopyFont(second);
    Package(context).SetValue(L"TestFont0.ttf", second.wstring());
    Package(context).SetValue(L"TestFont.ttf", first.wstring());
    REQUIRE(CreateFontFileInfo(context, first).InstallPath == first);
    REQUIRE(CreateFontFileInfo(context, second).InstallPath == second);
    REQUIRE(UninstallFontPackage(context).HResult == S_OK);
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_RejectsDuplicateSourceFilenames", "[fonts][fontlayout]")
{
    const auto scope = GENERATE(ScopeEnum::User, ScopeEnum::Machine);
    const auto names = GENERATE(
        std::make_pair(L"TestFont.ttf", L"TestFont.ttf"),
        std::make_pair(L"TestFont.ttf", L"TESTFONT.TTF"),
        std::make_pair(L"\u03C3.ttf", L"\u03A3.TTF"),
        std::make_pair(L"\u00E9.ttf", L"\u00C9.TTF"));
    auto context = Context(scope);
    const auto first = Directory.GetPath() / "First" / names.first;
    const auto second = Directory.GetPath() / "Second" / names.second;
    CopyFont(first);
    CopyFont(second);
    context.AddPackageFile(first);
    context.AddPackageFile(second);
    const auto validation = ValidateFontPackage(context);
    REQUIRE(validation.Result == FontResult::Error);
    REQUIRE(validation.HResult == APPINSTALLER_CLI_ERROR_FONT_VALIDATION_FAILED);
    REQUIRE(std::filesystem::exists(first));
    REQUIRE(std::filesystem::exists(second));
    REQUIRE_FALSE(std::filesystem::exists(RootPath(scope) / names.first));
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_AllowsDistinctUnicodeFilenames", "[fonts][fontlayout]")
{
    const auto scope = GENERATE(ScopeEnum::User, ScopeEnum::Machine);
    const auto names = GENERATE(
        std::make_pair(L"\u00DF.ttf", L"ss.ttf"),
        std::make_pair(L"\u03C3.ttf", L"\u03C2.ttf"));
    auto context = Context(scope);
    const auto first = Directory.GetPath() / "First" / names.first;
    const auto second = Directory.GetPath() / "Second" / names.second;
    CopyFont(first);
    CopyFont(second);
    context.AddPackageFile(first);
    context.AddPackageFile(second);

    REQUIRE(ValidateFontPackage(context).Result == FontResult::Success);
    REQUIRE(InstallFontPackage(context).HResult == S_OK);
    REQUIRE(ValidateFontPackage(context).Status == FontStatus::OK);
    REQUIRE(GetInstalledFontFiles().size() == 2);
    REQUIRE(RepairFontPackage(context).HResult == S_OK);
    REQUIRE(UninstallFontPackage(context).HResult == S_OK);
    REQUIRE_FALSE(std::filesystem::exists(RootPath(scope) / names.first));
    REQUIRE_FALSE(std::filesystem::exists(RootPath(scope) / names.second));
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_UninstallPreservesOtherUnicodePackage", "[fonts][fontlayout]")
{
    const auto scope = GENERATE(ScopeEnum::User, ScopeEnum::Machine);
    const auto names = GENERATE(
        std::make_pair(L"\u00DF.ttf", L"ss.ttf"),
        std::make_pair(L"ss.ttf", L"\u00DF.ttf"),
        std::make_pair(L"\u03C3.ttf", L"\u03C2.ttf"),
        std::make_pair(L"\u03C2.ttf", L"\u03C3.ttf"));
    const bool missingFile = GENERATE(false, true);
    auto context = Context(scope);
    auto otherContext = Context(scope);
    otherContext.PackageId = L"OtherPackage";
    const auto installed = RootPath(scope) / names.first;
    const auto other = RootPath(scope) / names.second;
    CopyFont(installed);
    CopyFont(other);
    REQUIRE_FALSE(std::filesystem::equivalent(installed, other));
    Package(context).SetValue(names.first, installed.wstring());
    Package(otherContext).SetValue(names.second, other.wstring());
    Root(scope).SetValue(names.first,
        scope == ScopeEnum::Machine ? installed.filename().wstring() : installed.wstring());
    const auto otherRegistration = scope == ScopeEnum::Machine ? other.filename().wstring() : other.wstring();
    Root(scope).SetValue(names.second, otherRegistration);
    if (missingFile)
    {
        std::filesystem::remove(installed);
    }

    REQUIRE(UninstallFontPackage(context).HResult == S_OK);
    REQUIRE_FALSE(std::filesystem::exists(installed));
    REQUIRE(std::filesystem::exists(other));
    REQUIRE(RegistryString(Root(scope), names.second) == otherRegistration);
    REQUIRE(RegistryString(Package(otherContext), names.second) == other.wstring());
    const auto inventory = GetInstalledFontFiles();
    REQUIRE(inventory.size() == 1);
    REQUIRE(inventory[0].PackageId == otherContext.PackageId);
    REQUIRE(inventory[0].Status == FontStatus::OK);
    const auto packages = GetInstalledFontPackages(scope);
    REQUIRE(packages.size() == 1);
    REQUIRE(packages[0].PackageId == otherContext.PackageId);
    REQUIRE(packages[0].Status == FontStatus::OK);
    REQUIRE(UninstallFontPackage(otherContext).HResult == S_OK);
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_RepairPreservesUnicodeMapping", "[fonts][fontlayout]")
{
    auto context = Context(ScopeEnum::User);
    const auto source = RootPath(context.Scope) / s_WinGetFontPrefix / context.PackageId / context.PackageVersion / L"\u00E9.ttf";
    const auto destination = RootPath(context.Scope) / source.filename();
    CopyFont(source);
    Package(context).SetValue(L"\u00C9.TTF", source.wstring());
    Root(context.Scope).SetValue(L"Font title", source.wstring());

    REQUIRE(RepairFontPackage(context).HResult == S_OK);
    REQUIRE(RegistryString(Package(context), source.filename().wstring()) == destination.wstring());
    REQUIRE_FALSE(Root(context.Scope)[std::wstring{ L"Font title" }].has_value());
    REQUIRE(RepairFontPackage(context).HResult == S_OK);
    REQUIRE(UninstallFontPackage(context).HResult == S_OK);
    REQUIRE_FALSE(std::filesystem::exists(destination));
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_RepairMoveFailurePreservesMappings", "[fonts][fontlayout]")
{
    auto context = Context(ScopeEnum::User);
    const auto source = AddPackageDirectoryFont(context);
    Root(context.Scope).SetValue(L"Font title", source.wstring());
    wil::unique_hfile lock{ CreateFileW(source.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr) };
    REQUIRE(lock);
    REQUIRE(RepairFontPackage(context).Result() == FontResult::Error);
    REQUIRE(std::filesystem::exists(source));
    REQUIRE_FALSE(std::filesystem::exists(RootPath(context.Scope) / s_FontFile));
    REQUIRE(RegistryString(Package(context), L"Font title") == source.wstring());
    REQUIRE_FALSE(Package(context)[std::wstring{ L"TestFont.ttf" }].has_value());
    REQUIRE(RegistryString(Root(context.Scope), L"Font title") == source.wstring());
    REQUIRE_FALSE(Root(context.Scope)[std::wstring{ L"TestFont.ttf" }].has_value());
    lock.reset();
    REQUIRE(RepairFontPackage(context).HResult == S_OK);
    REQUIRE(UninstallFontPackage(context).HResult == S_OK);
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_MissingFileRepairAndUninstall", "[fonts][fontlayout]")
{
    const auto scope = GENERATE(ScopeEnum::User, ScopeEnum::Machine);
    auto context = Context(scope);
    const auto missing = RootPath(scope) / s_FontFile;
    Package(context).SetValue(L"TestFont.ttf", missing.wstring());
    Root(scope).SetValue(L"TestFont.ttf", missing.wstring());
    REQUIRE(RepairFontPackage(context).HResult == APPINSTALLER_CLI_ERROR_FONT_FILE_NOT_FOUND);
    REQUIRE(RegistryString(Package(context), L"TestFont.ttf") == missing.wstring());
    REQUIRE(UninstallFontPackage(context).HResult == S_OK);
    REQUIRE_FALSE(Root(scope)[std::wstring{ L"TestFont.ttf" }].has_value());
    REQUIRE_FALSE(AppInstaller::Registry::Key::OpenIfExists(Hive(scope), PackagePath(context)));
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_RejectsFilesOutsideFontLocations", "[fonts][fontlayout]")
{
    auto context = Context(ScopeEnum::User);
    const auto unrelated = Directory.GetPath() / "Other" / s_FontFile;
    CopyFont(unrelated);
    Package(context).SetValue(L"TestFont.ttf", unrelated.wstring());
    REQUIRE(RepairFontPackage(context).HResult == APPINSTALLER_CLI_ERROR_FONT_VALIDATION_FAILED);
    REQUIRE(UninstallFontPackage(context).HResult == APPINSTALLER_CLI_ERROR_FONT_UNINSTALL_FAILED);
    REQUIRE(std::filesystem::exists(unrelated));
    REQUIRE(RegistryString(Package(context), L"TestFont.ttf") == unrelated.wstring());
}

TEST_CASE_METHOD(FontLayoutTest, "FontLayout_ReinstallUsesFlatDirectory", "[fonts][fontlayout]")
{
    auto context = Context(ScopeEnum::User);
    context.Force = GENERATE(false, true);
    const auto installed = AddPackageDirectoryFont(context);
    const bool missingFile = GENERATE(false, true);
    if (missingFile)
    {
        std::filesystem::remove(installed);
    }
    const auto source = Directory.GetPath() / "Source" / s_FontFile;
    CopyFont(source);
    context.AddPackageFile(source);
    REQUIRE(InstallFontPackage(context).HResult == S_OK);
    REQUIRE_FALSE(std::filesystem::exists(installed));
    REQUIRE(RegistryString(Package(context), L"TestFont.ttf") == (RootPath(context.Scope) / s_FontFile).wstring());
    REQUIRE(ValidateFontPackage(context).Status == FontStatus::OK);
    REQUIRE(UninstallFontPackage(context).HResult == S_OK);
}

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
