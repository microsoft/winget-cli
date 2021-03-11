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
using namespace AppInstaller::Repository::SQLite;

std::shared_ptr<SQLiteIndexSource> SimpleTestSetup(const std::string& filePath, SourceDetails& details, Manifest& manifest, std::string& relativePath)
{
    SQLiteIndex index = SQLiteIndex::CreateNew(filePath, Schema::Version::Latest());

    TestDataFile testManifest("Manifest-Good.yaml");
    manifest = YamlParser::CreateFromPath(testManifest);

    relativePath = testManifest.GetPath().filename().u8string();

    index.AddManifest(manifest, relativePath);

    details.Name = "TestName";
    details.Type = "TestType";
    details.Arg = testManifest.GetPath().parent_path().u8string();
    details.Data = "";

    return std::make_shared<SQLiteIndexSource>(details, "*SimpleTestSetup", std::move(index));
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
    auto latestVersion = results.Matches[0].Package->GetLatestAvailableVersion();

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
    auto latestVersion = results.Matches[0].Package->GetLatestAvailableVersion();

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

    auto result = results.Matches[0].Package->GetAvailableVersionKeys();
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].Version == manifest.Version);
    REQUIRE(result[0].Channel == manifest.Channel);
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
    auto package = results.Matches[0].Package.get();

    auto specificResultVersion = package->GetAvailableVersion(PackageVersionKey("", manifest.Version, manifest.Channel));
    REQUIRE(specificResultVersion);
    auto specificResult = specificResultVersion->GetManifest();
    REQUIRE(specificResult.Id == manifest.Id);
    REQUIRE(specificResult.DefaultLocalization.Get<Localization::PackageName>() == manifest.DefaultLocalization.Get<Localization::PackageName>());
    REQUIRE(specificResult.Version == manifest.Version);
    REQUIRE(specificResult.Channel == manifest.Channel);

    auto latestResultVersion = package->GetAvailableVersion(PackageVersionKey("", "", manifest.Channel));
    REQUIRE(latestResultVersion);
    auto latestResult = latestResultVersion->GetManifest();
    REQUIRE(latestResult.Id == manifest.Id);
    REQUIRE(latestResult.DefaultLocalization.Get<Localization::PackageName>() == manifest.DefaultLocalization.Get<Localization::PackageName>());
    REQUIRE(latestResult.Version == manifest.Version);
    REQUIRE(latestResult.Channel == manifest.Channel);

    auto noResultVersion = package->GetAvailableVersion(PackageVersionKey("", "blargle", "flargle"));
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

    auto result2 = source->Search(request);
    REQUIRE(result2.Matches.size() == 1);

    REQUIRE(result1.Matches[0].Package->IsSame(result2.Matches[0].Package.get()));
}
