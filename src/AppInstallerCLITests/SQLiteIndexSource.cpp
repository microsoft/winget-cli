// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <Manifest/Manifest.h>
#include <Microsoft/SQLiteIndexSource.h>

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
    manifest = Manifest::CreateFromPath(testManifest);

    relativePath = testManifest.GetPath().filename().u8string();

    index.AddManifest(manifest, relativePath);

    details.Name = "TestName";
    details.Type = "TestType";
    details.Arg = testManifest.GetPath().parent_path().u8string();
    details.Data = "";

    return std::make_shared<SQLiteIndexSource>(details, std::move(index));
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
    REQUIRE(results.Matches[0].Application);
    REQUIRE(results.Matches[0].MatchCriteria.Field == ApplicationMatchField::Id);
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
    REQUIRE(results.Matches[0].Application);
    IApplication* app = results.Matches[0].Application.get();

    REQUIRE(app->GetId().get() == manifest.Id);
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
    REQUIRE(results.Matches[0].Application);
    IApplication* app = results.Matches[0].Application.get();

    REQUIRE(app->GetName().get() == manifest.Name);
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
    REQUIRE(results.Matches[0].Application);
    IApplication* app = results.Matches[0].Application.get();

    auto result = app->GetVersions();
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].GetVersion().ToString() == manifest.Version);
    REQUIRE(result[0].GetChannel().ToString() == manifest.Channel);
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
    REQUIRE(results.Matches[0].Application);
    IApplication* app = results.Matches[0].Application.get();

    auto specificResult = app->GetManifest(manifest.Version, manifest.Channel);
    REQUIRE(specificResult.has_value());
    REQUIRE(specificResult->Id == manifest.Id);
    REQUIRE(specificResult->Name == manifest.Name);
    REQUIRE(specificResult->Version == manifest.Version);
    REQUIRE(specificResult->Channel == manifest.Channel);

    auto latestResult = app->GetManifest("", manifest.Channel);
    REQUIRE(latestResult.has_value());
    REQUIRE(latestResult->Id == manifest.Id);
    REQUIRE(latestResult->Name == manifest.Name);
    REQUIRE(latestResult->Version == manifest.Version);
    REQUIRE(latestResult->Channel == manifest.Channel);

    auto noResult = app->GetManifest("blargle", "flargle");
    REQUIRE(!noResult.has_value());
}
