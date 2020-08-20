// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerRepositorySource.h>
#include <AppInstallerRuntime.h>
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

constexpr std::string_view s_MsixFile_1 = "index.1.0.0.0.msix";
constexpr std::string_view s_MsixFile_2 = "index.2.0.0.0.msix";
constexpr std::string_view s_Msix_FamilyName = "AppInstallerCLITestsFakeIndex_125rzkzqaqjwj";
constexpr std::string_view s_AppxManifestFileName = "AppxManifest.xml"sv;
constexpr std::string_view s_IndexMsixName = "source.msix"sv;
constexpr std::string_view s_IndexFileName = "index.db"sv;

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
    RemoveSetting(Streams::UserSources);
    RemoveSetting(Streams::SourcesMetadata);
    fs::remove_all(GetPathToFileDir());
}

TEST_CASE("PIPS_Add", "[pips]")
{
    CleanSources();

    TempDirectory dir("pipssource");
    TestDataFile index(s_MsixFile_1);
    CopyIndexFileToDirectory(index, dir);

    std::string name = "TestName";
    std::string type(AppInstaller::Repository::Microsoft::PreIndexedPackageSourceFactory::Type());
    std::string arg = dir;
    ProgressCallback callback;

    AddSource(name, type, arg, callback);

    fs::path state = GetPathToFileDir();
    REQUIRE(fs::exists(state));

    fs::path manifest = state;
    manifest /= s_AppxManifestFileName;
    REQUIRE(fs::exists(manifest));
    REQUIRE(fs::file_size(manifest) > 0);

    fs::path indexFile = state;
    indexFile /= s_IndexFileName;
    REQUIRE(fs::exists(indexFile));
    REQUIRE(fs::file_size(indexFile) > 0);
}

TEST_CASE("PIPS_UpdateSameVersion", "[pips]")
{
    CleanSources();

    TempDirectory dir("pipssource");
    TestDataFile index(s_MsixFile_1);
    CopyIndexFileToDirectory(index, dir);

    std::string name = "TestName";
    std::string type(AppInstaller::Repository::Microsoft::PreIndexedPackageSourceFactory::Type());
    std::string arg = dir;
    TestProgress callback;

    AddSource(name, type, arg, callback);

    fs::path state = GetPathToFileDir();
    REQUIRE(fs::exists(state));

    bool progressCalled = false;
    callback.m_OnProgress = [&](uint64_t, uint64_t, ProgressType) { progressCalled = true; };

    UpdateSource(name, callback);
    REQUIRE(!progressCalled);
}

TEST_CASE("PIPS_UpdateNewVersion", "[pips]")
{
    CleanSources();

    TempDirectory dir("pipssource");
    TestDataFile indexMsix1(s_MsixFile_1);
    CopyIndexFileToDirectory(indexMsix1, dir);

    std::string name = "TestName";
    std::string type(AppInstaller::Repository::Microsoft::PreIndexedPackageSourceFactory::Type());
    std::string arg = dir;
    TestProgress callback;

    AddSource(name, type, arg, callback);

    fs::path state = GetPathToFileDir();
    REQUIRE(fs::exists(state));

    fs::path manifestPath = state;
    manifestPath /= s_AppxManifestFileName;
    std::string manifestContents1 = GetContents(manifestPath);

    fs::path indexPath = state;
    indexPath /= s_IndexFileName;
    std::string indexContents1 = GetContents(indexPath);

    TestDataFile indexMsix2(s_MsixFile_2);
    CopyIndexFileToDirectory(indexMsix2, dir);

    bool progressCalled = false;
    callback.m_OnProgress = [&](uint64_t, uint64_t, ProgressType) { progressCalled = true; };

    UpdateSource(name, callback);
    REQUIRE(progressCalled);

    std::string manifestContents2 = GetContents(manifestPath);
    REQUIRE(manifestContents1 != manifestContents2);

    std::string indexContents2 = GetContents(indexPath);
    REQUIRE(indexContents1 != indexContents2);
}

TEST_CASE("PIPS_Remove", "[pips]")
{
    CleanSources();

    TempDirectory dir("pipssource");
    TestDataFile index(s_MsixFile_1);
    CopyIndexFileToDirectory(index, dir);

    std::string name = "TestName";
    std::string type(AppInstaller::Repository::Microsoft::PreIndexedPackageSourceFactory::Type());
    std::string arg = dir;
    ProgressCallback callback;

    AddSource(name, type, arg, callback);

    fs::path state = GetPathToFileDir();
    REQUIRE(fs::exists(state));

    fs::path manifest = state;
    manifest /= s_AppxManifestFileName;
    REQUIRE(fs::exists(manifest));

    fs::path indexFile = state;
    indexFile /= s_IndexFileName;
    REQUIRE(fs::exists(indexFile));

    RemoveSource(name, callback);
    REQUIRE(!fs::exists(state));
}
