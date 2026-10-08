// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestSource.h"
#include "TestCommon.h"
#include "TestSettings.h"
#include <winget/RepositorySource.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerMsixInfo.h>
#include <AppInstallerStrings.h>
#include <Microsoft/PreIndexedPackageSourceFactory.h>
#include <winget/Settings.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace TestCommon;
using namespace AppInstaller;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Runtime;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Utility;

namespace fs = std::filesystem;

constexpr std::string_view s_RepositorySettings_UserSources = "usersources"sv;

constexpr std::string_view s_MsixFile_1 = "index.1.0.0.0.signed.msix";
constexpr std::string_view s_MsixFile_2 = "index.2.0.0.0.signed.msix";
constexpr std::string_view s_Msix_FamilyName = "AppInstallerCLITestsFakeIndex_8wekyb3d8bbwe";
constexpr std::string_view s_IndexMsixName = "source.msix"sv;

void CopyIndexFileToDirectory(const fs::path& from, const fs::path& to)
{
    fs::path toFile = to;
    toFile /= s_IndexMsixName;
    if (fs::exists(toFile))
    {
        fs::remove(toFile);
    }
    fs::copy_file(from, toFile);
}

fs::path GetPathToFileDir()
{
    fs::path result = GetPathTo(Runtime::PathName::LocalState);
    result /= AppInstaller::Repository::Microsoft::PreIndexedPackageSourceFactory::Type();
    result /= s_Msix_FamilyName;
    return result;
}

std::string GetContents(const fs::path& file)
{
    REQUIRE(fs::exists(file));
    std::ifstream stream(file);
    return ReadEntireStream(stream);
}

void CleanSources()
{
    RemoveSetting(Stream::UserSources);
    RemoveSetting(Stream::SourcesMetadata);
    fs::remove_all(GetPathToFileDir());
}

TEST_CASE("PIPS_PackagedFallbackPrecedence", "[pips]")
{
    using Factory = AppInstaller::Repository::Microsoft::PreIndexedPackageSourceFactory;

    const std::optional<AppInstaller::Msix::PackageVersion> older{ AppInstaller::Msix::PackageVersion{ "1.0.0.0" } };
    const std::optional<AppInstaller::Msix::PackageVersion> newer{ AppInstaller::Msix::PackageVersion{ "2.0.0.0" } };

    // A validated fallback is active while the extension is missing or older.
    REQUIRE(Factory::ShouldPreferDesktopContext(newer, std::nullopt, true));
    REQUIRE(Factory::ShouldPreferDesktopContext(newer, older, true));

    // The extension wins when it catches up, or when we cannot safely inspect the fallback.
    REQUIRE_FALSE(Factory::ShouldPreferDesktopContext(newer, newer, true));
    REQUIRE_FALSE(Factory::ShouldPreferDesktopContext(older, newer, true));
    REQUIRE_FALSE(Factory::ShouldPreferDesktopContext(newer, older, false));
    REQUIRE_FALSE(Factory::ShouldPreferDesktopContext(std::nullopt, older, true));
}

TEST_CASE("PIPS_Add", "[pips]")
{
    if (!Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    CleanSources();

    TempDirectory dir("pipssource");
    TestDataFile index(s_MsixFile_1);
    CopyIndexFileToDirectory(index, dir);

    bool shouldCleanCert = InstallCertFromSignedPackage(index);

    SourceDetails details;
    details.Name = "TestName";
    details.Type = AppInstaller::Repository::Microsoft::PreIndexedPackageSourceFactory::Type();
    details.Arg = dir;
    ProgressCallback callback;

    AddSource(details, callback);

    fs::path state = GetPathToFileDir();
    REQUIRE(fs::exists(state));

    fs::path indexMsix = state;
    indexMsix /= s_IndexMsixName;
    REQUIRE(fs::exists(indexMsix));
    REQUIRE(fs::file_size(indexMsix) > 0);

    if (shouldCleanCert)
    {
        UninstallCertFromSignedPackage(index);
    }
}

TEST_CASE("PIPS_UpdateSameVersion", "[pips]")
{
    if (!Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    CleanSources();

    TempDirectory dir("pipssource");
    TestDataFile index(s_MsixFile_1);
    CopyIndexFileToDirectory(index, dir);

    bool shouldCleanCert = InstallCertFromSignedPackage(index);

    SourceDetails details;
    details.Name = "TestName";
    details.Type = AppInstaller::Repository::Microsoft::PreIndexedPackageSourceFactory::Type();
    details.Arg = dir;
    TestProgress callback;

    AddSource(details, callback);

    fs::path state = GetPathToFileDir();
    REQUIRE(fs::exists(state));

    bool progressCalled = false;
    callback.m_OnProgress = [&](uint64_t, uint64_t, ProgressType) { progressCalled = true; };

    UpdateSource(details.Name, callback);
    REQUIRE(!progressCalled);

    if (shouldCleanCert)
    {
        UninstallCertFromSignedPackage(index);
    }
}

TEST_CASE("PIPS_UpdateNewVersion", "[pips]")
{
    if (!Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    CleanSources();

    TempDirectory dir("pipssource");
    TestDataFile indexMsix1(s_MsixFile_1);
    CopyIndexFileToDirectory(indexMsix1, dir);

    bool shouldCleanCert = InstallCertFromSignedPackage(indexMsix1);

    SourceDetails details;
    details.Name = "TestName";
    details.Type = AppInstaller::Repository::Microsoft::PreIndexedPackageSourceFactory::Type();
    details.Arg = dir;
    TestProgress callback;

    AddSource(details, callback);

    fs::path state = GetPathToFileDir();
    REQUIRE(fs::exists(state));

    fs::path indexMsix = state;
    indexMsix /= s_IndexMsixName;
    std::string indexContents1 = GetContents(indexMsix);

    TestDataFile indexMsix2(s_MsixFile_2);
    CopyIndexFileToDirectory(indexMsix2, dir);

    bool progressCalled = false;
    callback.m_OnProgress = [&](uint64_t, uint64_t, ProgressType) { progressCalled = true; };

    UpdateSource(details.Name, callback);
    REQUIRE(progressCalled);

    std::string indexContents2 = GetContents(indexMsix);
    REQUIRE(indexContents1 != indexContents2);

    if (shouldCleanCert)
    {
        UninstallCertFromSignedPackage(indexMsix1);
    }
}

TEST_CASE("PIPS_Remove", "[pips]")
{
    if (!Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    CleanSources();

    TempDirectory dir("pipssource");
    TestDataFile index(s_MsixFile_1);
    CopyIndexFileToDirectory(index, dir);

    bool shouldCleanCert = InstallCertFromSignedPackage(index);

    SourceDetails details;
    details.Name = "TestName";
    details.Type = AppInstaller::Repository::Microsoft::PreIndexedPackageSourceFactory::Type();
    details.Arg = dir;
    ProgressCallback callback;

    AddSource(details, callback);

    fs::path state = GetPathToFileDir();
    REQUIRE(fs::exists(state));

    fs::path indexMsix = state;
    indexMsix /= s_IndexMsixName;
    REQUIRE(fs::exists(indexMsix));

    RemoveSource(details.Name, callback);
    REQUIRE(!fs::exists(state));

    if (shouldCleanCert)
    {
        UninstallCertFromSignedPackage(index);
    }
}

TEST_CASE("PIPS_RejectsDifferentFamilyCachedPackage", "[pips]")
{
    if (Runtime::IsRunningInPackagedContext())
    {
        WARN("Test exercises the desktop source implementation. Skipped in packaged context.");
        return;
    }

    if (!Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    CleanSources();

    TempDirectory dir("pipssource");
    TestDataFile index(s_MsixFile_1);
    CopyIndexFileToDirectory(index, dir);

    bool shouldCleanCert = InstallCertFromSignedPackage(index);
    bool shouldCleanOtherCert = false;
    TestDataFile otherPackage("TestSignedApp.msix");
    auto cleanup = wil::scope_exit([&]()
        {
            try
            {
                CleanSources();
                if (shouldCleanOtherCert)
                {
                    UninstallCertFromSignedPackage(otherPackage);
                }
                if (shouldCleanCert)
                {
                    UninstallCertFromSignedPackage(index);
                }
            }
            CATCH_LOG();
        });

    SourceDetails details;
    details.Name = "TestName";
    details.Type = AppInstaller::Repository::Microsoft::PreIndexedPackageSourceFactory::Type();
    details.Arg = dir;
    ProgressCallback callback;

    AddSource(details, callback);

    Repository::Source configuredSource{ details.Name };
    auto factory = AppInstaller::Repository::Microsoft::PreIndexedPackageSourceFactory::Create();
    auto reference = factory->Create(configuredSource.GetDetails());

    // Both desktop opens and packaged fallback opens use OpenDesktopContextIndex.
    // Trust must pass so that this test reaches the family-name check.
    shouldCleanOtherCert = InstallCertFromSignedPackage(otherPackage);
    {
        AppInstaller::Msix::WriteLockedMsixFile trustedOtherPackage{ otherPackage.GetPath() };
        REQUIRE(trustedOtherPackage.ValidateTrustInfo(false));
        AppInstaller::Msix::MsixInfo otherInfo{ otherPackage.GetPath() };
        REQUIRE(AppInstaller::Msix::GetPackageFamilyNameFromFullName(otherInfo.GetPackageFullName()) != std::string{ s_Msix_FamilyName });
    }
    CopyIndexFileToDirectory(otherPackage, GetPathToFileDir());
    REQUIRE_THROWS_HR(reference->Open(callback), APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE);

}
