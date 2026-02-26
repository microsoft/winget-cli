// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <Microsoft/SQLiteIndexSource.h>
#include <winget/ManifestYamlParser.h>

using namespace std::string_literals;
using namespace TestCommon;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::SQLite;

using SQLiteVersion = AppInstaller::SQLite::Version;

static std::shared_ptr<SQLiteIndexSource> SimpleTestSetup(const std::string& filePath, SourceDetails& details, Manifest& manifest, std::string& relativePath, const std::filesystem::path& manifestFile = "Manifest-Good.yaml")
{
    SQLiteVersion latest1 = Version::LatestForMajor(1);
    SQLiteVersion latest2 = Version::LatestForMajor(2);

    const SQLiteVersion versionToUse = GENERATE_COPY(SQLiteVersion{ latest1 }, SQLiteVersion{ latest2 });

    SQLiteIndex index = SQLiteIndex::CreateNew(filePath, versionToUse);

    TestDataFile testManifest(manifestFile);
    manifest = YamlParser::CreateFromPath(testManifest);

    std::filesystem::path testManifestPath = testManifest.GetPath();
    relativePath = testManifestPath.filename().u8string();

    TempDirectory sourceFilesDirectory{ "SQLiteIndexSource" };
    std::filesystem::path sourceFilesDirectoryPath = sourceFilesDirectory.GetPath();
    std::filesystem::create_directories(sourceFilesDirectoryPath);
    std::filesystem::copy_file(testManifestPath, sourceFilesDirectoryPath / relativePath);
    sourceFilesDirectory.Release();

    index.AddManifest(manifest, relativePath);

    details.Name = "TestName";
    details.Type = "TestType";
    details.Arg = sourceFilesDirectoryPath.u8string();
    details.Data = "";
    details.Identifier = "SimpleTestSetup";

    if (versionToUse.MajorVersion == 2)
    {
        index.SetProperty(SQLiteIndex::Property::IntermediateFileOutputPath, sourceFilesDirectoryPath.u8string());
    }

    index.PrepareForPackaging();

    return std::make_shared<SQLiteIndexSource>(details, std::move(index));
}

static bool SupportsChannel(const std::shared_ptr<SQLiteIndexSource>& source)
{
    return source->GetIndex().GetVersion().MajorVersion == 1;
}

TEST_CASE("SQLiteIndexSource_Search_IdExactMatch", "[sqliteindexsource]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SourceDetails details;
    Manifest manifest;
    std::string relativePath;
    std::shared_ptr<SQLiteIndexSource> source = SimpleTestSetup(tempFile, details, manifest, relativePath);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, manifest.Id);

    auto results = source->Search(request);
    REQUIRE(results.Matches.size() == 1);
    REQUIRE(results.Matches[0].Package);
    REQUIRE(results.Matches[0].MatchCriteria.Field == PackageMatchField::Id);
    REQUIRE(results.Matches[0].MatchCriteria.Type == MatchType::Exact);
    REQUIRE(results.Matches[0].MatchCriteria.Value == manifest.Id);
}

TEST_CASE("SQLiteIndexSource_Search_NoMatch", "[sqliteindexsource]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SourceDetails details;
    Manifest manifest;
    std::string relativePath;
    std::shared_ptr<SQLiteIndexSource> source = SimpleTestSetup(tempFile, details, manifest, relativePath);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, "THIS DOES NOT MATCH ANYTHING!");

    auto results = source->Search(request);
    REQUIRE(results.Matches.size() == 0);
}

TEST_CASE("SQLiteIndexSource_Id", "[sqliteindexsource]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SourceDetails details;
    Manifest manifest;
    std::string relativePath;
    std::shared_ptr<SQLiteIndexSource> source = SimpleTestSetup(tempFile, details, manifest, relativePath);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, manifest.Id);

    auto results = source->Search(request);
    REQUIRE(results.Matches.size() == 1);
    REQUIRE(results.Matches[0].Package);
    auto latestVersion = results.Matches[0].Package->GetAvailable()[0]->GetLatestVersion();

    REQUIRE(latestVersion->GetProperty(PackageVersionProperty::Id).get() == manifest.Id);
}

TEST_CASE("SQLiteIndexSource_Name", "[sqliteindexsource]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SourceDetails details;
    Manifest manifest;
    std::string relativePath;
    std::shared_ptr<SQLiteIndexSource> source = SimpleTestSetup(tempFile, details, manifest, relativePath);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, manifest.Id);

    auto results = source->Search(request);
    REQUIRE(results.Matches.size() == 1);
    REQUIRE(results.Matches[0].Package);
    auto latestVersion = results.Matches[0].Package->GetAvailable()[0]->GetLatestVersion();

    REQUIRE(latestVersion->GetProperty(PackageVersionProperty::Name).get() == manifest.DefaultLocalization.Get<Localization::PackageName>());
}

TEST_CASE("SQLiteIndexSource_Versions", "[sqliteindexsource]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SourceDetails details;
    Manifest manifest;
    std::string relativePath;
    std::shared_ptr<SQLiteIndexSource> source = SimpleTestSetup(tempFile, details, manifest, relativePath);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, manifest.Id);

    auto results = source->Search(request);
    REQUIRE(results.Matches.size() == 1);
    REQUIRE(results.Matches[0].Package);
    REQUIRE(results.Matches[0].Package->GetAvailable().size() == 1);

    auto result = results.Matches[0].Package->GetAvailable()[0]->GetVersionKeys();
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].Version == manifest.Version);
    if (SupportsChannel(source))
    {
        REQUIRE(result[0].Channel == manifest.Channel);
    }
}

TEST_CASE("SQLiteIndexSource_GetManifest", "[sqliteindexsource]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SourceDetails details;
    Manifest manifest;
    std::string relativePath;
    std::shared_ptr<SQLiteIndexSource> source = SimpleTestSetup(tempFile, details, manifest, relativePath);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, manifest.Id);

    auto results = source->Search(request);
    REQUIRE(results.Matches.size() == 1);
    REQUIRE(results.Matches[0].Package);
    REQUIRE(results.Matches[0].Package->GetAvailable().size() == 1);
    auto package = results.Matches[0].Package->GetAvailable()[0];

    auto specificResultVersion = package->GetVersion(PackageVersionKey("", manifest.Version, SupportsChannel(source) ? manifest.Channel : ""));
    REQUIRE(specificResultVersion);
    auto specificResult = specificResultVersion->GetManifest();
    REQUIRE(specificResult.Id == manifest.Id);
    REQUIRE(specificResult.DefaultLocalization.Get<Localization::PackageName>() == manifest.DefaultLocalization.Get<Localization::PackageName>());
    REQUIRE(specificResult.Version == manifest.Version);
    if (SupportsChannel(source))
    {
        REQUIRE(specificResult.Channel == manifest.Channel);
    }

    if (SupportsChannel(source))
    {
        auto latestResultVersion = package->GetVersion(PackageVersionKey("", "", manifest.Channel));
        REQUIRE(latestResultVersion);
        auto latestResult = latestResultVersion->GetManifest();
        REQUIRE(latestResult.Id == manifest.Id);
        REQUIRE(latestResult.DefaultLocalization.Get<Localization::PackageName>() == manifest.DefaultLocalization.Get<Localization::PackageName>());
        REQUIRE(latestResult.Version == manifest.Version);
        REQUIRE(latestResult.Channel == manifest.Channel);
    }

    auto noResultVersion = package->GetVersion(PackageVersionKey("", "blargle", "flargle"));
    REQUIRE(!noResultVersion);
}

TEST_CASE("SQLiteIndexSource_IsSame", "[sqliteindexsource]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SourceDetails details;
    Manifest manifest;
    std::string relativePath;
    std::shared_ptr<SQLiteIndexSource> source = SimpleTestSetup(tempFile, details, manifest, relativePath);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, manifest.Id);

    auto result1 = source->Search(request);
    REQUIRE(result1.Matches.size() == 1);
    REQUIRE(result1.Matches[0].Package->GetAvailable().size() == 1);

    auto result2 = source->Search(request);
    REQUIRE(result2.Matches.size() == 1);
    REQUIRE(result2.Matches[0].Package->GetAvailable().size() == 1);

    REQUIRE(result1.Matches[0].Package->GetAvailable()[0]->IsSame(result2.Matches[0].Package->GetAvailable()[0].get()));
}

TEST_CASE("SQLiteIndexSource_Package_ProductCodes", "[sqliteindexsource]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SourceDetails details;
    Manifest manifest;
    std::string relativePath;
    std::shared_ptr<SQLiteIndexSource> source = SimpleTestSetup(tempFile, details, manifest, relativePath);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, manifest.Id);

    auto results = source->Search(request);
    REQUIRE(results.Matches.size() == 1);
    REQUIRE(results.Matches[0].Package);

    auto package = results.Matches[0].Package->GetAvailable()[0];

    auto manifestPCs = manifest.GetProductCodes();
    auto propertyPCs = package->GetMultiProperty(PackageMultiProperty::ProductCode);
    REQUIRE(manifestPCs.size() == 1);
    REQUIRE(propertyPCs.size() == 1);
    REQUIRE(manifestPCs[0] == propertyPCs[0].get());
}

TEST_CASE("SQLiteIndexSource_VersionSelection", "[sqliteindexsource]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SourceDetails details;
    Manifest manifest;
    std::string relativePath;
    std::shared_ptr<SQLiteIndexSource> source = SimpleTestSetup(tempFile, details, manifest, relativePath, "InstallFlowTest_Exe.yaml");

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, manifest.Id);

    auto results = source->Search(request);
    REQUIRE(results.Matches.size() == 1);
    REQUIRE(results.Matches[0].Package);

    auto package = results.Matches[0].Package->GetAvailable()[0];

    PackageVersionKey key{ {}, "1", {} };
    auto version = package->GetVersion(key);
    REQUIRE(version);
}
