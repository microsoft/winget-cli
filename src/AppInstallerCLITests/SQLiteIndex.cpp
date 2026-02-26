// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/SQLiteWrapper.h>
#include <PackageDependenciesValidation.h>
#include <ArpVersionValidation.h>
#include <Microsoft/SQLiteIndex.h>
#include <winget/Manifest.h>
#include <AppInstallerStrings.h>
#include <winget/SQLiteMetadataTable.h>
#include <winget/PackageVersionDataManifest.h>

#include <Microsoft/Schema/1_0/IdTable.h>
#include <Microsoft/Schema/1_0/NameTable.h>
#include <Microsoft/Schema/1_0/MonikerTable.h>
#include <Microsoft/Schema/1_0/VersionTable.h>
#include <Microsoft/Schema/1_0/ChannelTable.h>
#include <Microsoft/Schema/1_0/PathPartTable.h>
#include <Microsoft/Schema/1_0/ManifestTable.h>
#include <Microsoft/Schema/1_0/TagsTable.h>
#include <Microsoft/Schema/1_0/CommandsTable.h>
#include <Microsoft/Schema/1_0/SearchResultsTable.h>
#include <Microsoft/Schema/1_4/DependenciesTable.h>
#include <Microsoft/Schema/2_0/Interface.h>
#include <Microsoft/Schema/2_0/PackageUpdateTrackingTable.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace TestCommon;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::SQLite;
using namespace AppInstaller::Utility;

using UtilityVersion = AppInstaller::Utility::Version;
using SQLiteVersion = AppInstaller::SQLite::Version;

SQLiteIndex CreateTestIndex(const std::string& filePath, std::optional<SQLiteVersion> version = {})
{
    // If no specific version requested, then use generator to run against the last 3 versions.
    if (!version)
    {
        SQLiteVersion latestVersion{ 2, 0 };
        SQLiteVersion versionMinus1 = SQLiteVersion{ 1, 7 };
        SQLiteVersion versionMinus2 = SQLiteVersion{ 1, 6 };

        version = GENERATE_COPY(SQLiteVersion{ versionMinus2 }, SQLiteVersion{ versionMinus1 }, SQLiteVersion{ latestVersion });
    }

    return SQLiteIndex::CreateNew(filePath, version.value());
}

SQLiteVersion TestPrepareForRead(SQLiteIndex& index)
{
    SQLiteVersion latestVersion{ 2, 0 };
    SQLiteVersion versionMinus1 = SQLiteVersion{ 1, 7 };
    SQLiteVersion versionMinus2 = SQLiteVersion{ 1, 6 };

    index.PrepareForPackaging();

    if (index.GetVersion() == versionMinus2)
    {
        // Degenerate case where we don't need to do anything
    }
    else if (index.GetVersion() == versionMinus1)
    {
        SQLiteVersion version = GENERATE_COPY(SQLiteVersion{ versionMinus2 }, SQLiteVersion{ versionMinus1 });

        if (version != versionMinus1)
        {
            index.ForceVersion(version);
            return version;
        }
    }
    else if (index.GetVersion() == latestVersion)
    {
        // This crosses major versions, so leave it at 2.0 always
    }

    return index.GetVersion();
}

std::string GetPathFromManifest(Manifest& manifest)
{
    auto publisher = manifest.Id;
    AppInstaller::Utility::FindAndReplace(publisher, ".", "/");
    
    return AppInstaller::Utility::ToLower(publisher).append("/").append(manifest.Version);
}

void CreateFakeManifest(Manifest& manifest, string_t publisher, string_t version = "1.0.0")
{
    manifest.Installers.push_back({});
    manifest.Id = publisher.append(".").append("Id");
    manifest.DefaultLocalization.Add<Localization::PackageName>(publisher.append(" Name"));
    manifest.Moniker = "testmoniker";
    manifest.Version = version;
    manifest.Channel = "test";
    manifest.DefaultLocalization.Add<Localization::Tags>({ "t1", "t2" });
    manifest.Installers[0].Commands = { "test1", "test2" };
}

SQLiteIndex SimpleTestSetup(const std::string& filePath, Manifest& manifest, std::optional<SQLiteVersion> version = {})
{
    SQLiteIndex index = CreateTestIndex(filePath, version);

    string_t publisher = "Test";
    CreateFakeManifest(manifest, publisher);

    auto relativePath = GetPathFromManifest(manifest);

    index.AddManifest(manifest, relativePath);

    return index;
}

struct IndexFields
{
    IndexFields(
        std::string id,
        std::string name,
        std::string moniker,
        std::string version,
        std::string channel,
        std::vector<NormalizedString> tags,
        std::vector<NormalizedString> commands,
        std::string path
    ) :
        Id(std::move(id)),
        Name(std::move(name)),
        Moniker(std::move(moniker)),
        Version(std::move(version)),
        Channel(std::move(channel)),
        Tags(std::move(tags)),
        Commands(std::move(commands)),
        Path(std::move(path))
    {}

    IndexFields(
        std::string id,
        std::string name,
        std::string moniker,
        std::string version,
        std::string channel,
        std::vector<NormalizedString> tags,
        std::vector<NormalizedString> commands,
        std::string path,
        std::vector<NormalizedString> packageFamilyNames,
        std::vector<NormalizedString> productCodes
    ) :
        Id(std::move(id)),
        Name(std::move(name)),
        Moniker(std::move(moniker)),
        Version(std::move(version)),
        Channel(std::move(channel)),
        Tags(std::move(tags)),
        Commands(std::move(commands)),
        Path(std::move(path)),
        PackageFamilyNames(std::move(packageFamilyNames)),
        ProductCodes(std::move(productCodes))
    {}

    IndexFields(
        std::string id,
        std::string name,
        std::string publisher,
        std::string moniker,
        std::string version,
        std::string channel,
        std::vector<NormalizedString> tags,
        std::vector<NormalizedString> commands,
        std::string path,
        std::vector<NormalizedString> packageFamilyNames,
        std::vector<NormalizedString> productCodes
    ) :
        Id(std::move(id)),
        Name(std::move(name)),
        Publisher(std::move(publisher)),
        Moniker(std::move(moniker)),
        Version(std::move(version)),
        Channel(std::move(channel)),
        Tags(std::move(tags)),
        Commands(std::move(commands)),
        Path(std::move(path)),
        PackageFamilyNames(std::move(packageFamilyNames)),
        ProductCodes(std::move(productCodes))
    {}

    IndexFields(
        std::string id,
        std::string name,
        std::string publisher,
        std::string moniker,
        std::string version,
        std::string channel,
        std::vector<NormalizedString> tags,
        std::vector<NormalizedString> commands,
        std::string path,
        std::vector<NormalizedString> packageFamilyNames,
        std::vector<NormalizedString> productCodes,
        std::string arpName,
        std::string arpPublisher
    ) :
        Id(std::move(id)),
        Name(std::move(name)),
        Publisher(std::move(publisher)),
        Moniker(std::move(moniker)),
        Version(std::move(version)),
        Channel(std::move(channel)),
        Tags(std::move(tags)),
        Commands(std::move(commands)),
        Path(std::move(path)),
        PackageFamilyNames(std::move(packageFamilyNames)),
        ProductCodes(std::move(productCodes)),
        ArpName(std::move(arpName)),
        ArpPublisher(std::move(arpPublisher))
    {}

    std::string Id;
    std::string Name;
    std::string Publisher;
    std::string Moniker;
    std::string Version;
    std::string Channel;
    std::vector<NormalizedString> Tags;
    std::vector<NormalizedString> Commands;
    std::string Path;
    std::vector<NormalizedString> PackageFamilyNames;
    std::vector<NormalizedString> ProductCodes;
    std::string ArpName;
    std::string ArpPublisher;
};

SQLiteIndex SearchTestSetup(const std::string& filePath, std::initializer_list<IndexFields> data = {}, std::optional<SQLiteVersion> version = {})
{
    SQLiteIndex index = CreateTestIndex(filePath, version);

    Manifest manifest;

    auto addFunc = [&](const IndexFields& d)
    {
        manifest.Id = d.Id;
        manifest.DefaultLocalization.Add<Localization::PackageName>(d.Name);
        manifest.DefaultLocalization.Add<Localization::Publisher>(d.Publisher);
        manifest.Moniker = d.Moniker;
        manifest.Version = d.Version;
        manifest.DefaultLocalization.Add<Localization::Tags>(d.Tags);

        manifest.Installers.resize(std::max(d.PackageFamilyNames.size(), d.ProductCodes.size()));

        if (manifest.Installers.size() == 0)
        {
            manifest.Installers.push_back({});
        }

        manifest.Channel = d.Channel;
        manifest.Installers[0].Commands = d.Commands;

        for (size_t i = 0; i < d.PackageFamilyNames.size(); ++i)
        {
            manifest.Installers[i].PackageFamilyName = d.PackageFamilyNames[i];
        }

        for (size_t i = 0; i < d.ProductCodes.size(); ++i)
        {
            manifest.Installers[i].ProductCode = d.ProductCodes[i];
        }

        if (!d.ArpName.empty() || !d.ArpPublisher.empty())
        {
            manifest.Installers[0].AppsAndFeaturesEntries.push_back({});
            manifest.Installers[0].AppsAndFeaturesEntries[0].DisplayName = d.ArpName;
            manifest.Installers[0].AppsAndFeaturesEntries[0].Publisher = d.ArpPublisher;
        }

        index.AddManifest(manifest, d.Path);
    };

    for (const auto& d : data)
    {
        addFunc(d);
    }

    return index;
}

bool ArePackageFamilyNameAndProductCodeSupported(const SQLiteIndex& index, const SQLiteVersion& testVersion)
{
    UNSCOPED_INFO("Index " << index.GetVersion() << " | Test " << testVersion);
    return (index.GetVersion() >= SQLiteVersion{ 1, 1 } && testVersion >= SQLiteVersion{ 1, 1 });
}

bool AreNormalizedNameAndPublisherSupported(const SQLiteIndex& index, const SQLiteVersion& testVersion)
{
    UNSCOPED_INFO("Index " << index.GetVersion() << " | Test " << testVersion);
    return (index.GetVersion() >= SQLiteVersion{ 1, 2 } && testVersion >= SQLiteVersion{ 1, 2 });
}

bool IsManifestMetadataSupported(const SQLiteIndex& index, const SQLiteVersion& testVersion)
{
    UNSCOPED_INFO("Index " << index.GetVersion() << " | Test " << testVersion);
    return (index.GetVersion() >= SQLiteVersion{ 1, 1 } && testVersion >= SQLiteVersion{ 1, 1 });
}

bool AreManifestHashesSupported(const SQLiteIndex& index, const SQLiteVersion& testVersion)
{
    UNSCOPED_INFO("Index " << index.GetVersion() << " | Test " << testVersion);
    return (index.GetVersion() >= SQLiteVersion{ 1, 3 } && testVersion >= SQLiteVersion{ 1, 3 } && index.GetVersion() < SQLiteVersion{ 2, 0 });
}

bool AreArpVersionsSupported(const SQLiteIndex& index, const SQLiteVersion& testVersion)
{
    UNSCOPED_INFO("Index " << index.GetVersion() << " | Test " << testVersion);
    return (index.GetVersion() >= SQLiteVersion{ 1, 5 } && testVersion >= SQLiteVersion{ 1, 5 });
}

bool AreArpVersionsNullable(const SQLiteIndex& index)
{
    UNSCOPED_INFO("Index " << index.GetVersion());
    return (index.GetVersion() >= SQLiteVersion{ 2, 0 });
}

bool IsMapDataFoldingSupported(const SQLiteIndex& index, const SQLiteVersion& testVersion)
{
    UNSCOPED_INFO("Index " << index.GetVersion() << " | Test " << testVersion);
    return (index.GetVersion() >= SQLiteVersion{ 1, 7 } && testVersion >= SQLiteVersion{ 1, 7 });
}

bool IsMapDataFolded(const SQLiteIndex& index)
{
    UNSCOPED_INFO("Index " << index.GetVersion());
    return (index.GetVersion() >= SQLiteVersion{ 1, 7 });
}

bool AreVersionKeysSupported(const SQLiteIndex& index)
{
    UNSCOPED_INFO("Index " << index.GetVersion());
    return (index.GetVersion() < SQLiteVersion{ 2, 0 });
}

bool AreChannelsSupported(const SQLiteIndex& index)
{
    UNSCOPED_INFO("Index " << index.GetVersion());
    return (index.GetVersion() < SQLiteVersion{ 2, 0 });
}

bool AreManifestPathsSupported(const SQLiteIndex& index)
{
    UNSCOPED_INFO("Index " << index.GetVersion());
    return (index.GetVersion() < SQLiteVersion{ 2, 0 });
}

std::string GetPropertyStringByKey(const SQLiteIndex& index, rowid_t primaryId, PackageVersionProperty property)
{
    auto result = index.GetPropertyByPrimaryId(primaryId, property);
    REQUIRE(result);
    return result.value();
}

std::string GetPropertyStringByKey(const SQLiteIndex& index, rowid_t id, PackageVersionProperty property, std::string_view version, std::string_view channel)
{
    if (AreVersionKeysSupported(index))
    {
        auto manifestId = index.GetManifestIdByKey(id, version, channel);
        REQUIRE(manifestId);
        auto result = index.GetPropertyByPrimaryId(manifestId.value(), property);
        REQUIRE(result);
        return result.value();
    }
    else
    {
        return GetPropertyStringByKey(index, id, property);
    }
}

std::string GetPropertyStringById(const SQLiteIndex& index, rowid_t id, PackageVersionProperty property)
{
    if (AreVersionKeysSupported(index))
    {
        auto versions = index.GetVersionKeysById(id);
        REQUIRE(!versions.empty());
        return GetPropertyStringByKey(index, versions[0].ManifestId, property);
    }
    else
    {
        return GetPropertyStringByKey(index, id, property);
    }
}

std::string GetIdStringById(const SQLiteIndex& index, rowid_t id)
{
    return GetPropertyStringById(index, id, PackageVersionProperty::Id);
}

std::string GetNameStringById(const SQLiteIndex& index, rowid_t id)
{
    return GetPropertyStringById(index, id, PackageVersionProperty::Name);
}

std::string GetPathStringByKey(const SQLiteIndex& index, rowid_t id, std::string_view version, std::string_view channel)
{
    return GetPropertyStringByKey(index, id, PackageVersionProperty::RelativePath, version, channel);
}

TEST_CASE("SQLiteIndexCreateLatestAndReopen", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteVersion versionCreated;

    // Create the index
    {
        SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, SQLiteVersion::Latest());
        versionCreated = index.GetVersion();
    }

    // Reopen the index for read only
    {
        INFO("Trying with Read");
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::Read);
        SQLiteVersion versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }

    // Reopen the index for read/write
    {
        INFO("Trying with ReadWrite");
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        SQLiteVersion versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }

    // Reopen the index for immutable read
    {
        INFO("Trying with Immutable");
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::Immutable);
        SQLiteVersion versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }
}

TEST_CASE("SQLiteIndexCreateAndAddManifest", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest manifest;
    std::string relativePath = "test/id/1.0.0.yaml";

    SQLiteIndex index = SimpleTestSetup(tempFile, manifest, SQLiteVersion::Latest());
}

TEST_CASE("SQLiteIndexCreateAndAddManifestFile", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = CreateTestIndex(tempFile);

    TestDataFile manifestFile{ "Manifest-Good.yaml" };
    std::filesystem::path manifestPath{ "microsoft/msixsdk/microsoft.msixsdk-1.7.32.yaml" };

    index.AddManifest(manifestFile, manifestPath);
}

TEST_CASE("SQLiteIndexCreateAndAddManifestDuplicate", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest manifest;

    SQLiteIndex index = SimpleTestSetup(tempFile, manifest);
    auto relativePath = GetPathFromManifest(manifest);

    // Attempting to add the same manifest at a different path should fail.
    REQUIRE_THROWS_HR(index.AddManifest(manifest, "differentpath.yaml"), HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS));

    // Attempting to add the same manifest with a differently cased Id at a different path should fail.
    manifest.Id = ToLower(manifest.Id);
    REQUIRE_THROWS_HR(index.AddManifest(manifest, "differentpath.yaml"), HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS));

    // Attempting to add a different manifest at the same path should fail.
    manifest.Id += "-new";
    REQUIRE_THROWS_HR(index.AddManifest(manifest, relativePath), HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS));
}

TEST_CASE("SQLiteIndex_VersionReferencedByDependenciesClearsUnusedVersionAndKeepUsedVersion", "[sqliteindex][V1_4]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest dependencyManifest1, dependencyManifest2, manifest, isolatedManifest;
    SQLiteIndex index = SimpleTestSetup(tempFile, dependencyManifest1, SQLiteVersion::Latest());

    auto& publisher2 = "Test2";
    CreateFakeManifest(dependencyManifest2, publisher2);
    index.AddManifest(dependencyManifest2, GetPathFromManifest(dependencyManifest2));

    auto& publisher3 = "Test3";
    CreateFakeManifest(manifest, publisher3);
    std::string dependencyOnlyVersion = "0.0.5";
    manifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, dependencyManifest1.Id, "1.0.0"));
    manifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, dependencyManifest2.Id, dependencyOnlyVersion));

    index.AddManifest(manifest, GetPathFromManifest(manifest));
    
    // Create a new manifest that depends on v0.0.5
    auto& publisher4 = "Test4";
    CreateFakeManifest(isolatedManifest, publisher4);
    isolatedManifest.Version = dependencyOnlyVersion;
    index.AddManifest(isolatedManifest);

    index.RemoveManifest(isolatedManifest);
    // After deletion that the version(v0.0.5) must be present because it still referenced via dependencies table.
    {
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadOnly);
        REQUIRE(Schema::V1_0::VersionTable::SelectIdByValue(connection, dependencyOnlyVersion).has_value());
    }

    index.RemoveManifest(manifest);
    // Now, that we've deleted the manifest depending on version(v0.0.5), it should be absent.
    {
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadOnly);
        REQUIRE(!Schema::V1_0::VersionTable::SelectIdByValue(connection, dependencyOnlyVersion).has_value());
    }
    index.RemoveManifest(dependencyManifest1);
    index.RemoveManifest(dependencyManifest2);
    
    // Final sanity check, nothing should be in the version table.
    {
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadOnly);
        REQUIRE(Schema::V1_0::VersionTable::IsEmpty(connection));
    }
}

TEST_CASE("SQLiteIndex_AddUpdateRemoveManifestWithDependencies", "[sqliteindex][V1_4]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest dependencyManifest1, dependencyManifest2, manifest;
    SQLiteIndex index = SimpleTestSetup(tempFile, dependencyManifest1, SQLiteVersion::Latest());

    auto& publisher2 = "Test2";
    CreateFakeManifest(dependencyManifest2, publisher2);
    index.AddManifest(dependencyManifest2, GetPathFromManifest(dependencyManifest2));

    auto& publisher3 = "Test3";
    CreateFakeManifest(manifest, publisher3);
    manifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, dependencyManifest1.Id, "1.0.0"));
    manifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, dependencyManifest2.Id, "1.0.0"));

    index.AddManifest(manifest, GetPathFromManifest(manifest));
    index.UpdateManifest(manifest, GetPathFromManifest(manifest));
    index.RemoveManifest(manifest);
}

TEST_CASE("SQLiteIndex_AddManifestWithDependencies_MissingPackage", "[sqliteindex][V1_4]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest dependencyManifest1, dependencyManifest2, manifest;
    SQLiteIndex index = SimpleTestSetup(tempFile, dependencyManifest1, SQLiteVersion::Latest());

    // Publisher2 is not present
    auto& publisher2 = "Test2";
    CreateFakeManifest(dependencyManifest2, publisher2);

    auto& publisher3 = "Test3";
    CreateFakeManifest(manifest, publisher3);
    manifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, dependencyManifest1.Id, "1.0.0"));
    manifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, dependencyManifest2.Id, "1.0.0"));

    REQUIRE_THROWS_HR(index.AddManifest(manifest, GetPathFromManifest(manifest)), APPINSTALLER_CLI_ERROR_MISSING_PACKAGE);
}

TEST_CASE("SQLiteIndex_AddUpdateRemoveManifestWithDependencies_MissingVersion", "[sqliteindex][V1_4]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest dependencyManifest1, dependencyManifest2, manifest;
    SQLiteIndex index = SimpleTestSetup(tempFile, dependencyManifest1, SQLiteVersion::Latest());

    auto& publisher2 = "Test2";
    CreateFakeManifest(dependencyManifest2, publisher2);
    index.AddManifest(dependencyManifest2, GetPathFromManifest(dependencyManifest2));

    auto& publisher3 = "Test3";
    CreateFakeManifest(manifest, publisher3);
    manifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, dependencyManifest1.Id, "0.0.1"));
    manifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, dependencyManifest2.Id, "0.0.2"));

    index.AddManifest(manifest, GetPathFromManifest(manifest));
    index.UpdateManifest(manifest, GetPathFromManifest(manifest));
    index.RemoveManifest(manifest);
}

TEST_CASE("SQLiteIndex_AddUpdateRemoveManifestWithDependencies_EmptyManifestVersion", "[sqliteindex][V1_4]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest dependencyManifest1, dependencyManifest2, manifest;
    SQLiteIndex index = SimpleTestSetup(tempFile, dependencyManifest1, SQLiteVersion::Latest());

    auto& publisher2 = "Test2";
    CreateFakeManifest(dependencyManifest2, publisher2);
    index.AddManifest(dependencyManifest2, GetPathFromManifest(dependencyManifest2));

    auto& publisher3 = "Test3";
    CreateFakeManifest(manifest, publisher3);
    manifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, dependencyManifest1.Id));
    manifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, dependencyManifest2.Id));

    index.AddManifest(manifest, GetPathFromManifest(manifest));
    index.UpdateManifest(manifest, GetPathFromManifest(manifest));
    index.RemoveManifest(manifest);
}

TEST_CASE("SQLiteIndex_DependenciesTable_CheckConsistency", "[sqliteindex][V1_4]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());
    
    {
        Manifest levelOneManifest, levelTwoManifest, levelThreeManifest, topLevelManifest;
        SQLiteIndex index = SimpleTestSetup(tempFile, levelThreeManifest, SQLiteVersion::Latest());

        constexpr std::string_view levelTwoManifestPublisher = "LevelTwoManifest";
        CreateFakeManifest(levelTwoManifest, levelTwoManifestPublisher);

        levelTwoManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelThreeManifest.Id, "1.0.0"));
        index.AddManifest(levelTwoManifest, GetPathFromManifest(levelTwoManifest));

        constexpr std::string_view levelOneManifestPublisher = "LevelOneManifest";
        CreateFakeManifest(levelOneManifest, levelOneManifestPublisher);
        levelOneManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelTwoManifest.Id, "1.0.0"));
        index.AddManifest(levelOneManifest, GetPathFromManifest(levelOneManifest));

        constexpr std::string_view topLevelManifestPublisher = "TopLevelManifest";
        CreateFakeManifest(topLevelManifest, topLevelManifestPublisher);
        topLevelManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelOneManifest.Id, "1.0.0"));
    }

    {
        // Open it directly to modify the table
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);
        rowid_t nonExistentRowId = 40;
        rowid_t nonExistentManifest = 41;
        rowid_t nonExistentVersion = 42;
        rowid_t nonExistentPackageId = 43;

        Builder::StatementBuilder builder;
        builder.InsertInto(Schema::V1_4::DependenciesTable::TableName())
            .Values(nonExistentRowId, nonExistentManifest, nonExistentVersion, nonExistentPackageId);
        builder.Execute(connection);
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);

        REQUIRE(!index.CheckConsistency(true));
    }

    TempFile tempFile2{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile2.GetPath());

    {
        SQLiteIndex index = CreateTestIndex(tempFile2, SQLiteVersion::Latest());

        Manifest manifest;
        manifest.Id = "Foo";
        manifest.Version = "10.0";

        index.AddManifest(manifest, "path");

        REQUIRE(index.CheckConsistency(true));

        // Add dependency that does not require min version
        Manifest manifestWithDependency1;
        manifestWithDependency1.Id = "Bar1";
        manifestWithDependency1.Version = "10.0";
        manifestWithDependency1.Installers.push_back({});
        manifestWithDependency1.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, manifest.Id));

        index.AddManifest(manifestWithDependency1, "path1");

        REQUIRE(index.CheckConsistency(true));

        // Add dependency with min version satisfied
        Manifest manifestWithDependency2;
        manifestWithDependency2.Id = "Bar2";
        manifestWithDependency2.Version = "10.0";
        manifestWithDependency2.Installers.push_back({});
        manifestWithDependency2.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, manifest.Id, "1.0"));

        index.AddManifest(manifestWithDependency2, "path2");

        REQUIRE(index.CheckConsistency(true));

        // Add dependency with min version not satisfied
        Manifest manifestWithDependency3;
        manifestWithDependency3.Id = "Bar3";
        manifestWithDependency3.Version = "10.0";
        manifestWithDependency3.Installers.push_back({});
        manifestWithDependency3.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, manifest.Id, "11.0"));

        index.AddManifest(manifestWithDependency3, "path3");

        REQUIRE_FALSE(index.CheckConsistency(true));
    }
}

TEST_CASE("SQLiteIndex_RemoveManifestFile_NotPresent", "[sqliteindex]")
{
    SQLiteIndex index = CreateTestIndex(SQLITE_MEMORY_DB_CONNECTION_TARGET);

    TestDataFile manifestFile{ "Manifest-Good.yaml" };
    std::filesystem::path manifestPath{ "microsoft/msixsdk/microsoft.msixsdk-1.7.32.yaml" };

    REQUIRE_THROWS_HR(index.RemoveManifest(manifestFile, manifestPath), E_NOT_SET);
}

TEST_CASE("SQLiteIndex_RemoveManifest", "[sqliteindex][V1_0]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string manifest1Path = "test/id/test.id-1.0.0.yaml";
    Manifest manifest1;
    manifest1.Installers.push_back({});
    manifest1.Id = "test.id";
    manifest1.DefaultLocalization.Add<Localization::PackageName>("Test Name");
    manifest1.Moniker = "testmoniker";
    manifest1.Version = "1.0.0";
    manifest1.Channel = "test";
    manifest1.DefaultLocalization.Add<Localization::Tags>({ "t1", "t2" });
    manifest1.Installers[0].Commands = { "test1", "test2" };

    std::string manifest2Path = "test/woah/test.id-1.0.0.yaml";
    Manifest manifest2;
    manifest2.Installers.push_back({});
    manifest2.Id = "test.woah";
    manifest2.DefaultLocalization.Add<Localization::PackageName>("Test Name WOAH");
    manifest2.Moniker = "testmoniker";
    manifest2.Version = "1.0.0";
    manifest2.Channel = "test";
    manifest2.DefaultLocalization.Add<Localization::Tags>({});
    manifest2.Installers[0].Commands = { "test1", "test2", "test3" };

    {
        SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, { 1, 0 });

        index.AddManifest(manifest1, manifest1Path);
        index.AddManifest(manifest2, manifest2Path);

        // Now remove manifest1
        index.RemoveManifest(manifest1, manifest1Path);
    }

    {
        // Open it directly to directly test table state
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);

        REQUIRE(!Schema::V1_0::ManifestTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::IdTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::NameTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::MonikerTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::VersionTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::ChannelTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::PathPartTable::IsEmpty(connection));
        // Because manifest2 had no tags
        REQUIRE(Schema::V1_0::TagsTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::CommandsTable::IsEmpty(connection));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);

        // Now remove manifest2
        index.RemoveManifest(manifest2, manifest2Path);
    }

    // Open it directly to directly test table state
    Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);

    REQUIRE(Schema::V1_0::ManifestTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::IdTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::NameTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::MonikerTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::VersionTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::ChannelTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::PathPartTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::TagsTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::CommandsTable::IsEmpty(connection));
}

TEST_CASE("SQLiteIndex_RemoveManifestWithDependencies", "[sqliteindex][V1_4]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest dependencyManifest1, dependencyManifest2, manifest;
    SQLiteIndex index = SimpleTestSetup(tempFile, dependencyManifest1, SQLiteVersion::Latest());

    auto& publisher2 = "Test2";
    CreateFakeManifest(dependencyManifest2, publisher2);
    index.AddManifest(dependencyManifest2, GetPathFromManifest(dependencyManifest2));

    auto& publisher3 = "Test3";
    CreateFakeManifest(manifest, publisher3);
    manifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, dependencyManifest1.Id, "1.0.0"));
    manifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, dependencyManifest2.Id, "1.0.0"));

    index.AddManifest(manifest, GetPathFromManifest(manifest));

    index.RemoveManifest(manifest, GetPathFromManifest(manifest));
}

TEST_CASE("SQLiteIndex_ValidateManifestWithDependencies", "[sqliteindex][V1_4]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest levelOneManifest, levelTwoManifest, levelThreeManifest, topLevelManifest;
    SQLiteIndex index = SimpleTestSetup(tempFile, levelThreeManifest, SQLiteVersion::Latest());

    constexpr std::string_view levelTwoManifestPublisher = "LevelTwoManifest";
    CreateFakeManifest(levelTwoManifest, levelTwoManifestPublisher);

    levelTwoManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelThreeManifest.Id, "1.0.0"));
    index.AddManifest(levelTwoManifest, GetPathFromManifest(levelTwoManifest));

    constexpr std::string_view levelOneManifestPublisher = "LevelOneManifest";
    CreateFakeManifest(levelOneManifest, levelOneManifestPublisher);
    levelOneManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelTwoManifest.Id, "1.0.0"));
    index.AddManifest(levelOneManifest, GetPathFromManifest(levelOneManifest));

    constexpr std::string_view topLevelManifestPublisher = "TopLevelManifest";
    CreateFakeManifest(topLevelManifest, topLevelManifestPublisher);
    topLevelManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelOneManifest.Id, "1.0.0"));
    REQUIRE(PackageDependenciesValidation::ValidateManifestDependencies(&index, topLevelManifest));
}

TEST_CASE("SQLiteIndex_ValidateManifestWithDependenciesHasLoops", "[sqliteindex][V1_4]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest levelOneManifest, levelTwoManifest, levelThreeManifest, topLevelManifest;
    SQLiteIndex index = SimpleTestSetup(tempFile, levelThreeManifest, SQLiteVersion::Latest());

    constexpr std::string_view levelTwoManifestPublisher = "LevelTwoManifest";
    CreateFakeManifest(levelTwoManifest, levelTwoManifestPublisher);

    levelTwoManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelThreeManifest.Id, "1.0.0"));
    index.AddManifest(levelTwoManifest, GetPathFromManifest(levelTwoManifest));

    constexpr std::string_view levelOneManifestPublisher = "LevelOneManifest";
    CreateFakeManifest(levelOneManifest, levelOneManifestPublisher);
    levelOneManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelTwoManifest.Id, "1.0.0"));
    index.AddManifest(levelOneManifest, GetPathFromManifest(levelOneManifest));

    constexpr std::string_view topLevelManifestPublisher = "TopLevelManifest";
    CreateFakeManifest(topLevelManifest, topLevelManifestPublisher);
    topLevelManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelOneManifest.Id, "1.0.0"));
    index.AddManifest(topLevelManifest, GetPathFromManifest(topLevelManifest));

    levelThreeManifest.Installers.push_back(ManifestInstaller{});
    levelThreeManifest.Installers[1].Dependencies.Add(Dependency(DependencyType::Package, topLevelManifest.Id, "1.0.0"));
    REQUIRE_THROWS_HR(
        PackageDependenciesValidation::ValidateManifestDependencies(&index, levelThreeManifest),
        APPINSTALLER_CLI_ERROR_DEPENDENCIES_VALIDATION_FAILED);
}

TEST_CASE("SQLiteIndex_ValidateManifestWithDependenciesMissingNode", "[sqliteindex][V1_4]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest levelOneManifest, levelTwoManifest, levelThreeManifest, topLevelManifest;
    SQLiteIndex index = SimpleTestSetup(tempFile, levelThreeManifest, SQLiteVersion::Latest());

    constexpr std::string_view levelTwoManifestPublisher = "LevelTwoManifest";
    CreateFakeManifest(levelTwoManifest, levelTwoManifestPublisher);

    levelTwoManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelThreeManifest.Id, "1.0.0"));
    index.AddManifest(levelTwoManifest, GetPathFromManifest(levelTwoManifest));

    // This node is missing, because it's not in the index.
    constexpr std::string_view levelOneManifestPublisher = "LevelOneManifest";
    CreateFakeManifest(levelOneManifest, levelOneManifestPublisher);
    levelOneManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelTwoManifest.Id, "1.0.0"));

    constexpr std::string_view topLevelManifestPublisher = "TopLevelManifest";
    CreateFakeManifest(topLevelManifest, topLevelManifestPublisher);
    topLevelManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelOneManifest.Id, "1.0.0"));
    REQUIRE_THROWS_HR(
        PackageDependenciesValidation::ValidateManifestDependencies(&index, topLevelManifest),
        APPINSTALLER_CLI_ERROR_DEPENDENCIES_VALIDATION_FAILED);
}

TEST_CASE("SQLiteIndex_ValidateManifestWithDependenciesNoSuitableMinVersion", "[sqliteindex][V1_4]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest levelOneManifest, levelTwoManifest, levelThreeManifest, topLevelManifest;
    SQLiteIndex index = SimpleTestSetup(tempFile, levelThreeManifest, SQLiteVersion::Latest());

    constexpr std::string_view levelTwoManifestPublisher = "LevelTwoManifest";
    CreateFakeManifest(levelTwoManifest, levelTwoManifestPublisher);

    levelTwoManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelThreeManifest.Id, "1.0.0"));
    index.AddManifest(levelTwoManifest, GetPathFromManifest(levelTwoManifest));

    constexpr std::string_view levelOneManifestPublisher = "LevelOneManifest";
    CreateFakeManifest(levelOneManifest, levelOneManifestPublisher);
    levelOneManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelTwoManifest.Id, "1.0.0"));
    index.AddManifest(levelOneManifest, GetPathFromManifest(levelOneManifest));

    constexpr std::string_view topLevelManifestPublisher = "TopLevelManifest";
    CreateFakeManifest(topLevelManifest, topLevelManifestPublisher);
    topLevelManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelOneManifest.Id, "2.0.0"));

    REQUIRE_THROWS_HR(
        PackageDependenciesValidation::ValidateManifestDependencies(&index, topLevelManifest),
        APPINSTALLER_CLI_ERROR_DEPENDENCIES_VALIDATION_FAILED);
}

TEST_CASE("SQLiteIndex_ValidateManifestWhenManifestIsDependency_StructureBroken", "[sqliteindex][V1_4]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest levelOneManifest, levelTwoManifest, levelThreeManifest, topLevelManifest;
    SQLiteIndex index = SimpleTestSetup(tempFile, levelThreeManifest, SQLiteVersion::Latest());

    constexpr std::string_view levelTwoManifestPublisher = "LevelTwoManifest";
    CreateFakeManifest(levelTwoManifest, levelTwoManifestPublisher);

    levelTwoManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelThreeManifest.Id, "1.0.0"));
    index.AddManifest(levelTwoManifest, GetPathFromManifest(levelTwoManifest));

    constexpr std::string_view levelOneManifestPublisher = "LevelOneManifest";
    CreateFakeManifest(levelOneManifest, levelOneManifestPublisher);
    levelOneManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelTwoManifest.Id, "1.0.0"));
    index.AddManifest(levelOneManifest, GetPathFromManifest(levelOneManifest));

    constexpr std::string_view topLevelManifestPublisher = "TopLevelManifest";
    CreateFakeManifest(topLevelManifest, topLevelManifestPublisher);
    topLevelManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelOneManifest.Id, "1.0.0"));
    index.AddManifest(topLevelManifest, GetPathFromManifest(topLevelManifest));

    REQUIRE_THROWS_HR(
        PackageDependenciesValidation::VerifyDependenciesStructureForManifestDelete(&index, levelThreeManifest),
        APPINSTALLER_CLI_ERROR_DEPENDENCIES_VALIDATION_FAILED);
}

TEST_CASE("SQLiteIndex_ValidateManifestWhenManifestIsDependency_StructureNotBroken", "[sqliteindex][V1_4]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest levelOneManifest, levelTwoManifest, levelThreeManifest, topLevelManifest, levelThreeManifestV2;
    SQLiteIndex index = SimpleTestSetup(tempFile, levelThreeManifest, SQLiteVersion::Latest());

    constexpr std::string_view levelTwoManifestPublisher = "LevelTwoManifest";
    CreateFakeManifest(levelTwoManifest, levelTwoManifestPublisher);

    levelTwoManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelThreeManifest.Id, "1.0.0"));
    index.AddManifest(levelTwoManifest, GetPathFromManifest(levelTwoManifest));

    constexpr std::string_view levelOneManifestPublisher = "LevelOneManifest";
    CreateFakeManifest(levelOneManifest, levelOneManifestPublisher);
    levelOneManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelTwoManifest.Id, "1.0.0"));
    index.AddManifest(levelOneManifest, GetPathFromManifest(levelOneManifest));

    constexpr std::string_view topLevelManifestPublisher = "TopLevelManifest";
    CreateFakeManifest(topLevelManifest, topLevelManifestPublisher);
    topLevelManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelOneManifest.Id, "1.0.0"));
    index.AddManifest(topLevelManifest, GetPathFromManifest(topLevelManifest));

    constexpr std::string_view levelThreeManifestV2Publisher = "Test";
    CreateFakeManifest(levelThreeManifestV2, levelThreeManifestV2Publisher, "2.0.0");
    index.AddManifest(levelThreeManifestV2, GetPathFromManifest(levelThreeManifestV2));

    REQUIRE(PackageDependenciesValidation::VerifyDependenciesStructureForManifestDelete(&index, levelThreeManifest));
}

TEST_CASE("SQLiteIndex_ValidateManifestWhenManifestIsDependency_StructureBroken_NoSuitableOldManifest", "[sqliteindex][V1_4]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest levelOneManifest, levelTwoManifest, levelThreeManifest, topLevelManifest, levelThreeManifestV2;
    SQLiteIndex index = SimpleTestSetup(tempFile, levelThreeManifest, SQLiteVersion::Latest());

    constexpr std::string_view levelThreeManifestV2Publisher = "Test";
    CreateFakeManifest(levelThreeManifestV2, levelThreeManifestV2Publisher, "2.0.0");
    index.AddManifest(levelThreeManifestV2, GetPathFromManifest(levelThreeManifestV2));

    constexpr std::string_view levelTwoManifestPublisher = "LevelTwoManifest";
    CreateFakeManifest(levelTwoManifest, levelTwoManifestPublisher);

    levelTwoManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelThreeManifest.Id, "2.0.0"));
    index.AddManifest(levelTwoManifest, GetPathFromManifest(levelTwoManifest));

    constexpr std::string_view levelOneManifestPublisher = "LevelOneManifest";
    CreateFakeManifest(levelOneManifest, levelOneManifestPublisher);
    levelOneManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelTwoManifest.Id, "1.0.0"));
    index.AddManifest(levelOneManifest, GetPathFromManifest(levelOneManifest));

    constexpr std::string_view topLevelManifestPublisher = "TopLevelManifest";
    CreateFakeManifest(topLevelManifest, topLevelManifestPublisher);
    topLevelManifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, levelOneManifest.Id, "1.0.0"));
    index.AddManifest(topLevelManifest, GetPathFromManifest(topLevelManifest)); 

    REQUIRE_THROWS(
        PackageDependenciesValidation::VerifyDependenciesStructureForManifestDelete(&index, levelThreeManifestV2),
        APPINSTALLER_CLI_ERROR_DEPENDENCIES_VALIDATION_FAILED);
}

TEST_CASE("SQLiteIndex_RemoveManifest_EnsureConsistentRowId", "[sqliteindex][V1_7]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string manifest1Path = "test/id/test.id-1.0.0.yaml";
    Manifest manifest1;
    manifest1.Installers.push_back({});
    manifest1.Id = "test.id";
    manifest1.DefaultLocalization.Add<Localization::PackageName>("Test Name");
    manifest1.Moniker = "testmoniker";
    manifest1.Version = "1.0.0";
    manifest1.Channel = "test";
    manifest1.DefaultLocalization.Add<Localization::Tags>({ "t1", "t2" });
    manifest1.Installers[0].Commands = { "test1", "test2" };

    std::string manifest2Path = "test/woah/test.id-1.0.0.yaml";
    Manifest manifest2;
    manifest2.Installers.push_back({});
    manifest2.Id = "test.woah";
    manifest2.DefaultLocalization.Add<Localization::PackageName>("Test Name WOAH");
    manifest2.Moniker = "testmoniker";
    manifest2.Version = "1.0.0";
    manifest2.Channel = "test";
    manifest2.DefaultLocalization.Add<Localization::Tags>({});
    manifest2.Installers[0].Commands = { "test1", "test2", "test3" };

    SQLiteIndex index = CreateTestIndex(tempFile, SQLiteVersion{ 1, 7 });

    index.AddManifest(manifest1, manifest1Path);
    index.AddManifest(manifest2, manifest2Path);

    // Get the second manifest's id for validating consistency
    SearchRequest request;
    request.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, manifest2.Id));
    auto result = index.Search(request);

    REQUIRE(result.Matches.size() == 1);
    auto manifest2IdRowId = result.Matches[0].first;

    auto rowId = index.GetManifestIdByKey(manifest2IdRowId, {}, {});
    REQUIRE(rowId);
    auto manifest2RowId = rowId.value();

    // Now remove manifest1 and prepare
    index.RemoveManifest(manifest1, manifest1Path);
    index.PrepareForPackaging();

    // Checking consistency will also uncover issues, but not potentially the same ones as below.
    REQUIRE(index.CheckConsistency(true));

    // Repeat search to ensure consistent ids
    result = index.Search(request);
    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].first == manifest2IdRowId);

    rowId = index.GetManifestIdByKey(manifest2IdRowId, {}, {});
    REQUIRE(rowId);
    REQUIRE(rowId.value() == manifest2RowId);

    REQUIRE(manifest2.Id == index.GetPropertyByPrimaryId(manifest2RowId, PackageVersionProperty::Id));
    REQUIRE(manifest2.DefaultLocalization.Get<Localization::PackageName>() == index.GetPropertyByPrimaryId(manifest2RowId, PackageVersionProperty::Name));
    REQUIRE(manifest2.Moniker == index.GetPropertyByPrimaryId(manifest2RowId, PackageVersionProperty::Moniker));
    REQUIRE(manifest2.Version == index.GetPropertyByPrimaryId(manifest2RowId, PackageVersionProperty::Version));
    REQUIRE(manifest2.Channel == index.GetPropertyByPrimaryId(manifest2RowId, PackageVersionProperty::Channel));
    REQUIRE(manifest2Path == index.GetPropertyByPrimaryId(manifest2RowId, PackageVersionProperty::RelativePath));
}

TEST_CASE("SQLiteIndex_RemoveManifestFile", "[sqliteindex][V1_0]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    {
        SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, { 1, 0 });

        TestDataFile manifestFile{ "Manifest-Good.yaml" };
        std::filesystem::path manifestPath{ "microsoft/msixsdk/microsoft.msixsdk-1.7.32.yaml" };

        index.AddManifest(manifestFile, manifestPath);

        // Now remove that manifest
        index.RemoveManifest(manifestFile, manifestPath);
    }

    // Open it directly to directly test table state
    Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);

    REQUIRE(Schema::V1_0::ManifestTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::IdTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::NameTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::MonikerTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::VersionTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::ChannelTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::PathPartTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::TagsTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::CommandsTable::IsEmpty(connection));
}

TEST_CASE("SQLiteIndex_UpdateManifest", "[sqliteindex][V1_4]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string manifestPath = "test/id/test.id-1.0.0.yaml";
    Manifest manifest;
    manifest.Installers.push_back({});
    manifest.Id = "test.id";
    manifest.DefaultLocalization.Add < Localization::PackageName>("Test Name");
    manifest.Moniker = "testmoniker";
    manifest.Version = "1.0.0";
    manifest.Channel = "test";
    manifest.DefaultLocalization.Add<Localization::Tags>({ "t1", "t2" });
    manifest.Installers[0].Commands = { "test1", "test2" };

    
    {
        auto version = GENERATE(SQLiteVersion{ 1, 0 }, SQLiteVersion::Latest());
        SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, version);

        index.AddManifest(manifest, manifestPath);
    }

    {
        // Open it directly to directly test table state
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);

        REQUIRE(!Schema::V1_0::ManifestTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::IdTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::NameTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::MonikerTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::VersionTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::ChannelTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::PathPartTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::TagsTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::CommandsTable::IsEmpty(connection));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);

        // Update with no updates should return false
        REQUIRE(!index.UpdateManifest(manifest, manifestPath));

        manifest.DefaultLocalization.Add<Localization::Description>("description2");

        // Update with no indexed updates should return false
        REQUIRE(!index.UpdateManifest(manifest, manifestPath));

        // Update with indexed changes
        manifest.DefaultLocalization.Add<Localization::PackageName>("Test Name2");
        manifest.Moniker = "testmoniker2";
        manifest.DefaultLocalization.Add<Localization::Tags>({ "t1", "t2", "t3" });
        manifest.Installers[0].Commands = {};

        REQUIRE(index.UpdateManifest(manifest, manifestPath));
    }

    {
        // Open it directly to directly test table state
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);

        REQUIRE(!Schema::V1_0::ManifestTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::IdTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::NameTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::MonikerTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::VersionTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::ChannelTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::PathPartTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::TagsTable::IsEmpty(connection));
        // The update removed all commands
        REQUIRE(Schema::V1_0::CommandsTable::IsEmpty(connection));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);

        // Now remove manifest2
        index.RemoveManifest(manifest, manifestPath);
    }

    // Open it directly to directly test table state
    Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);

    REQUIRE(Schema::V1_0::ManifestTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::IdTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::NameTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::MonikerTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::VersionTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::ChannelTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::PathPartTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::TagsTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::CommandsTable::IsEmpty(connection));
}

TEST_CASE("SQLiteIndex_UpdateManifestWithDependencies", "[sqliteindex][V1_4]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest dependencyManifest1, dependencyManifest2, manifest, updateManifest;
    SQLiteIndex index = SimpleTestSetup(tempFile, dependencyManifest1, SQLiteVersion::Latest());

    auto& publisher2 = "Test2";
    CreateFakeManifest(dependencyManifest2, publisher2);
    index.AddManifest(dependencyManifest2, GetPathFromManifest(dependencyManifest2));

    auto& publisher3 = "Test3";
    CreateFakeManifest(manifest, publisher3);
    const std::string dependencyPath3 = GetPathFromManifest(manifest);

    manifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, dependencyManifest1.Id, "1.0.0"));
    manifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, dependencyManifest2.Id, "1.0.0"));

    index.AddManifest(manifest, dependencyPath3);

    auto& publisher4 = "Test4";
    CreateFakeManifest(updateManifest, publisher4);
    index.AddManifest(updateManifest, GetPathFromManifest(updateManifest));
    manifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, updateManifest.Id, "1.0.0"));

    REQUIRE(index.UpdateManifest(manifest, dependencyPath3));
}

TEST_CASE("SQLiteIndex_UpdateManifestWithDependenciesDeleteAndAdd", "[sqliteindex][V1_4]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest dependencyManifest1, dependencyManifest2, manifest, updateManifest;
    SQLiteIndex index = SimpleTestSetup(tempFile, dependencyManifest1, SQLiteVersion::Latest());

    auto& publisher2 = "Test2";
    CreateFakeManifest(dependencyManifest2, publisher2);
    index.AddManifest(dependencyManifest2, GetPathFromManifest(dependencyManifest2));

    auto& publisher3 = "Test3";
    CreateFakeManifest(manifest, publisher3);
    const std::string dependencyPath3 = GetPathFromManifest(manifest);

    manifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, dependencyManifest1.Id, "1.0.0"));
    manifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, dependencyManifest2.Id, "1.0.0"));

    index.AddManifest(manifest, dependencyPath3);

    manifest.Installers[0].Dependencies.Clear();

    auto& publisher4 = "Test4";
    CreateFakeManifest(updateManifest, publisher4);
    index.AddManifest(updateManifest, GetPathFromManifest(updateManifest));
    manifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, updateManifest.Id, "1.0.0"));

    REQUIRE(index.UpdateManifest(manifest, dependencyPath3));
}

TEST_CASE("SQLiteIndex_UpdateManifestChangePath", "[sqliteindex][V1_0]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string manifestPath = "test/id/test.id-1.0.0.yaml";
    Manifest manifest;
    manifest.Installers.push_back({});
    manifest.Id = "test.id";
    manifest.DefaultLocalization.Add<Localization::PackageName>("Test Name");
    manifest.Moniker = "testmoniker";
    manifest.Version = "1.0.0";
    manifest.Channel = "test";
    manifest.DefaultLocalization.Add<Localization::Tags>({ "t1", "t2" });
    manifest.Installers[0].Commands = { "test1", "test2" };

    {
        SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, { 1, 0 });

        index.AddManifest(manifest, manifestPath);
    }

    {
        // Open it directly to directly test table state
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);

        REQUIRE(!Schema::V1_0::ManifestTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::IdTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::NameTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::MonikerTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::VersionTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::ChannelTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::PathPartTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::TagsTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::CommandsTable::IsEmpty(connection));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);

        manifestPath = "test/newid/test.newid-1.0.0.yaml";

        // Update with path update should indicate change
        REQUIRE(index.UpdateManifest(manifest, manifestPath));
    }

    {
        // Open it directly to directly test table state
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);

        REQUIRE(!Schema::V1_0::ManifestTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::IdTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::NameTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::MonikerTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::VersionTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::ChannelTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::PathPartTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::TagsTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::CommandsTable::IsEmpty(connection));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);

        // Now remove manifest, with unknown path
        index.RemoveManifest(manifest, "");
    }

    // Open it directly to directly test table state
    Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);

    REQUIRE(Schema::V1_0::ManifestTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::IdTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::NameTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::MonikerTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::VersionTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::ChannelTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::PathPartTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::TagsTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::CommandsTable::IsEmpty(connection));
}

TEST_CASE("SQLiteIndex_UpdateManifest_Pathless", "[sqliteindex][V1_0]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest manifest;
    manifest.Installers.push_back({});
    manifest.Id = "test.id";
    manifest.DefaultLocalization.Add < Localization::PackageName>("Test Name");
    manifest.Moniker = "testmoniker";
    manifest.Version = "1.0.0";
    manifest.Channel = "test";
    manifest.DefaultLocalization.Add<Localization::Tags>({ "t1", "t2" });
    manifest.Installers[0].Commands = { "test1", "test2" };

    {
        SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, { 1, 0 });

        index.AddManifest(manifest);
    }

    {
        // Open it directly to directly test table state
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);

        REQUIRE(!Schema::V1_0::ManifestTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::IdTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::NameTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::MonikerTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::VersionTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::ChannelTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::PathPartTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::TagsTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::CommandsTable::IsEmpty(connection));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);

        // Update with no updates should return false
        REQUIRE(!index.UpdateManifest(manifest));

        manifest.DefaultLocalization.Add<Localization::Description>("description2");

        // Update with no indexed updates should return false
        REQUIRE(!index.UpdateManifest(manifest));

        // Update with indexed changes
        manifest.DefaultLocalization.Add<Localization::PackageName>("Test Name2");
        manifest.Moniker = "testmoniker2";
        manifest.DefaultLocalization.Add<Localization::Tags>({ "t1", "t2", "t3" });
        manifest.Installers[0].Commands = {};

        REQUIRE(index.UpdateManifest(manifest));
    }

    {
        // Open it directly to directly test table state
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);

        REQUIRE(!Schema::V1_0::ManifestTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::IdTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::NameTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::MonikerTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::VersionTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::ChannelTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::PathPartTable::IsEmpty(connection));
        REQUIRE(!Schema::V1_0::TagsTable::IsEmpty(connection));
        // The update removed all commands
        REQUIRE(Schema::V1_0::CommandsTable::IsEmpty(connection));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);

        // Now remove manifest2
        index.RemoveManifest(manifest);
    }

    // Open it directly to directly test table state
    Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);

    REQUIRE(Schema::V1_0::ManifestTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::IdTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::NameTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::MonikerTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::VersionTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::ChannelTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::TagsTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::CommandsTable::IsEmpty(connection));
}

TEST_CASE("SQLiteIndex_UpdateManifestChangeCase", "[sqliteindex][V1_0]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string manifestPath = "test/id/test.id-1.0.0.yaml";
    Manifest manifest;
    manifest.Installers.push_back({});
    manifest.Id = "test.id";
    manifest.DefaultLocalization.Add<Localization::PackageName>("Test Name");
    manifest.Moniker = "testmoniker";
    manifest.Version = "1.0.0-test";
    manifest.Channel = "test";
    manifest.DefaultLocalization.Add<Localization::Tags>({ "t1", "t2" });
    manifest.Installers[0].Commands = { "test1", "test2" };

    {
        SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, { 1, 0 });

        index.AddManifest(manifest, manifestPath);
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);

        manifest.Id = "Test.Id";

        // Update with path update should indicate change
        REQUIRE(index.UpdateManifest(manifest, manifestPath));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);

        manifest.Version = "1.0.0-Test";

        // Update with path update should indicate change
        REQUIRE(index.UpdateManifest(manifest, manifestPath));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);

        manifest.Channel = "Test";

        // Update with path update should indicate change
        REQUIRE(index.UpdateManifest(manifest, manifestPath));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);

        manifest.DefaultLocalization.Add<Localization::PackageName>("test name");

        // Update with path update should indicate change
        REQUIRE(index.UpdateManifest(manifest, manifestPath));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);

        // Now remove manifest, with unknown path
        index.RemoveManifest(manifest, "");
    }

    // Open it directly to directly test table state
    Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);

    REQUIRE(Schema::V1_0::ManifestTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::IdTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::NameTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::MonikerTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::VersionTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::ChannelTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::PathPartTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::TagsTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::CommandsTable::IsEmpty(connection));
}

TEST_CASE("SQLiteIndex_IdCaseInsensitivity", "[sqliteindex][V1_0]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string manifest1Path = "test/id/test.id-1.0.0.yaml";
    Manifest manifest1;
    manifest1.Installers.push_back({});
    manifest1.Id = "test.id";
    manifest1.DefaultLocalization.Add<Localization::PackageName>("Test Name");
    manifest1.Moniker = "testmoniker";
    manifest1.Version = "1.0.0";
    manifest1.DefaultLocalization.Add<Localization::Tags>({ "t1", "t2" });
    manifest1.Installers[0].Commands = { "test1", "test2" };

    std::string manifest2Path = "test/id/test.id-2.0.0.yaml";
    Manifest manifest2 = manifest1;
    manifest2.Id = "Test.Id";
    manifest1.Version = "2.0.0";

    {
        SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, { 1, 0 });

        index.AddManifest(manifest1, manifest1Path);

        auto results = index.Search({});
        REQUIRE(results.Matches.size() == 1);
        REQUIRE(manifest1.Id == GetIdStringById(index, results.Matches[0].first));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);

        index.AddManifest(manifest2, manifest2Path);

        auto results = index.Search({});
        REQUIRE(results.Matches.size() == 1);
        REQUIRE(manifest2.Id == GetIdStringById(index, results.Matches[0].first));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);

        manifest1.Id = "TEST.ID";

        REQUIRE(index.UpdateManifest(manifest1, manifest1Path));

        auto results = index.Search({});
        REQUIRE(results.Matches.size() == 1);
        REQUIRE(manifest1.Id == GetIdStringById(index, results.Matches[0].first));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);

        index.RemoveManifest(manifest1, manifest1Path);

        auto results = index.Search({});
        REQUIRE(results.Matches.size() == 1);
        REQUIRE(manifest1.Id == GetIdStringById(index, results.Matches[0].first));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);

        index.RemoveManifest(manifest2, manifest2Path);

        auto results = index.Search({});
        REQUIRE(results.Matches.empty());
    }

    // Open it directly to directly test table state
    Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);

    REQUIRE(Schema::V1_0::ManifestTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::IdTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::NameTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::MonikerTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::VersionTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::ChannelTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::PathPartTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::TagsTable::IsEmpty(connection));
    REQUIRE(Schema::V1_0::CommandsTable::IsEmpty(connection));
}

TEST_CASE("PathPartTable_EnsurePathExists_Negative_Paths", "[sqliteindex][V1_0]")
{
    // Open it directly to directly test pathpart table
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    REQUIRE_THROWS_HR(Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"()", false), E_INVALIDARG);
    REQUIRE_THROWS_HR(Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(\)", false), E_INVALIDARG);
    REQUIRE_THROWS_HR(Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(/)", false), E_INVALIDARG);
    REQUIRE_THROWS_HR(Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(C:)", false), E_INVALIDARG);
    REQUIRE_THROWS_HR(Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(C:\\)", false), E_INVALIDARG);
    REQUIRE_THROWS_HR(Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(C:\temp\path\file.txt)", false), E_INVALIDARG);
    REQUIRE_THROWS_HR(Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(\temp\path\file.txt)", false), E_INVALIDARG);
}

TEST_CASE("PathPartTable_EnsurePathExists", "[sqliteindex][V1_0]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    // Create the index
    {
        SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, { 1, 0 });
        SQLiteVersion versionCreated = index.GetVersion();
        REQUIRE(versionCreated == SQLiteVersion{ 1, 0 });
    }

    // Open it directly to directly test pathpart table
    Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);

    // attempt to find path that doesn't exist
    auto result0 = Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(a\b\c.txt)", false);
    REQUIRE(!std::get<0>(result0));

    // add path
    auto result1 = Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(a\b\c.txt)", true);
    REQUIRE(std::get<0>(result1));

    // Second time trying to create should return false and same id
    auto result2 = Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(a\b\c.txt)", true);
    REQUIRE(!std::get<0>(result2));
    REQUIRE(std::get<1>(result1) == std::get<1>(result2));

    // Trying to find but not create should return true because it exists
    auto result3 = Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(a\b\c.txt)", false);
    REQUIRE(std::get<0>(result3));
    REQUIRE(std::get<1>(result1) == std::get<1>(result3));

    // attempt to find a different file
    auto result4 = Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(a\b\d.txt)", false);
    REQUIRE(!std::get<0>(result4));

    // add a different file
    auto result5 = Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(a\b\d.txt)", true);
    REQUIRE(std::get<0>(result5));
    REQUIRE(std::get<1>(result1) != std::get<1>(result5));

    // add the same file but deeper
    auto result6 = Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(a\b\d\c.txt)", true);
    REQUIRE(std::get<0>(result6));
    REQUIRE(std::get<1>(result1) != std::get<1>(result6));

    // get the deeper file with extra separators
    auto result7 = Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(a\\b\d\\c.txt)", true);
    REQUIRE(!std::get<0>(result7));
    REQUIRE(std::get<1>(result6) == std::get<1>(result7));
}

TEST_CASE("SQLiteIndex_PrepareForPackaging", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = CreateTestIndex(tempFile);

    TestDataFile manifestFile{ "Manifest-Good.yaml" };
    std::filesystem::path manifestPath{ "microsoft/msixsdk/microsoft.msixsdk-1.7.32.yaml" };

    index.AddManifest(manifestFile, manifestPath);

    index.PrepareForPackaging();
}

TEST_CASE("SQLiteIndex_Search_IdExactMatch", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest manifest;
    std::string relativePath = "test/id/1.0.0.yaml";
    SQLiteIndex index = SimpleTestSetup(tempFile, manifest);

    TestPrepareForRead(index);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, manifest.Id);

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);
    REQUIRE(results.Matches[0].second.Field == PackageMatchField::Id);
    REQUIRE(results.Matches[0].second.Type == MatchType::Exact);
    REQUIRE(results.Matches[0].second.Value == manifest.Id);
}

TEST_CASE("SQLiteIndex_Search_MultipleMatch", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest manifest;
    std::string relativePath = "test/id/1.0.0.yaml";
    SQLiteIndex index = SimpleTestSetup(tempFile, manifest);

    manifest.Version = "2.0.0";
    index.AddManifest(manifest, relativePath + "2");

    TestPrepareForRead(index);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, manifest.Id);

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    if (AreVersionKeysSupported(index))
    {
        auto result = index.GetVersionKeysById(results.Matches[0].first);
        REQUIRE(result.size() == 2);
    }
    else
    {
        REQUIRE_THROWS_HR(index.GetVersionKeysById(results.Matches[0].first), E_NOT_VALID_STATE);
    }
}

TEST_CASE("SQLiteIndex_Search_NoMatch", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest manifest;
    SQLiteIndex index = SimpleTestSetup(tempFile, manifest);

    TestPrepareForRead(index);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, "THIS DOES NOT MATCH ANYTHING!");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 0);
}

TEST_CASE("SQLiteIndex_IdString", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest manifest;
    SQLiteIndex index = SimpleTestSetup(tempFile, manifest);

    TestPrepareForRead(index);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, manifest.Id);

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto result = GetIdStringById(index, results.Matches[0].first);
    REQUIRE(result == manifest.Id);
}

TEST_CASE("SQLiteIndex_NameString", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest manifest;
    SQLiteIndex index = SimpleTestSetup(tempFile, manifest);

    TestPrepareForRead(index);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, manifest.Id);

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto result = GetNameStringById(index, results.Matches[0].first);
    REQUIRE(result == manifest.DefaultLocalization.Get<Localization::PackageName>());
}

TEST_CASE("SQLiteIndex_PathString", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest manifest;
    SQLiteIndex index = SimpleTestSetup(tempFile, manifest);
    auto relativePath = GetPathFromManifest(manifest);

    TestPrepareForRead(index);

    if (!AreManifestPathsSupported(index))
    {
        return;
    }

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, manifest.Id);

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto specificResult = GetPathStringByKey(index, results.Matches[0].first, manifest.Version, manifest.Channel);
    REQUIRE(specificResult == relativePath);

    auto latestResult = GetPathStringByKey(index, results.Matches[0].first, "", manifest.Channel);
    REQUIRE(latestResult == relativePath);
}

TEST_CASE("SQLiteIndex_PathlessString", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest manifest;
    std::string relativePath;

    SQLiteIndex index = CreateTestIndex(tempFile);
    CreateFakeManifest(manifest, "Test");
    index.AddManifest(manifest);

    TestPrepareForRead(index);

    if (!AreManifestPathsSupported(index))
    {
        return;
    }

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, manifest.Id);

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto specificResult = GetPathStringByKey(index, results.Matches[0].first, manifest.Version, manifest.Channel);
    REQUIRE(specificResult == relativePath);

    auto latestResult = GetPathStringByKey(index, results.Matches[0].first, "", manifest.Channel);
    REQUIRE(latestResult == relativePath);
}

TEST_CASE("SQLiteIndex_Versions", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest manifest;
    std::string relativePath = "test/id/1.0.0.yaml";
    SQLiteIndex index = SimpleTestSetup(tempFile, manifest);

    TestPrepareForRead(index);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, manifest.Id);

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    if (AreVersionKeysSupported(index))
    {
        auto result = index.GetVersionKeysById(results.Matches[0].first);
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].VersionAndChannel.GetVersion().ToString() == manifest.Version);
        REQUIRE(result[0].VersionAndChannel.GetChannel().ToString() == manifest.Channel);
    }
    else
    {
        REQUIRE_THROWS_HR(index.GetVersionKeysById(results.Matches[0].first), E_NOT_VALID_STATE);
    }
}

TEST_CASE("SQLiteIndex_Search_VersionSorting", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::vector<VersionAndChannel> sortedList =
    {
        { UtilityVersion("15.0.0"), Channel("") },
        { UtilityVersion("14.0.0"), Channel("") },
        { UtilityVersion("13.2.0"), Channel("") },
        { UtilityVersion("13.2.0-bugfix"), Channel("") },
        { UtilityVersion("13.0.0"), Channel("") },
        { UtilityVersion("16.0.0"), Channel("alpha") },
        { UtilityVersion("15.8.0"), Channel("alpha") },
        { UtilityVersion("15.1.0"), Channel("beta") },
    };

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id", "Name", "Moniker", "14.0.0", "", { "foot" }, { "com34" }, "Path1" },
        { "Id", "Name", "Moniker", "16.0.0", "alpha", { "floor" }, { "com3" }, "Path2" },
        { "Id", "Name", "Moniker", "15.0.0", "", {}, { "Command" }, "Path3" },
        { "Id", "Name", "Moniker", "13.2.0", "", {}, { "Command" }, "Path4" },
        { "Id", "Name", "Moniker", "15.1.0", "beta", { "foo" }, { "com3" }, "Path5" },
        { "Id", "Name", "Moniker", "15.8.0", "alpha", { "foo" }, { "com3" }, "Path6" },
        { "Id", "Name", "Moniker", "13.2.0-bugfix", "", { "foo" }, { "com3" }, "Path7" },
        { "Id", "Name", "Moniker", "13.0.0", "", { "foo" }, { "com3" }, "Path8" },
        });

    TestPrepareForRead(index);

    if (!AreChannelsSupported(index))
    {
        return;
    }

    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Id, MatchType::Exact, "Id");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto result = index.GetVersionKeysById(results.Matches[0].first);
    REQUIRE(result.size() == sortedList.size());

    for (size_t i = 0; i < result.size(); ++i)
    {
        const VersionAndChannel& sortedVAC = sortedList[i];
        const VersionAndChannel& resultVAC = result[i].VersionAndChannel;

        INFO(i);
        REQUIRE(sortedVAC.GetVersion().ToString() == resultVAC.GetVersion().ToString());
        REQUIRE(sortedVAC.GetChannel().ToString() == resultVAC.GetChannel().ToString());
    }
}

TEST_CASE("SQLiteIndex_PathString_VersionSorting", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::vector<VersionAndChannel> sortedList =
    {
        { UtilityVersion("15.0.0"), Channel("") },
        { UtilityVersion("14.0.0"), Channel("") },
        { UtilityVersion("13.2.0"), Channel("") },
        { UtilityVersion("13.2.0-bugfix"), Channel("") },
        { UtilityVersion("13.0.0"), Channel("") },
        { UtilityVersion("16.0.0"), Channel("alpha") },
        { UtilityVersion("15.8.0"), Channel("alpha") },
        { UtilityVersion("15.1.0"), Channel("beta") },
    };

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id", "Name", "Moniker", "14.0.0", "", { "foot" }, { "com34" }, "Path1" },
        { "Id", "Name", "Moniker", "16.0.0", "alpha", { "floor" }, { "com3" }, "Path2" },
        { "Id", "Name", "Moniker", "15.0.0", "", {}, { "Command" }, "Path3" },
        { "Id", "Name", "Moniker", "13.2.0", "", {}, { "Command" }, "Path4" },
        { "Id", "Name", "Moniker", "15.1.0", "beta", { "foo" }, { "com3" }, "Path5" },
        { "Id", "Name", "Moniker", "15.8.0", "alpha", { "foo" }, { "com3" }, "Path6" },
        { "Id", "Name", "Moniker", "13.2.0-bugfix", "", { "foo" }, { "com3" }, "Path7" },
        { "Id", "Name", "Moniker", "13.0.0", "", { "foo" }, { "com3" }, "Path8" },
        });

    TestPrepareForRead(index);

    if (!AreChannelsSupported(index))
    {
        return;
    }

    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Id, MatchType::Exact, "Id");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto result = GetPathStringByKey(index, results.Matches[0].first, "", "");
    REQUIRE(result == "Path3");

    result = GetPathStringByKey(index, results.Matches[0].first, "", "alpha");
    REQUIRE(result == "Path2");

    result = GetPathStringByKey(index, results.Matches[0].first, "", "beta");
    REQUIRE(result == "Path5");

    auto nonResult = index.GetManifestIdByKey(results.Matches[0].first, "", "gamma");
    REQUIRE(!nonResult.has_value());
}

TEST_CASE("SQLiteIndex_PathString_CaseInsensitive", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id", "Name", "Moniker", "14.0.0", "", { "foot" }, { "com34" }, "Path1" },
        { "Id", "Name", "Moniker", "16.0.0", "alpha", { "floor" }, { "com3" }, "Path2" },
        { "Id", "Name", "Moniker", "15.0.0", "", {}, { "Command" }, "Path3" },
        { "Id", "Name", "Moniker", "13.2.0-BUGFIX", "", {}, { "Command" }, "Path4" },
        { "Id", "Name", "Moniker", "15.1.0", "beta", { "foo" }, { "com3" }, "Path5" },
        { "Id", "Name", "Moniker", "15.8.0", "alpha", { "foo" }, { "com3" }, "Path6" },
        { "Id", "Name", "Moniker", "13.2.0-bugfix", "beta", { "foo" }, { "com3" }, "Path7" },
        { "Id", "Name", "Moniker", "13.0.0", "", { "foo" }, { "com3" }, "Path8" },
        });

    TestPrepareForRead(index);

    if (!AreChannelsSupported(index))
    {
        return;
    }

    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Id, MatchType::Exact, "Id");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto result = index.GetManifestIdByKey(results.Matches[0].first, "", "Alpha");
    REQUIRE(result.has_value());

    result = index.GetManifestIdByKey(results.Matches[0].first, "13.2.0-BugFix", "");
    REQUIRE(result.has_value());

    result = index.GetManifestIdByKey(results.Matches[0].first, "13.2.0-BugFix", "BETA");
    REQUIRE(result.has_value());
}

TEST_CASE("SQLiteIndex_SearchResultsTableSearches", "[sqliteindex][V1_0]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest manifest;
    {
        (void)SimpleTestSetup(tempFile, manifest, SQLiteVersion{ 1, 0 });
    }

    Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadOnly);
    Schema::V1_0::SearchResultsTable search(connection);

    std::string value = "test";

    // Perform every type of field and match search
    PackageMatchFilter filter(PackageMatchField::Id, MatchType::Exact, value);

    for (auto field : { PackageMatchField::Id, PackageMatchField::Name, PackageMatchField::Moniker, PackageMatchField::Tag, PackageMatchField::Command })
    {
        filter.Field = field;

        for (auto match : { MatchType::Exact, MatchType::Fuzzy, MatchType::FuzzySubstring, MatchType::Substring, MatchType::Wildcard })
        {
            filter.Type = match;
            search.SearchOnField(filter);
        }
    }
}

TEST_CASE("SQLiteIndex_Search_EmptySearch", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile,{
        { "Id1", "Name", "Moniker", "Version1", "Channel", { "Tag" }, { "Command" }, "Path1" },
        { "Id1", "Name", "Moniker", "Version2", "Channel", { "Tag" }, { "Command" }, "Path2" },
        { "Id2", "Name", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path3" },
        { "Id3", "Name", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path4" },
        });

    TestPrepareForRead(index);

    SearchRequest request;

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 3);
}

TEST_CASE("SQLiteIndex_Search_Exact", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id", "Name", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1" },
        { "Id2", "Name", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path2" },
        });

    TestPrepareForRead(index);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, "Id");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);
}

TEST_CASE("SQLiteIndex_Search_Substring", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id", "Name", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1" },
        { "Id2", "Name", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path2" },
        });

    TestPrepareForRead(index);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Substring, "Id");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 2);
}

TEST_CASE("SQLiteIndex_Search_ExactBeforeSubstring", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id2", "Name", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1" },
        { "Id", "Name", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path2" },
        });

    TestPrepareForRead(index);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Substring, "Id");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 2);

    REQUIRE(GetIdStringById(index, results.Matches[0].first) == "Id");
    REQUIRE(GetIdStringById(index, results.Matches[1].first) == "Id2");
}

TEST_CASE("SQLiteIndex_Search_SingleFilter", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id", "Name", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1" },
        { "Id2", "Na", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path2" },
        { "Id3", "No", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path3" },
        });

    TestPrepareForRead(index);

    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Name, MatchType::Substring, "a");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 2);

    request.Filters[0].Value = "e";

    results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);
}

TEST_CASE("SQLiteIndex_Search_Multimatch", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id1", "Name", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1" },
        { "Id1", "Name1", "Moniker", "Version1", "Channel", { "Tag" }, { "Command" }, "Path2" },
        { "Id2", "Name", "Moniker", "Version", "", { "Tag" }, { "Command" }, "Path3" },
        { "Id2", "Name", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path4" },
        { "Id3", "Name", "Moniker", "Version1", "", { "Tag" }, { "Command" }, "Path5" },
        { "Id3", "Name", "Moniker", "Version2", "", { "Tag" }, { "Command" }, "Path6" },
        { "Id3", "Name", "Moniker", "Version3", "", { "Tag" }, { "Command" }, "Path7" },
        });

    TestPrepareForRead(index);

    SearchRequest request;
    // An empty string should match all substrings
    request.Query = RequestMatch(MatchType::Substring, "");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 3);
}

TEST_CASE("SQLiteIndex_Search_QueryAndFilter", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Nope", "Name", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1" },
        { "Id2", "Na", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path2" },
        { "Id3", "No", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path3" },
        });

    TestPrepareForRead(index);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Substring, "Id");
    request.Filters.emplace_back(PackageMatchField::Name, MatchType::Substring, "Na");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto result = GetIdStringById(index, results.Matches[0].first);
    REQUIRE(result == "Id2");
}

TEST_CASE("SQLiteIndex_Search_QueryAndMultipleFilters", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id1", "Name", "Moniker", "Version", "Channel", { "foot" }, { "com34" }, "Path1" },
        { "Id1", "Name1", "Moniker", "Version1", "Channel", { "floor" }, { "com3" }, "Path2" },
        { "Id2", "Name", "Moniker", "Version", "", {}, { "Command" }, "Path3" },
        { "Id2", "Name", "Moniker", "Version", "Channel", {}, { "Command" }, "Path4" },
        { "Id3", "Tagit", "Moniker", "Version1", "", { "foo" }, { "com3" }, "Path5" },
        { "Id3", "Tagit", "Moniker", "Version2", "", { "foo" }, { "com3" }, "Path6" },
        { "Id3", "Tagit", "new", "Version3", "", { "foo" }, { "com3" }, "Path7" },
        });

    TestPrepareForRead(index);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Substring, "tag");
    request.Filters.emplace_back(PackageMatchField::Command, MatchType::Exact, "com3");
    request.Filters.emplace_back(PackageMatchField::Tag, MatchType::Substring, "foo");
    request.Filters.emplace_back(PackageMatchField::Moniker, MatchType::Substring, "new");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto result = GetIdStringById(index, results.Matches[0].first);
    REQUIRE(result == "Id3");
}

TEST_CASE("SQLiteIndex_Search_SimpleICULike", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    // Insert decomposed character: [upper] A + umlaut
    SQLiteIndex index = SearchTestSetup(tempFile, {
        { u8"\x41\x308wesomeApp", "HasUmlaut", "Moniker", "Version", "Channel", { "foot" }, { "com34" }, "Path1" },
        { u8"AwesomeApp", "Nope", "Moniker", "Version", "Channel", { "foot" }, { "com34" }, "Path2" },
        });

    TestPrepareForRead(index);

    SearchRequest request;
    // Search for anything containing: [lower] a + umlaut
    request.Filters.emplace_back(PackageMatchField::Id, MatchType::Substring, u8"\xE4");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto result = GetNameStringById(index, results.Matches[0].first);
    REQUIRE(result == "HasUmlaut");
}

TEST_CASE("SQLiteIndex_Search_MaximumResults_Equal", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Nope", "Name", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1" },
        { "Id2", "Na", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path2" },
        { "Id3", "No", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path3" },
        });

    TestPrepareForRead(index);

    SearchRequest request;
    request.MaximumResults = 3;

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 3);
    REQUIRE(!results.Truncated);
}

TEST_CASE("SQLiteIndex_Search_MaximumResults_Less", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Nope", "Name", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1" },
        { "Id2", "Na", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path2" },
        { "Id3", "No", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path3" },
        });

    TestPrepareForRead(index);

    SearchRequest request;
    request.MaximumResults = 2;

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 2);
    REQUIRE(results.Truncated);
}

TEST_CASE("SQLiteIndex_Search_MaximumResults_Greater", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Nope", "Name", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1" },
        { "Id2", "Na", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path2" },
        { "Id3", "No", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path3" },
        });

    TestPrepareForRead(index);

    SearchRequest request;
    request.MaximumResults = 4;

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 3);
    REQUIRE(!results.Truncated);
}

TEST_CASE("SQLiteIndex_Search_QueryAndInclusion", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Nope", "Name", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1" },
        { "Id2", "Na", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path2" },
        { "Id3", "No", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path3" },
        });

    TestPrepareForRead(index);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::CaseInsensitive, "id3");
    request.Inclusions.emplace_back(PackageMatchField::Name, MatchType::Substring, "Na");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 3);
}

TEST_CASE("SQLiteIndex_Search_InclusionOnly", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Nope", "Name", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1" },
        { "Id2", "Na", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path2" },
        { "Id3", "No", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path3" },
        });

    TestPrepareForRead(index);

    SearchRequest request;
    request.Inclusions.emplace_back(PackageMatchField::Name, MatchType::Substring, "Na");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 2);
}

TEST_CASE("SQLiteIndex_Search_InclusionAndFilter", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Nope", "Name", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1" },
        { "Id2", "Na", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path2" },
        { "Id3", "No", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path3" },
        });

    TestPrepareForRead(index);

    SearchRequest request;
    request.Inclusions.emplace_back(PackageMatchField::Name, MatchType::Substring, "Na");
    request.Filters.emplace_back(PackageMatchField::Name, MatchType::CaseInsensitive, "name");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto result = GetIdStringById(index, results.Matches[0].first);
    REQUIRE(result == "Nope");
}

TEST_CASE("SQLiteIndex_Search_QueryInclusionAndFilter", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Nope", "Name", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1" },
        { "Id2", "Na", "monicka", "Version", "Channel", { "Tag" }, { "Command" }, "Path2" },
        { "Id3", "No", "moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path3" },
        });

    TestPrepareForRead(index);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Substring, "id3");
    request.Inclusions.emplace_back(PackageMatchField::Name, MatchType::Substring, "na");
    request.Filters.emplace_back(PackageMatchField::Moniker, MatchType::CaseInsensitive, "MONIKER");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 2);
}

TEST_CASE("SQLiteIndex_Search_CaseInsensitive", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Nope", "id3", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1" },
        { "Id2", "Na", "Moniker", "Version", "Channel", { "ID3" }, { "Command" }, "Path2" },
        { "Id3", "No", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path3" },
        });

    TestPrepareForRead(index);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::CaseInsensitive, "id3");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 3);
}

TEST_CASE("SQLiteIndex_Search_StartsWith", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "NopeId", "id3", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1" },
        { "Id2", "Na", "Moniker", "Version", "Channel", { "ID3" }, { "Command" }, "Path2" },
        { "Id3", "No", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path3" },
        });

    TestPrepareForRead(index);

    SearchRequest request;
    request.Inclusions.push_back(PackageMatchFilter(PackageMatchField::Id, MatchType::StartsWith, "id"));

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 2);
}

TEST_CASE("SQLiteIndex_Search_Query_PackageFamilyNameSubstring", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id1", "Name1", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1", { "PFN1" }, { "PC1" } },
        { "Id2", "Name2", "Moniker", "Version", "Channel", { "ID3" }, { "Command" }, "Path2", { "PFN2" }, { "PC2" } },
        { "Id3", "Name3", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path3", { "PFN3" }, { "PC3" } },
        });

    SQLiteVersion testVersion = TestPrepareForRead(index);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Substring, "PFN");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 0);
}

TEST_CASE("SQLiteIndex_Search_Query_ProductCodeSubstring", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id1", "Name1", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1", { "PFN1" }, { "PC1" } },
        { "Id2", "Name2", "Moniker", "Version", "Channel", { "ID3" }, { "Command" }, "Path2", { "PFN2" }, { "PC2" } },
        { "Id3", "Name3", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path3", { "PFN3" }, { "PC3" } },
        });

    SQLiteVersion testVersion = TestPrepareForRead(index);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Substring, "PC");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 0);
}

TEST_CASE("SQLiteIndex_Search_Query_PackageFamilyNameMatch", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id1", "Name1", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1", { "PFN1" }, { "PC1" } },
        { "Id2", "Name2", "Moniker", "Version", "Channel", { "ID3" }, { "Command" }, "Path2", { "PFN2" }, { "PC2" } },
        { "Id3", "Name3", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path3", { "PFN3" }, { "PC3" } },
        });

    SQLiteVersion testVersion = TestPrepareForRead(index);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Substring, "pfn1");

    auto results = index.Search(request);

    if (ArePackageFamilyNameAndProductCodeSupported(index, testVersion))
    {
        REQUIRE(results.Matches.size() == 1);
    }
    else
    {
        REQUIRE(results.Matches.size() == 0);
    }
}

TEST_CASE("SQLiteIndex_Search_Query_ProductCodeMatch", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id1", "Name1", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1", { "PFN1" }, { "PC1" } },
        { "Id2", "Name2", "Moniker", "Version", "Channel", { "ID3" }, { "Command" }, "Path2", { "PFN2" }, { "PC2" } },
        { "Id3", "Name3", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path3", { "PFN3" }, { "PC3" } },
        });

    SQLiteVersion testVersion = TestPrepareForRead(index);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Substring, "pc2");

    auto results = index.Search(request);

    if (ArePackageFamilyNameAndProductCodeSupported(index, testVersion))
    {
        REQUIRE(results.Matches.size() == 1);
    }
    else
    {
        REQUIRE(results.Matches.size() == 0);
    }
}

TEST_CASE("SQLiteIndex_Search_PackageFamilyNameSubstring", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id1", "Name1", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1", { "PFN1" }, { "PC1" } },
        { "Id2", "Name2", "Moniker", "Version", "Channel", { "ID3" }, { "Command" }, "Path2", { "PFN2" }, { "PC2" } },
        { "Id3", "Name3", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path3", { "PFN3" }, { "PC3" } },
        });

    SQLiteVersion testVersion = TestPrepareForRead(index);

    SearchRequest request;
    request.Inclusions.emplace_back(PackageMatchField::PackageFamilyName, MatchType::Substring, "PFN");

    auto results = index.Search(request);

    if (ArePackageFamilyNameAndProductCodeSupported(index, testVersion))
    {
        REQUIRE(results.Matches.size() == 3);
    }
    else
    {
        REQUIRE(results.Matches.size() == 0);
    }
}

TEST_CASE("SQLiteIndex_Search_ProductCodeSubstring", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id1", "Name1", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1", { "PFN1" }, { "PC1" } },
        { "Id2", "Name2", "Moniker", "Version", "Channel", { "ID3" }, { "Command" }, "Path2", { "PFN2" }, { "PC2" } },
        { "Id3", "Name3", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path3", { "PFN3" }, { "PC3" } },
        });

    SQLiteVersion testVersion = TestPrepareForRead(index);

    SearchRequest request;
    request.Inclusions.emplace_back(PackageMatchField::ProductCode, MatchType::Substring, "PC");

    auto results = index.Search(request);

    if (ArePackageFamilyNameAndProductCodeSupported(index, testVersion))
    {
        REQUIRE(results.Matches.size() == 3);
    }
    else
    {
        REQUIRE(results.Matches.size() == 0);
    }
}

TEST_CASE("SQLiteIndex_Search_PackageFamilyNameMatch", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id1", "Name1", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1", { "PFN1" }, { "PC1" } },
        { "Id2", "Name2", "Moniker", "Version", "Channel", { "ID3" }, { "Command" }, "Path2", { "PFN2" }, { "PC2" } },
        { "Id3", "Name3", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path3", { "PFN3" }, { "PC3" } },
        });

    SQLiteVersion testVersion = TestPrepareForRead(index);

    SearchRequest request;
    request.Inclusions.emplace_back(PackageMatchField::PackageFamilyName, MatchType::Exact, "pfn1");

    auto results = index.Search(request);

    if (ArePackageFamilyNameAndProductCodeSupported(index, testVersion))
    {
        REQUIRE(results.Matches.size() == 1);
    }
    else
    {
        REQUIRE(results.Matches.size() == 0);
    }
}

TEST_CASE("SQLiteIndex_Search_ProductCodeMatch", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id1", "Name1", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1", { "PFN1" }, { "PC1" } },
        { "Id2", "Name2", "Moniker", "Version", "Channel", { "ID3" }, { "Command" }, "Path2", { "PFN2" }, { "PC2" } },
        { "Id3", "Name3", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path3", { "PFN3" }, { "PC3" } },
        });

    SQLiteVersion testVersion = TestPrepareForRead(index);

    SearchRequest request;
    request.Inclusions.emplace_back(PackageMatchField::ProductCode, MatchType::Exact, "pc2");

    auto results = index.Search(request);

    if (ArePackageFamilyNameAndProductCodeSupported(index, testVersion))
    {
        REQUIRE(results.Matches.size() == 1);
    }
    else
    {
        REQUIRE(results.Matches.size() == 0);
    }
}

TEST_CASE("SQLiteIndex_CheckConsistency_Failure", "[sqliteindex][V1_1]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string manifest1Path = "test/id/test.id-1.0.0.yaml";
    Manifest manifest1;
    manifest1.Installers.push_back({});
    manifest1.Id = "test.id";
    manifest1.DefaultLocalization.Add<Localization::PackageName>("Test Name");
    manifest1.Moniker = "testmoniker";
    manifest1.Version = "1.0.0";
    manifest1.Channel = "test";
    manifest1.DefaultLocalization.Add<Localization::Tags>({ "t1", "t2" });
    manifest1.Installers[0].Commands = { "test1", "test2" };

    std::string manifest2Path = "test/woah/test.id-1.0.0.yaml";
    Manifest manifest2;
    manifest2.Installers.push_back({});
    manifest2.Id = "test.woah";
    manifest2.DefaultLocalization.Add<Localization::PackageName>("Test Name WOAH");
    manifest2.Moniker = "testmoniker";
    manifest2.Version = "1.0.0";
    manifest2.Channel = "test";
    manifest2.DefaultLocalization.Add<Localization::Tags>({});
    manifest2.Installers[0].Commands = { "test1", "test2", "test3" };

    rowid_t manifestRowId = 0;

    {
        SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, { 1, 1 });

        index.AddManifest(manifest1, manifest1Path);
        index.AddManifest(manifest2, manifest2Path);

        // Get the first manifest's id for removal
        SearchRequest request;
        request.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, manifest1.Id));
        auto result = index.Search(request);

        REQUIRE(result.Matches.size() == 1);
        manifestRowId = result.Matches[0].first;
    }

    {
        // Open it directly to modify the table
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);

        Builder::StatementBuilder builder;
        builder.DeleteFrom(Schema::V1_0::IdTable::TableName()).Where(RowIDName).Equals(manifestRowId);
        builder.Execute(connection);
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);

        REQUIRE(!index.CheckConsistency(true));
    }
}

TEST_CASE("SQLiteIndex_GetMultiProperty_PackageFamilyName", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id1", "Name1", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1", { "PFN1", "PFN2" }, {} },
        });

    SQLiteVersion testVersion = TestPrepareForRead(index);

    SearchRequest request;

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto props = index.GetMultiPropertyByPrimaryId(results.Matches[0].first, PackageVersionMultiProperty::PackageFamilyName);

    if (ArePackageFamilyNameAndProductCodeSupported(index, testVersion))
    {
        REQUIRE(props.size() == 2);
        REQUIRE(std::find(props.begin(), props.end(), FoldCase("PFN1"sv)) != props.end());
        REQUIRE(std::find(props.begin(), props.end(), FoldCase("PFN2"sv)) != props.end());
    }
    else
    {
        REQUIRE(props.empty());
    }
}

TEST_CASE("SQLiteIndex_GetMultiProperty_ProductCode", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id1", "Name1", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1", {}, { "PC1", "PC2" } },
        });

    SQLiteVersion testVersion = TestPrepareForRead(index);

    SearchRequest request;

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto props = index.GetMultiPropertyByPrimaryId(results.Matches[0].first, PackageVersionMultiProperty::ProductCode);

    if (ArePackageFamilyNameAndProductCodeSupported(index, testVersion))
    {
        REQUIRE(props.size() == 2);
        REQUIRE(std::find(props.begin(), props.end(), FoldCase("PC1"sv)) != props.end());
        REQUIRE(std::find(props.begin(), props.end(), FoldCase("PC2"sv)) != props.end());
    }
    else
    {
        REQUIRE(props.empty());
    }
}

TEST_CASE("SQLiteIndex_GetMultiProperty_Tag", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id1", "Name1", "Moniker", "Version", "Channel", { "Tag1", "Tag2" }, { "Command" }, "Path1", {}, { "PC1", "PC2" } },
        });

    SQLiteVersion testVersion = TestPrepareForRead(index);

    SearchRequest request;

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto props = index.GetMultiPropertyByPrimaryId(results.Matches[0].first, PackageVersionMultiProperty::Tag);

    REQUIRE(props.size() == 2);
    REQUIRE(std::find(props.begin(), props.end(), "Tag1") != props.end());
    REQUIRE(std::find(props.begin(), props.end(), "Tag2") != props.end());
}

TEST_CASE("SQLiteIndex_GetMultiProperty_Command", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id1", "Name1", "Moniker", "Version", "Channel", { "Tag1", "Tag2" }, { "Command" }, "Path1", {}, { "PC1", "PC2" } },
        });

    SQLiteVersion testVersion = TestPrepareForRead(index);

    SearchRequest request;

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto props = index.GetMultiPropertyByPrimaryId(results.Matches[0].first, PackageVersionMultiProperty::Command);

    REQUIRE(props.size() == 1);
    REQUIRE(props[0] == "Command");
}

TEST_CASE("SQLiteIndex_ManifestMetadata", "[sqliteindex][V1_7]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id1", "Name1", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1", {}, { "PC1", "PC2" } },
        { "Id2", "Name2", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path2", { "PFN1", "PFN2" }, {} },
        }, SQLiteVersion{ 1, 7 });

    SQLiteVersion testVersion = TestPrepareForRead(index);

    SearchRequest request;

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 2);

    for (const auto [id, match] : results.Matches)
    {
        REQUIRE(index.GetMetadataByManifestId(id).empty());
    }

    auto manifestId1 = results.Matches[0].first;
    auto manifestId2 = results.Matches[1].first;

    std::string metadataValue = "data about data";

    index.SetMetadataByManifestId(manifestId1, PackageVersionMetadata::InstalledType, metadataValue);

    if (IsManifestMetadataSupported(index, testVersion))
    {
        auto metadataResult = index.GetMetadataByManifestId(manifestId1);
        REQUIRE(metadataResult.size() == 1);
        REQUIRE(metadataResult[0].first == PackageVersionMetadata::InstalledType);
        REQUIRE(metadataResult[0].second == metadataValue);
    }
    else
    {
        REQUIRE(index.GetMetadataByManifestId(manifestId1).empty());
    }

    REQUIRE(index.GetMetadataByManifestId(manifestId2).empty());
}

TEST_CASE("SQLiteIndex_NormNameAndPublisher_Exact", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string testName = "Name";
    std::string testPublisher = "Publisher";

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id1", testName, testPublisher, "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1", {}, { "PC1", "PC2" } },
        });

    SQLiteVersion testVersion = TestPrepareForRead(index);

    SearchRequest request;
    request.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::NormalizedNameAndPublisher, MatchType::Exact, testName, testPublisher));

    auto results = index.Search(request);

    if (AreNormalizedNameAndPublisherSupported(index, testVersion))
    {
        REQUIRE(results.Matches.size() == 1);
    }
    else
    {
        REQUIRE(results.Matches.empty());
    }
}

TEST_CASE("SQLiteIndex_NormNameAndPublisher_Simple", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string testName = "Name";
    std::string testPublisher = "Publisher";

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id1", testName, testPublisher, "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1", {}, { "PC1", "PC2" } },
        });

    SQLiteVersion testVersion = TestPrepareForRead(index);

    SearchRequest request;
    request.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::NormalizedNameAndPublisher, MatchType::Exact, testName + " 1.0", testPublisher + " Corporation"));

    auto results = index.Search(request);

    if (AreNormalizedNameAndPublisherSupported(index, testVersion))
    {
        REQUIRE(results.Matches.size() == 1);
    }
    else
    {
        REQUIRE(results.Matches.empty());
    }
}

TEST_CASE("SQLiteIndex_NormNameAndPublisher_Complex", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string testName = "Name";
    std::string testPublisher = "Publisher";

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id1", testName, testPublisher, "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1", {}, { "PC1", "PC2" } },
        { "Id2", testName, "Different Publisher", "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path2", {}, { "PC1", "PC2" } },
        });

    SQLiteVersion testVersion = TestPrepareForRead(index);

    SearchRequest request;
    request.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::NormalizedNameAndPublisher, MatchType::Exact, testName + " 1.0", testPublisher));

    auto results = index.Search(request);

    if (AreNormalizedNameAndPublisherSupported(index, testVersion))
    {
        REQUIRE(results.Matches.size() == 1);
    }
    else
    {
        REQUIRE(results.Matches.empty());
    }
}

TEST_CASE("SQLiteIndex_NormNameAndPublisher_AppsAndFeatures", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string testName = "Name";
    std::string testPublisher = "Publisher";
    std::string arpTestName = "Other Thing";
    std::string arpTestPublisher = "Big Company Name";

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id1", testName, testPublisher, "Moniker", "Version", "Channel", { "Tag" }, { "Command" }, "Path1", {}, { "PC1", "PC2" }, arpTestName, arpTestPublisher },
        });

    SQLiteVersion testVersion = TestPrepareForRead(index);

    SearchRequest request;
    request.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::NormalizedNameAndPublisher, MatchType::Exact, arpTestName, arpTestPublisher));

    auto results = index.Search(request);

    if (AreNormalizedNameAndPublisherSupported(index, testVersion))
    {
        REQUIRE(results.Matches.size() == 1);
    }
    else
    {
        REQUIRE(results.Matches.empty());
    }
}

TEST_CASE("SQLiteIndex_ManifestHash_Present", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    uint8_t data[4] = { 1, 2, 3, 4 };
    SHA256::HashBuffer hash = SHA256::ComputeHash(data, sizeof(data));

    SQLiteIndex index = CreateTestIndex(tempFile);

    Manifest manifest;
    manifest.Id = "Foo";
    manifest.Version = "Bar";
    manifest.StreamSha256 = hash;
    index.AddManifest(manifest, "path");

    SQLiteVersion testVersion = TestPrepareForRead(index);

    auto results = index.Search({});
    REQUIRE(results.Matches.size() == 1);

    auto hashResult = index.GetPropertyByPrimaryId(results.Matches[0].first, PackageVersionProperty::ManifestSHA256Hash);

    // Regardless of what hash, it should still be a SHA256 hash
    REQUIRE(hashResult);
    auto hashResultBytes = SHA256::ConvertToBytes(hashResult.value());
    REQUIRE(hash.size() == hashResultBytes.size());

    if (AreManifestHashesSupported(index, testVersion))
    {
        REQUIRE(std::equal(hash.begin(), hash.end(), hashResultBytes.begin()));
    }
}

TEST_CASE("SQLiteIndex_ManifestHash_Missing", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = CreateTestIndex(tempFile);

    Manifest manifest;
    manifest.Id = "Foo";
    manifest.Version = "Bar";
    index.AddManifest(manifest, "path");

    SQLiteVersion testVersion = TestPrepareForRead(index);

    auto results = index.Search({});
    REQUIRE(results.Matches.size() == 1);

    auto hashResult = index.GetPropertyByPrimaryId(results.Matches[0].first, PackageVersionProperty::ManifestSHA256Hash);

    if (AreManifestHashesSupported(index, testVersion))
    {
        REQUIRE(!hashResult);
    }
}

TEST_CASE("SQLiteIndex_ManifestArpVersion_Present_Add", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = CreateTestIndex(tempFile);

    Manifest manifest;
    manifest.Id = "Foo";
    manifest.Version = "Bar";
    manifest.Installers.push_back({});
    manifest.Installers[0].BaseInstallerType = InstallerTypeEnum::Exe;
    manifest.Installers[0].AppsAndFeaturesEntries.push_back({});
    manifest.Installers[0].AppsAndFeaturesEntries[0].DisplayVersion = "1.0";
    manifest.Installers[0].AppsAndFeaturesEntries.push_back({});
    manifest.Installers[0].AppsAndFeaturesEntries[1].DisplayVersion = "1.1";

    index.AddManifest(manifest, "path");

    SQLiteVersion testVersion = TestPrepareForRead(index);

    auto results = index.Search({});
    REQUIRE(results.Matches.size() == 1);

    auto arpMin = index.GetPropertyByPrimaryId(results.Matches[0].first, PackageVersionProperty::ArpMinVersion);
    auto arpMax = index.GetPropertyByPrimaryId(results.Matches[0].first, PackageVersionProperty::ArpMaxVersion);

    if (AreArpVersionsSupported(index, testVersion))
    {
        REQUIRE(arpMin);
        REQUIRE(UtilityVersion(arpMin.value()) == UtilityVersion(manifest.Installers[0].AppsAndFeaturesEntries[0].DisplayVersion));
        REQUIRE(arpMax);
        REQUIRE(UtilityVersion(arpMax.value()) == UtilityVersion(manifest.Installers[0].AppsAndFeaturesEntries[1].DisplayVersion));
    }
    else
    {
        REQUIRE_FALSE(arpMin);
        REQUIRE_FALSE(arpMax);
    }
}

TEST_CASE("SQLiteIndex_ManifestArpVersion_Present_AddThenUpdate", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = CreateTestIndex(tempFile);

    Manifest manifest;
    manifest.Id = "Foo";
    manifest.Version = "Bar";
    manifest.Installers.push_back({});
    manifest.Installers[0].BaseInstallerType = InstallerTypeEnum::Exe;
    manifest.Installers[0].AppsAndFeaturesEntries.push_back({});
    manifest.Installers[0].AppsAndFeaturesEntries[0].DisplayVersion = "1.0";
    manifest.Installers[0].AppsAndFeaturesEntries.push_back({});
    manifest.Installers[0].AppsAndFeaturesEntries[1].DisplayVersion = "1.1";

    index.AddManifest(manifest, "path");

    manifest.Installers[0].AppsAndFeaturesEntries[0].DisplayVersion = "1.1";

    index.UpdateManifest(manifest, "path");

    SQLiteVersion testVersion = TestPrepareForRead(index);

    auto results = index.Search({});
    REQUIRE(results.Matches.size() == 1);

    auto arpMin = index.GetPropertyByPrimaryId(results.Matches[0].first, PackageVersionProperty::ArpMinVersion);
    auto arpMax = index.GetPropertyByPrimaryId(results.Matches[0].first, PackageVersionProperty::ArpMaxVersion);

    if (AreArpVersionsSupported(index, testVersion))
    {
        REQUIRE(arpMin);
        REQUIRE(arpMin.value() == "1.1");
        REQUIRE(arpMax);
        REQUIRE(arpMax.value() == "1.1");
    }
    else
    {
        REQUIRE_FALSE(arpMin);
        REQUIRE_FALSE(arpMax);
    }
}

TEST_CASE("SQLiteIndex_ManifestArpVersion_Empty", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = CreateTestIndex(tempFile);

    Manifest manifest;
    manifest.Id = "Foo";
    manifest.Version = "Bar";
    index.AddManifest(manifest, "path");

    SQLiteVersion testVersion = TestPrepareForRead(index);

    auto results = index.Search({});
    REQUIRE(results.Matches.size() == 1);

    auto arpMin = index.GetPropertyByPrimaryId(results.Matches[0].first, PackageVersionProperty::ArpMinVersion);
    auto arpMax = index.GetPropertyByPrimaryId(results.Matches[0].first, PackageVersionProperty::ArpMaxVersion);

    if (AreArpVersionsSupported(index, testVersion) && !AreArpVersionsNullable(index))
    {
        REQUIRE(arpMin);
        REQUIRE(arpMin.value() == "");
        REQUIRE(arpMax);
        REQUIRE(arpMax.value() == "");
    }
    else
    {
        REQUIRE_FALSE(arpMin);
        REQUIRE_FALSE(arpMax);
    }
}

TEST_CASE("SQLiteIndex_RemoveManifestArpVersionKeepUsedDeleteUnused", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = CreateTestIndex(tempFile, SQLiteVersion::Latest());

    Manifest manifest;
    manifest.Id = "Foo";
    manifest.Version = "10.0";
    manifest.Installers.push_back({});
    manifest.Installers[0].BaseInstallerType = InstallerTypeEnum::Exe;
    manifest.Installers[0].AppsAndFeaturesEntries.push_back({});
    manifest.Installers[0].AppsAndFeaturesEntries[0].DisplayVersion = "1.0";
    manifest.Installers[0].AppsAndFeaturesEntries.push_back({});
    manifest.Installers[0].AppsAndFeaturesEntries[1].DisplayVersion = "1.1";

    index.AddManifest(manifest, "path");

    Manifest manifest2;
    manifest2.Id = "Foo2";
    manifest2.Version = "1.0";
    manifest2.Installers.push_back({});
    manifest2.Installers[0].BaseInstallerType = InstallerTypeEnum::Exe;
    manifest2.Installers[0].AppsAndFeaturesEntries.push_back({});
    manifest2.Installers[0].AppsAndFeaturesEntries[0].DisplayVersion = "10.0";

    index.AddManifest(manifest2, "path2");

    // Before removing, "10.0", "1.0" and "1.1" should all exist.
    {
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadOnly);
        REQUIRE(Schema::V1_0::VersionTable::SelectIdByValue(connection, "10.0").has_value());
        REQUIRE(Schema::V1_0::VersionTable::SelectIdByValue(connection, "1.0").has_value());
        REQUIRE(Schema::V1_0::VersionTable::SelectIdByValue(connection, "1.1").has_value());
    }

    index.RemoveManifest(manifest);

    // After removing the first manifest, "10.0" and "1.0" should still stay, "1.1" should be removed.
    {
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadOnly);
        REQUIRE(Schema::V1_0::VersionTable::SelectIdByValue(connection, "10.0").has_value());
        REQUIRE(Schema::V1_0::VersionTable::SelectIdByValue(connection, "1.0").has_value());
        REQUIRE_FALSE(Schema::V1_0::VersionTable::SelectIdByValue(connection, "1.1").has_value());
    }
}

TEST_CASE("SQLiteIndex_ManifestArpVersionConflict_AddThrows", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = CreateTestIndex(tempFile, SQLiteVersion::Latest());

    Manifest manifest;
    manifest.Id = "Foo";
    manifest.Version = "10.0";
    manifest.DefaultLocalization.Add<Localization::PackageName>("ArpVersionCheckConsistencyTest");
    manifest.Moniker = "testmoniker";
    manifest.Installers.push_back({});
    manifest.Installers[0].BaseInstallerType = InstallerTypeEnum::Exe;
    manifest.Installers[0].AppsAndFeaturesEntries.push_back({});
    manifest.Installers[0].AppsAndFeaturesEntries[0].DisplayVersion = "1.0";
    manifest.Installers[0].AppsAndFeaturesEntries.push_back({});
    manifest.Installers[0].AppsAndFeaturesEntries[1].DisplayVersion = "1.1";

    index.AddManifest(manifest, "path");

    REQUIRE(index.CheckConsistency(true));

    // Add a conflicting one
    manifest.Version = "10.1";

    REQUIRE_THROWS_HR(index.AddManifest(manifest, "path2"), APPINSTALLER_CLI_ERROR_ARP_VERSION_VALIDATION_FAILED);
}

TEST_CASE("SQLiteIndex_ManifestArpVersionConflict_UpdateThrows", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = CreateTestIndex(tempFile, SQLiteVersion::Latest());

    Manifest manifest;
    manifest.Id = "Foo";
    manifest.Version = "10.0";
    manifest.DefaultLocalization.Add<Localization::PackageName>("ArpVersionCheckConsistencyTest");
    manifest.Moniker = "testmoniker";
    manifest.Installers.push_back({});
    manifest.Installers[0].BaseInstallerType = InstallerTypeEnum::Exe;
    manifest.Installers[0].AppsAndFeaturesEntries.push_back({});
    manifest.Installers[0].AppsAndFeaturesEntries[0].DisplayVersion = "1.0";
    manifest.Installers[0].AppsAndFeaturesEntries.push_back({});
    manifest.Installers[0].AppsAndFeaturesEntries[1].DisplayVersion = "1.1";

    index.AddManifest(manifest, "path");
    REQUIRE(index.CheckConsistency(true));

    // Add another version
    manifest.Version = "10.1";
    manifest.Installers[0].AppsAndFeaturesEntries[0].DisplayVersion = "2.0";
    manifest.Installers[0].AppsAndFeaturesEntries[1].DisplayVersion = "2.1";

    index.AddManifest(manifest, "path2");
    REQUIRE(index.CheckConsistency(true));

    // Update to a conflict
    manifest.Installers[0].AppsAndFeaturesEntries[0].DisplayVersion = "1.0";
    manifest.Installers[0].AppsAndFeaturesEntries[1].DisplayVersion = "2.1";

    REQUIRE_THROWS_HR(index.UpdateManifest(manifest, "path2"), APPINSTALLER_CLI_ERROR_ARP_VERSION_VALIDATION_FAILED);
}

TEST_CASE("SQLiteIndex_ManifestArpVersion_ValidateManifestAgainstIndex", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = CreateTestIndex(tempFile, SQLiteVersion::Latest());

    Manifest manifest;
    manifest.Id = "Foo";
    manifest.Version = "10.0";
    manifest.Installers.push_back({});
    manifest.Installers[0].BaseInstallerType = InstallerTypeEnum::Exe;
    manifest.Installers[0].AppsAndFeaturesEntries.push_back({});
    manifest.Installers[0].AppsAndFeaturesEntries[0].DisplayVersion = "1.0";
    manifest.Installers[0].AppsAndFeaturesEntries.push_back({});
    manifest.Installers[0].AppsAndFeaturesEntries[1].DisplayVersion = "1.1";

    index.AddManifest(manifest, "path");

    // Updating same version should not result in failure.
    REQUIRE_NOTHROW(ValidateManifestArpVersion(&index, manifest));

    // Add different version should result in failure.
    manifest.Version = "10.1";
    REQUIRE_THROWS(ValidateManifestArpVersion(&index, manifest));
}

TEST_CASE("SQLiteIndex_CheckConsistency_FindEmbeddedNull", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = CreateTestIndex(tempFile, SQLiteVersion::Latest());

    Manifest manifest;
    manifest.Id = "Foo";
    manifest.Version = "10.0";
    manifest.Installers.push_back({});
    manifest.Installers[0].BaseInstallerType = InstallerTypeEnum::Exe;
    manifest.Installers[0].AppsAndFeaturesEntries.push_back({});
    manifest.Installers[0].AppsAndFeaturesEntries[0].DisplayVersion = "1.0";
    manifest.Installers[0].AppsAndFeaturesEntries.push_back({});
    manifest.Installers[0].AppsAndFeaturesEntries[1].DisplayVersion = "1.1";

    index.AddManifest(manifest, "path");

    // Inject a null character using SQL without binding since we block it
    Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);
    Statement update = Statement::Create(connection, "Update versions set version = '10.0'||char(0)||'After Null' where version = '10.0'");
    update.Execute();

    REQUIRE(!index.CheckConsistency(true));
}

TEST_CASE("SQLiteIndex_MapDataFolding_Tags", "[sqliteindex][mapdatafolding]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string tag1 = "Tag1";
    std::string tag2 = "Tag2";

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id", "Name", "Publisher", "Moniker", "Version1", "", { tag1 }, { "Command" }, "Path1", {}, { "PC1" } },
        { "Id", "Name", "Publisher", "Moniker", "Version2", "", { tag2 }, { "Command" }, "Path2", {}, { "PC2" } },
        });

    SQLiteVersion testVersion = TestPrepareForRead(index);

    SearchRequest request1;
    request1.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::Tag, MatchType::Exact, tag1));
    auto results1 = index.Search(request1);

    SearchRequest request2;
    request2.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::Tag, MatchType::Exact, tag2));
    auto results2 = index.Search(request2);

    REQUIRE(results1.Matches.size() == 1);
    REQUIRE(results2.Matches.size() == 1);
    REQUIRE(results1.Matches[0].first == results2.Matches[0].first);
}

TEST_CASE("SQLiteIndex_MapDataFolding_PFNs", "[sqliteindex][mapdatafolding]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string pfn1 = "PFN1";
    std::string pfn2 = "PFN2";

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id", "Name", "Publisher", "Moniker", "Version1", "", { }, { "Command" }, "Path1", { pfn1 }, { } },
        { "Id", "Name", "Publisher", "Moniker", "Version2", "", { }, { "Command" }, "Path2", { pfn2 }, { } },
        });

    SQLiteVersion testVersion = TestPrepareForRead(index);

    if (!AreChannelsSupported(index))
    {
        return;
    }

    SearchRequest request1;
    request1.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::PackageFamilyName, MatchType::Exact, pfn1));
    auto results1 = index.Search(request1);

    SearchRequest request2;
    request2.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::PackageFamilyName, MatchType::Exact, pfn2));
    auto results2 = index.Search(request2);

    REQUIRE(results1.Matches.size() == 1);
    REQUIRE(results2.Matches.size() == 1);
    REQUIRE(results1.Matches[0].first == results2.Matches[0].first);

    auto versionKeys = index.GetVersionKeysById(results1.Matches[0].first);
    REQUIRE(versionKeys.size() == 2);

    auto manifestId1 = versionKeys[0].ManifestId;
    auto manifestId2 = versionKeys[1].ManifestId;

    auto pfnValues1 = index.GetMultiPropertyByPrimaryId(manifestId1, PackageVersionMultiProperty::PackageFamilyName);
    auto pfnValues2 = index.GetMultiPropertyByPrimaryId(manifestId2, PackageVersionMultiProperty::PackageFamilyName);

    if (IsMapDataFoldingSupported(index, testVersion))
    {
        REQUIRE(pfnValues1.size() == 2);
        REQUIRE(pfnValues2.size() == 2);
        REQUIRE(pfnValues1[0] != pfnValues1[1]);
    }
    else if (IsMapDataFolded(index))
    {
        if (manifestId1 > manifestId2)
        {
            REQUIRE(pfnValues1.size() == 2);
            REQUIRE(pfnValues2.size() == 0);
            REQUIRE(pfnValues1[0] != pfnValues1[1]);
        }
        else
        {
            REQUIRE(pfnValues1.size() == 0);
            REQUIRE(pfnValues2.size() == 2);
            REQUIRE(pfnValues2[0] != pfnValues2[1]);
        }
    }
    else
    {
        REQUIRE(pfnValues1.size() == 1);
        REQUIRE(pfnValues2.size() == 1);
        REQUIRE(pfnValues1[0] != pfnValues2[0]);
    }
}

TEST_CASE("SQLiteIndex_MapDataFolding_ProductCodes", "[sqliteindex][mapdatafolding]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string pc1 = "PC1";
    std::string pc2 = "PC2";

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { "Id", "Name", "Publisher", "Moniker", "Version1", "", { }, { "Command" }, "Path1", { }, { pc1 } },
        { "Id", "Name", "Publisher", "Moniker", "Version2", "", { }, { "Command" }, "Path2", { }, { pc2 } },
        });

    SQLiteVersion testVersion = TestPrepareForRead(index);

    if (!AreChannelsSupported(index))
    {
        return;
    }

    SearchRequest request1;
    request1.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::ProductCode, MatchType::Exact, pc1));
    auto results1 = index.Search(request1);

    SearchRequest request2;
    request2.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::ProductCode, MatchType::Exact, pc2));
    auto results2 = index.Search(request2);

    REQUIRE(results1.Matches.size() == 1);
    REQUIRE(results2.Matches.size() == 1);
    REQUIRE(results1.Matches[0].first == results2.Matches[0].first);

    auto versionKeys = index.GetVersionKeysById(results1.Matches[0].first);
    REQUIRE(versionKeys.size() == 2);

    auto manifestId1 = versionKeys[0].ManifestId;
    auto manifestId2 = versionKeys[1].ManifestId;

    auto pcValues1 = index.GetMultiPropertyByPrimaryId(manifestId1, PackageVersionMultiProperty::ProductCode);
    auto pcValues2 = index.GetMultiPropertyByPrimaryId(manifestId2, PackageVersionMultiProperty::ProductCode);

    REQUIRE(pcValues1.size() == 1);
    REQUIRE(pcValues2.size() == 1);
    REQUIRE(pcValues1[0] != pcValues2[0]);
}

TEST_CASE("SQLiteIndex_FilePath_Memory", "[sqliteindex]")
{
    SQLiteIndex index = SQLiteIndex::CreateNew(SQLITE_MEMORY_DB_CONNECTION_TARGET);
    auto contextData = index.GetContextData();
    REQUIRE(!contextData.Contains(Schema::Property::DatabaseFilePath));
}

TEST_CASE("SQLiteIndex_FilePath_Create", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };

    SQLiteIndex index = SQLiteIndex::CreateNew(tempFile);
    auto contextData = index.GetContextData();
    REQUIRE(contextData.Contains(Schema::Property::DatabaseFilePath));
    REQUIRE(contextData.Get<Schema::Property::DatabaseFilePath>() == tempFile.GetPath());
}

TEST_CASE("SQLiteIndex_FilePath_Open", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };

    {
        SQLiteIndex index = SQLiteIndex::CreateNew(tempFile);
    }

    SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::Read);
    auto contextData = index.GetContextData();
    REQUIRE(contextData.Contains(Schema::Property::DatabaseFilePath));
    REQUIRE(contextData.Get<Schema::Property::DatabaseFilePath>() == tempFile.GetPath());
}

TEST_CASE("SQLiteIndex_MigrateTo_Unsupported", "[sqliteindex][V1_7]")
{
    SQLiteIndex index = SQLiteIndex::CreateNew(SQLITE_MEMORY_DB_CONNECTION_TARGET, SQLiteVersion{ 1, 6 });
    REQUIRE(!index.MigrateTo(SQLiteVersion{ 1, 7 }));
}

TEST_CASE("SQLiteIndex_MigrateTo_Empty", "[sqliteindex][V2_0]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    {
        SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, SQLiteVersion{ 1, 7 });
        REQUIRE(index.MigrateTo(SQLiteVersion{ 2, 0 }));
        REQUIRE(index.GetVersion() == SQLiteVersion{ 2, 0 });
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::Read);
        REQUIRE(index.GetVersion() == SQLiteVersion{ 2, 0 });
    }
}

TEST_CASE("SQLiteIndex_MigrateTo_Data", "[sqliteindex][V2_0]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string packageId1 = "Id1";
    std::string packageId2 = "Id2";
    std::string packageId3 = "Id3";

    SQLiteIndex index = SearchTestSetup(tempFile, {
        { packageId1, "Name1", "Moniker", "Version", "", { "Tag" }, { "Command" }, "Path1", { "PFN1" }, { "PC1" } },
        { packageId2, "Name2", "Moniker", "Version", "", { "ID3" }, { "Command" }, "Path2", { "PFN2" }, { "PC2" } },
        { packageId3, "Name3", "Moniker", "Version", "", { "Tag" }, { "Command" }, "Path3", { "PFN3" }, { "PC3" } },
        });

    auto preMigrationVersion = index.GetVersion();

    if (preMigrationVersion == SQLiteVersion{ 1, 7 })
    {
        REQUIRE(index.MigrateTo(SQLiteVersion{ 2, 0 }));
        REQUIRE(index.GetVersion() == SQLiteVersion{ 2, 0 });

        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);
        auto updateData = Schema::V2_0::PackageUpdateTrackingTable::GetUpdatesSince(connection, 0);

        REQUIRE(updateData.size() == 3);
        REQUIRE(std::count_if(updateData.begin(), updateData.end(), [&](const auto& x) { return x.PackageIdentifier == packageId1; }) == 1);
        REQUIRE(std::count_if(updateData.begin(), updateData.end(), [&](const auto& x) { return x.PackageIdentifier == packageId2; }) == 1);
        REQUIRE(std::count_if(updateData.begin(), updateData.end(), [&](const auto& x) { return x.PackageIdentifier == packageId3; }) == 1);
    }
    else
    {
        REQUIRE(!index.MigrateTo(SQLiteVersion{ 2, 0 }));
        REQUIRE(index.GetVersion() == preMigrationVersion);
    }
}

TEST_CASE("SQLiteIndex_Property_IntermediateFilePath", "[sqliteindex]")
{
    SQLiteIndex index = SQLiteIndex::CreateNew(SQLITE_MEMORY_DB_CONNECTION_TARGET);
    std::filesystem::path intermediateFilePath = "A:\\Path";
    index.SetProperty(SQLiteIndex::Property::IntermediateFileOutputPath, intermediateFilePath.u8string());

    auto contextData = index.GetContextData();
    REQUIRE(contextData.Contains(Schema::Property::IntermediateFileOutputPath));
    REQUIRE(contextData.Get<Schema::Property::IntermediateFileOutputPath>() == intermediateFilePath);
}

struct ManifestAndPath
{
    Manifest Manifest;
    std::string Path;
};

void CreateFakeManifestAndPath(
    ManifestAndPath& manifestAndPath,
    const string_t& publisher,
    std::string_view version = "1.0.0",
    std::optional<std::string_view> arpMinVersion = {},
    std::optional<std::string_view> arpMaxVersion = {})
{
    CreateFakeManifest(manifestAndPath.Manifest, publisher, version);
    manifestAndPath.Path = ConvertToUTF8(CreateNewGuidNameWString());
    manifestAndPath.Manifest.StreamSha256 = SHA256::ComputeHash(manifestAndPath.Path);

    if (arpMinVersion)
    {
        manifestAndPath.Manifest.Installers[0].BaseInstallerType = InstallerTypeEnum::Exe;
        manifestAndPath.Manifest.Installers[0].AppsAndFeaturesEntries.push_back({});
        manifestAndPath.Manifest.Installers[0].AppsAndFeaturesEntries.back().DisplayVersion = arpMinVersion.value();
    }

    if (arpMaxVersion)
    {
        manifestAndPath.Manifest.Installers[0].BaseInstallerType = InstallerTypeEnum::Exe;
        manifestAndPath.Manifest.Installers[0].AppsAndFeaturesEntries.push_back({});
        manifestAndPath.Manifest.Installers[0].AppsAndFeaturesEntries.back().DisplayVersion = arpMaxVersion.value();
    }
}

std::filesystem::path GetOnlyChild(const std::filesystem::path& parent)
{
    auto parentDirectoryIterator = std::filesystem::directory_iterator{ parent };
    std::filesystem::path result = parentDirectoryIterator->path();
    REQUIRE(++parentDirectoryIterator == std::filesystem::directory_iterator{});
    return result;
}

void CheckIntermediates(const std::filesystem::path& baseDirectory, const std::vector<std::vector<ManifestAndPath>>& expectedIntermediatesData, std::chrono::seconds sleep = 1s)
{
    std::filesystem::path intermediatesDirectory = baseDirectory / "packages";

    size_t intermediatePackageCount = std::count_if(std::filesystem::directory_iterator{ intermediatesDirectory }, std::filesystem::directory_iterator{}, [](const auto&){ return true; });
    REQUIRE(intermediatePackageCount == expectedIntermediatesData.size());

    for (const auto& versions : expectedIntermediatesData)
    {
        REQUIRE(!versions.empty());
        INFO(versions[0].Manifest.Id);
        std::filesystem::path packageDirectory = intermediatesDirectory / ConvertToUTF16(versions[0].Manifest.Id);

        REQUIRE(std::filesystem::exists(packageDirectory));
        std::filesystem::path hashDirectory = GetOnlyChild(packageDirectory);

        std::filesystem::path versionDataFile = GetOnlyChild(hashDirectory);
        std::ifstream versionDataStream{ versionDataFile, std::ios_base::in | std::ios_base::binary };
        auto versionDataBytes = ReadEntireStreamAsByteArray(versionDataStream);

        PackageVersionDataManifest versionDataManifest;
        versionDataManifest.Deserialize(PackageVersionDataManifest::CreateDecompressor().Decompress(versionDataBytes));

        const auto& versionDataVersions = versionDataManifest.Versions();
        REQUIRE(versionDataVersions.size() == versions.size());

        for (const auto& manifestAndPath : versions)
        {
            const auto& versionDataItr = std::find_if(versionDataVersions.begin(), versionDataVersions.end(), [&](const auto& v) { return v.Version == manifestAndPath.Manifest.Version; });
            REQUIRE(versionDataItr != versionDataVersions.end());
            const auto& versionData = *versionDataItr;

            REQUIRE(manifestAndPath.Path == versionData.ManifestRelativePath);
            REQUIRE(SHA256::ConvertToString(manifestAndPath.Manifest.StreamSha256) == versionData.ManifestHash);

            auto versionRange = manifestAndPath.Manifest.GetArpVersionRange();
            if (!versionRange.IsEmpty())
            {
                REQUIRE(versionData.ArpMinVersion);
                REQUIRE(versionRange.GetMinVersion() == versionData.ArpMinVersion.value());
                REQUIRE(versionData.ArpMaxVersion);
                REQUIRE(versionRange.GetMaxVersion() == versionData.ArpMaxVersion.value());
            }
        }
    }

    // This is needed to force the timestamp to roll over to a new value for the next call to this function.
    // An alternate solution would be to hook the timestamp function and control the values it returns
    // so that we can advance/halt time arbitrarily.
    std::this_thread::sleep_for(sleep);
}

void PrepareAndCheckIntermediates(const std::filesystem::path& baseFile, const std::filesystem::path& preparedFile, const std::vector<std::vector<ManifestAndPath>>& expectedIntermediatesData, std::chrono::seconds sleep = 1s)
{
    TempDirectory intermediatesDirectory{ "v2_0_intermediates" };
    INFO("Intermediates directory: " << intermediatesDirectory.GetPath());

    std::filesystem::copy_file(baseFile, preparedFile, std::filesystem::copy_options::overwrite_existing);

    SQLiteIndex index = SQLiteIndex::Open(preparedFile.u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite);
    index.SetProperty(SQLiteIndex::Property::IntermediateFileOutputPath, intermediatesDirectory);
    index.PrepareForPackaging();

    CheckIntermediates(intermediatesDirectory, expectedIntermediatesData, sleep);
}

TEST_CASE("SQLiteIndex_V2_0_UsageFlow_Simple", "[sqliteindex][V2_0]")
{
    TempFile baseFile{ "v2_0_index_tempdb"s, ".db"s };
    TempFile preparedFile{ "v2_0_index_prepared_tempdb"s, ".db"s };
    INFO("Using files named: [" << baseFile.GetPath() << "] and [" << preparedFile.GetPath() << "]");

    // Create empty index
    std::ignore = SQLiteIndex::CreateNew(baseFile, SQLiteVersion{ 2, 0 });

    std::string publisher = "Publisher";
    ManifestAndPath manifest1;
    CreateFakeManifestAndPath(manifest1, publisher, "1.0");

    {
        // Open existing file to add a manifest
        SQLiteIndex index = SQLiteIndex::Open(baseFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        index.SetProperty(SQLiteIndex::Property::PackageUpdateTrackingBaseTime, "");
        index.AddManifest(manifest1.Manifest, manifest1.Path);
    }

    PrepareAndCheckIntermediates(baseFile, preparedFile, { { manifest1 } }, 0s);
}

TEST_CASE("SQLiteIndex_V2_0_UsageFlow_Complex", "[sqliteindex][V2_0]")
{
    TempFile baseFile{ "v2_0_index_tempdb"s, ".db"s };
    TempFile preparedFile{ "v2_0_index_prepared_tempdb"s, ".db"s };
    INFO("Using files named: [" << baseFile.GetPath() << "] and [" << preparedFile.GetPath() << "]");

    // Create empty index
    std::ignore = SQLiteIndex::CreateNew(baseFile, SQLiteVersion{ 2, 0 });

    // Open existing file to add a new package
    std::string Publisher1 = "Publisher1";
    ManifestAndPath manifest1;
    CreateFakeManifestAndPath(manifest1, Publisher1, "1.0");

    {
        SQLiteIndex index = SQLiteIndex::Open(baseFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        index.SetProperty(SQLiteIndex::Property::PackageUpdateTrackingBaseTime, "");
        index.AddManifest(manifest1.Manifest, manifest1.Path);
    }

    PrepareAndCheckIntermediates(baseFile, preparedFile, { { manifest1 } });

    // Open existing file to add another new package
    ManifestAndPath manifest2;
    CreateFakeManifestAndPath(manifest2, "Publisher2", "1.0");

    {
        SQLiteIndex index = SQLiteIndex::Open(baseFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        index.SetProperty(SQLiteIndex::Property::PackageUpdateTrackingBaseTime, "");
        index.AddManifest(manifest2.Manifest, manifest2.Path);
    }

    PrepareAndCheckIntermediates(baseFile, preparedFile, { { manifest2 } });

    // Open existing file to add a new version of existing package
    ManifestAndPath manifest3;
    CreateFakeManifestAndPath(manifest3, Publisher1, "2.0");

    {
        SQLiteIndex index = SQLiteIndex::Open(baseFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        index.SetProperty(SQLiteIndex::Property::PackageUpdateTrackingBaseTime, "");
        index.AddManifest(manifest3.Manifest, manifest3.Path);
    }

    PrepareAndCheckIntermediates(baseFile, preparedFile, { { manifest1, manifest3 } });

    // Open existing file to add a new version of existing package and update an existing version
    manifest2.Manifest.StreamSha256 = SHA256::ComputeHash(manifest2.Manifest.Id);

    ManifestAndPath manifest4;
    CreateFakeManifestAndPath(manifest4, Publisher1, "3.0");

    {
        SQLiteIndex index = SQLiteIndex::Open(baseFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        index.SetProperty(SQLiteIndex::Property::PackageUpdateTrackingBaseTime, "");
        index.UpdateManifest(manifest2.Manifest, manifest2.Path);
        index.AddManifest(manifest4.Manifest, manifest4.Path);
    }

    PrepareAndCheckIntermediates(baseFile, preparedFile, { { manifest2 }, { manifest1, manifest3, manifest4 } }, 0s);
}

void MigratePrepareAndCheckIntermediates(const std::filesystem::path& baseFile, const std::filesystem::path& preparedFile, const std::vector<std::vector<ManifestAndPath>>& expectedIntermediatesData)
{
    TempDirectory intermediatesDirectory{ "v2_0_intermediates" };
    INFO("Intermediates directory: " << intermediatesDirectory.GetPath());

    std::filesystem::copy_file(baseFile, preparedFile, std::filesystem::copy_options::overwrite_existing);

    SQLiteIndex index = SQLiteIndex::Open(preparedFile.u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite);
    index.MigrateTo({ 2, 0 });
    index.SetProperty(SQLiteIndex::Property::IntermediateFileOutputPath, intermediatesDirectory);
    index.PrepareForPackaging();

    CheckIntermediates(intermediatesDirectory, expectedIntermediatesData, 0s);
}

TEST_CASE("SQLiteIndex_V2_0_UsageFlow_ComplexMigration", "[sqliteindex][V2_0]")
{
    TempFile baseFile{ "v1_7_index_tempdb"s, ".db"s };
    TempFile preparedFile{ "v2_0_index_prepared_tempdb"s, ".db"s };
    INFO("Using files named: [" << baseFile.GetPath() << "] and [" << preparedFile.GetPath() << "]");

    // Create empty index
    std::ignore = SQLiteIndex::CreateNew(baseFile, SQLiteVersion{ 1, 7 });

    // Open existing file to add a new package
    std::string Publisher1 = "Publisher1";
    ManifestAndPath manifest1;
    CreateFakeManifestAndPath(manifest1, Publisher1, "1.0");

    {
        SQLiteIndex index = SQLiteIndex::Open(baseFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        index.AddManifest(manifest1.Manifest, manifest1.Path);
    }

    MigratePrepareAndCheckIntermediates(baseFile, preparedFile, { { manifest1 } });

    // Open existing file to add another new package
    ManifestAndPath manifest2;
    CreateFakeManifestAndPath(manifest2, "Publisher2", "1.0");

    {
        SQLiteIndex index = SQLiteIndex::Open(baseFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        index.AddManifest(manifest2.Manifest, manifest2.Path);
    }

    MigratePrepareAndCheckIntermediates(baseFile, preparedFile, { {  manifest2 }, { manifest1 } });

    // Open existing file to add a new version of existing package
    ManifestAndPath manifest3;
    CreateFakeManifestAndPath(manifest3, Publisher1, "2.0");

    {
        SQLiteIndex index = SQLiteIndex::Open(baseFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        index.AddManifest(manifest3.Manifest, manifest3.Path);
    }

    MigratePrepareAndCheckIntermediates(baseFile, preparedFile, { {  manifest2 }, { manifest1, manifest3 } });

    // Open existing file to add a new version of existing package and update an existing version
    manifest2.Manifest.StreamSha256 = SHA256::ComputeHash(manifest2.Manifest.Id);

    ManifestAndPath manifest4;
    CreateFakeManifestAndPath(manifest4, Publisher1, "3.0");

    {
        SQLiteIndex index = SQLiteIndex::Open(baseFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        index.UpdateManifest(manifest2.Manifest, manifest2.Path);
        index.AddManifest(manifest4.Manifest, manifest4.Path);
    }

    MigratePrepareAndCheckIntermediates(baseFile, preparedFile, { { manifest2 }, { manifest1, manifest3, manifest4 } });
}

TEST_CASE("SQLiteIndex_DependencyWithCaseMismatch", "[sqliteindex][V1_4]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest dependencyManifest1, dependencyManifest2, manifest;
    SQLiteIndex index = SimpleTestSetup(tempFile, dependencyManifest1, SQLiteVersion::Latest());

    // Must contain some upper case
    auto& publisher2 = "Test2";
    CreateFakeManifest(dependencyManifest2, publisher2);
    index.AddManifest(dependencyManifest2, GetPathFromManifest(dependencyManifest2));

    auto& publisher3 = "Test3";
    CreateFakeManifest(manifest, publisher3);
    manifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, dependencyManifest1.Id, "1.0.0"));
    manifest.Installers[0].Dependencies.Add(Dependency(DependencyType::Package, ToLower(dependencyManifest2.Id), "1.0.0"));

    index.AddManifest(manifest, GetPathFromManifest(manifest));
}

TEST_CASE("SQLiteIndex_AddOrUpdateManifest", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string manifestPath = "test/id/test.id-1.0.0.yaml";
    Manifest manifest;
    manifest.Installers.push_back({});
    manifest.Id = "test.id";
    manifest.DefaultLocalization.Add < Localization::PackageName>("Test Name");
    manifest.Moniker = "testmoniker";
    manifest.Version = "1.0.0";
    manifest.Channel = "test";
    manifest.DefaultLocalization.Add<Localization::Tags>({ "t1", "t2" });
    manifest.Installers[0].Commands = { "test1", "test2" };

    {
        auto version = GENERATE(SQLiteVersion{ 1, 0 }, SQLiteVersion::Latest());
        SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, version);

        REQUIRE(index.AddOrUpdateManifest(manifest, manifestPath));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);

        // Update should return false
        REQUIRE(!index.AddOrUpdateManifest(manifest, manifestPath));

        manifest.DefaultLocalization.Add<Localization::Description>("description2");

        // Update should return false
        REQUIRE(!index.AddOrUpdateManifest(manifest, manifestPath));

        // Update with indexed changes should still return false
        manifest.DefaultLocalization.Add<Localization::PackageName>("Test Name2");
        manifest.Moniker = "testmoniker2";
        manifest.DefaultLocalization.Add<Localization::Tags>({ "t1", "t2", "t3" });
        manifest.Installers[0].Commands = {};

        REQUIRE(!index.AddOrUpdateManifest(manifest, manifestPath));
    }
}

TEST_CASE("SQLiteIndex_VersionStringPreserved", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest manifest;
    SQLiteIndex index = CreateTestIndex(tempFile);

    string_t publisher = "Test";
    std::string version = GENERATE("1.0", "1.10");

    CreateFakeManifest(manifest, publisher, version);
    index.AddManifest(manifest, GetPathFromManifest(manifest));

    TempDirectory intermediatesDirectory{ "v2_0_intermediates" };
    INFO("Intermediates directory: " << intermediatesDirectory.GetPath());

    index.SetProperty(SQLiteIndex::Property::IntermediateFileOutputPath, intermediatesDirectory);
    index.PrepareForPackaging();

    auto results = index.Search({});
    REQUIRE(results.Matches.size() == 1);

    std::string extractedVersion = GetPropertyStringById(index, results.Matches[0].first, PackageVersionProperty::Version);

    REQUIRE(extractedVersion == version);
}
