// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <Microsoft/SQLiteIndexSource.h>
#include <winget/ManifestYamlParser.h>
#include <winget/PackageTrackingCatalog.h>

using namespace std::string_literals;
using namespace TestCommon;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::SQLite;
using namespace AppInstaller::Utility;

namespace
{
    static Source SimpleTestSetup(const std::string& filePath, SourceDetails& details, Manifest& manifest, std::string& relativePath)
    {
        SQLiteIndex index = SQLiteIndex::CreateNew(filePath, AppInstaller::SQLite::Version::Latest(), SQLiteIndex::CreateOptions::SupportPathless | SQLiteIndex::CreateOptions::DisableDependenciesSupport);

        TestDataFile testManifest("Manifest-Good.yaml");
        manifest = YamlParser::CreateFromPath(testManifest);

        relativePath = testManifest.GetPath().filename().u8string();

        index.AddManifest(manifest, relativePath);

        details.Identifier = "*SimpleTestSetup";
        details.Name = "TestName";
        details.Type = "TestType";
        details.Arg = testManifest.GetPath().parent_path().u8string();
        details.Data = "";

        auto result = std::make_shared<SQLiteIndexSource>(details, std::move(index));

        PackageTrackingCatalog::RemoveForSource(result->GetIdentifier());

        return { result };
    }

    struct TestCatalog : public PackageTrackingCatalog
    {
        using PackageTrackingCatalog::CreateForSource;
    };

    PackageTrackingCatalog CreatePackageTrackingCatalogForSource(const Source& source)
    {
        return TestCatalog::CreateForSource(source);
    }
}

TEST_CASE("TrackingCatalog_Create", "[tracking_catalog]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SourceDetails details;
    Manifest manifest;
    std::string relativePath;
    auto source = SimpleTestSetup(tempFile, details, manifest, relativePath);

    PackageTrackingCatalog catalog = CreatePackageTrackingCatalogForSource(source);
}

TEST_CASE("TrackingCatalog_Install", "[tracking_catalog]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SourceDetails details;
    Manifest manifest;
    std::string relativePath;
    auto source = SimpleTestSetup(tempFile, details, manifest, relativePath);

    PackageTrackingCatalog catalog = CreatePackageTrackingCatalogForSource(source);

    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Id, MatchType::Exact, manifest.Id);

    SearchResult resultBefore = catalog.Search(request);
    REQUIRE(resultBefore.Matches.size() == 0);

    catalog.RecordInstall(manifest, manifest.Installers[0], false);

    SearchResult resultAfter = catalog.Search(request);
    REQUIRE(resultAfter.Matches.size() == 1);
    REQUIRE(resultAfter.Matches[0].Package->GetAvailable().size() == 1);

    auto trackingVersion = resultAfter.Matches[0].Package->GetAvailable()[0]->GetLatestVersion();
    REQUIRE(trackingVersion);

    auto metadata = trackingVersion->GetMetadata();
    REQUIRE(metadata.find(PackageVersionMetadata::TrackingWriteTime) != metadata.end());
}

TEST_CASE("TrackingCatalog_Reinstall", "[tracking_catalog]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SourceDetails details;
    Manifest manifest;
    std::string relativePath;
    auto source = SimpleTestSetup(tempFile, details, manifest, relativePath);

    PackageTrackingCatalog catalog = CreatePackageTrackingCatalogForSource(source);

    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Id, MatchType::Exact, manifest.Id);

    catalog.RecordInstall(manifest, manifest.Installers[0], false);

    SearchResult resultBefore = catalog.Search(request);
    REQUIRE(resultBefore.Matches.size() == 1);
    REQUIRE(resultBefore.Matches[0].Package->GetAvailable().size() == 1);
    REQUIRE(resultBefore.Matches[0].Package->GetAvailable()[0]->GetLatestVersion()->GetProperty(PackageVersionProperty::Name) ==
        manifest.DefaultLocalization.Get<Localization::PackageName>());

    // Change name
    std::string newName = "New Package Name";
    manifest.DefaultLocalization.Add<Localization::PackageName>(newName);

    catalog.RecordInstall(manifest, manifest.Installers[0], false);

    SearchResult resultAfter = catalog.Search(request);
    REQUIRE(resultAfter.Matches.size() == 1);
    REQUIRE(resultAfter.Matches[0].Package->GetAvailable().size() == 1);
    REQUIRE(resultAfter.Matches[0].Package->GetAvailable()[0]->GetLatestVersion()->GetProperty(PackageVersionProperty::Name) ==
        newName);
}

TEST_CASE("TrackingCatalog_Upgrade", "[tracking_catalog]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SourceDetails details;
    Manifest manifest;
    std::string relativePath;
    auto source = SimpleTestSetup(tempFile, details, manifest, relativePath);

    PackageTrackingCatalog catalog = CreatePackageTrackingCatalogForSource(source);

    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Id, MatchType::Exact, manifest.Id);

    catalog.RecordInstall(manifest, manifest.Installers[0], false);

    SearchResult resultBefore = catalog.Search(request);
    REQUIRE(resultBefore.Matches.size() == 1);
    REQUIRE(resultBefore.Matches[0].Package->GetAvailable().size() == 1);
    REQUIRE(resultBefore.Matches[0].Package->GetAvailable()[0]->GetLatestVersion()->GetProperty(PackageVersionProperty::Version) ==
        manifest.Version);

    // Change version
    manifest.Version = "99.1.2.3";

    catalog.RecordInstall(manifest, manifest.Installers[0], true);

    SearchResult resultAfter = catalog.Search(request);
    REQUIRE(resultAfter.Matches.size() == 1);
    REQUIRE(resultAfter.Matches[0].Package->GetAvailable().size() == 1);
    REQUIRE(resultAfter.Matches[0].Package->GetAvailable()[0]->GetLatestVersion()->GetProperty(PackageVersionProperty::Version) ==
        manifest.Version);
}

TEST_CASE("TrackingCatalog_Uninstall", "[tracking_catalog]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SourceDetails details;
    Manifest manifest;
    std::string relativePath;
    auto source = SimpleTestSetup(tempFile, details, manifest, relativePath);

    PackageTrackingCatalog catalog = CreatePackageTrackingCatalogForSource(source);

    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Id, MatchType::Exact, manifest.Id);

    catalog.RecordInstall(manifest, manifest.Installers[0], false);

    SearchResult resultBefore = catalog.Search(request);
    REQUIRE(resultBefore.Matches.size() == 1);

    catalog.RecordUninstall(LocIndString{ manifest.Id });

    SearchResult resultAfter = catalog.Search(request);
    REQUIRE(resultAfter.Matches.size() == 0);
}

TEST_CASE("TrackingCatalog_Overlapping_ARP_Range", "[tracking_catalog]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SourceDetails details;
    Manifest manifest;
    std::string relativePath;
    auto source = SimpleTestSetup(tempFile, details, manifest, relativePath);

    REQUIRE(manifest.Installers.size() >= 2);
    AppsAndFeaturesEntry appEntry{};
    appEntry.DisplayVersion = "1.23";
    manifest.Installers[0].AppsAndFeaturesEntries.emplace_back(appEntry);
    appEntry.DisplayVersion = "1.24";
    manifest.Installers[1].AppsAndFeaturesEntries.emplace_back(appEntry);

    PackageTrackingCatalog catalog = CreatePackageTrackingCatalogForSource(source);

    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Id, MatchType::Exact, manifest.Id);

    catalog.RecordInstall(manifest, manifest.Installers[0], false);

    SearchResult resultBefore = catalog.Search(request);
    REQUIRE(resultBefore.Matches.size() == 1);
    REQUIRE(resultBefore.Matches[0].Package->GetAvailable().size() == 1);
    REQUIRE(resultBefore.Matches[0].Package->GetAvailable()[0]->GetLatestVersion()->GetProperty(PackageVersionProperty::Version) ==
        manifest.Version);

    // Change version
    manifest.Version = "99.1.2.3";

    catalog.RecordInstall(manifest, manifest.Installers[0], true);

    SearchResult resultAfter = catalog.Search(request);
    REQUIRE(resultAfter.Matches.size() == 1);
    REQUIRE(resultAfter.Matches[0].Package->GetAvailable().size() == 1);
    REQUIRE(resultAfter.Matches[0].Package->GetAvailable()[0]->GetLatestVersion()->GetProperty(PackageVersionProperty::Version) ==
        manifest.Version);
}
