// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <SQLiteWrapper.h>
#include <Manifest/Manifest.h>
#include <Microsoft/SQLiteIndex.h>

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

using namespace std::string_literals;
using namespace TestCommon;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Repository::SQLite;
using namespace AppInstaller::Utility;

SQLiteIndex SimpleTestSetup(const std::string& filePath, Manifest& manifest, std::string& relativePath)
{
    SQLiteIndex index = SQLiteIndex::CreateNew(filePath, Schema::Version::Latest());

    manifest.Id = "Test.Id";
    manifest.Name = "Test Name";
    manifest.AppMoniker = "testmoniker";
    manifest.Version = "1.0.0";
    manifest.Channel = "test";
    manifest.Tags = { "t1", "t2" };
    manifest.Commands = { "test1", "test2" };

    relativePath = "test/id/1.0.0.yaml";

    index.AddManifest(manifest, relativePath);

    return index;
}

struct IndexFields
{
    std::string Id;
    std::string Name;
    std::string Moniker;
    std::string Version;
    std::string Channel;
    std::vector<NormalizedString> Tags;
    std::vector<NormalizedString> Commands;
    std::string Path;
};

SQLiteIndex SearchTestSetup(const std::string& filePath, std::initializer_list<IndexFields> data = {}, Schema::Version version = Schema::Version::Latest())
{
    SQLiteIndex index = SQLiteIndex::CreateNew(filePath, version);

    Manifest manifest;

    auto addFunc = [&](const IndexFields& d) {
        manifest.Id = d.Id;
        manifest.Name = d.Name;
        manifest.AppMoniker = d.Moniker;
        manifest.Version = d.Version;
        manifest.Channel = d.Channel;
        manifest.Tags = d.Tags;
        manifest.Commands = d.Commands;

        index.AddManifest(manifest, d.Path);
    };

    for (const auto& d : data)
    {
        addFunc(d);
    }

    return index;
}

TEST_CASE("SQLiteIndexCreateLatestAndReopen", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Schema::Version versionCreated;

    // Create the index
    {
        SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, Schema::Version::Latest());
        versionCreated = index.GetVersion();
    }

    // Reopen the index for read only
    {
        INFO("Trying with Read");
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::Read);
        Schema::Version versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }

    // Reopen the index for read/write
    {
        INFO("Trying with ReadWrite");
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::ReadWrite);
        Schema::Version versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }

    // Reopen the index for immutable read
    {
        INFO("Trying with Immutable");
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::Immutable);
        Schema::Version versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }
}

TEST_CASE("SQLiteIndexCreateAndAddManifest", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest manifest;
    std::string relativePath;

    SQLiteIndex index = SimpleTestSetup(tempFile, manifest, relativePath);
}

TEST_CASE("SQLiteIndexCreateAndAddManifestFile", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, Schema::Version::Latest());

    TestDataFile manifestFile{ "Manifest-Good.yaml" };
    std::filesystem::path manifestPath{ "microsoft/msixsdk/microsoft.msixsdk-1.7.32.yaml" };

    index.AddManifest(manifestFile, manifestPath);
}

TEST_CASE("SQLiteIndexCreateAndAddManifestDuplicate", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest manifest;
    std::string relativePath;

    SQLiteIndex index = SimpleTestSetup(tempFile, manifest, relativePath);

    // Attempting to add the same manifest at a different path should fail.
    REQUIRE_THROWS_HR(index.AddManifest(manifest, "differentpath.yaml"), HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS));

    // Attempting to add the same manifest with a differently cased Id at a different path should fail.
    manifest.Id = ToLower(manifest.Id);
    REQUIRE_THROWS_HR(index.AddManifest(manifest, "differentpath.yaml"), HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS));

    // Attempting to add a different manifest at the same path should fail.
    manifest.Id += "-new";
    REQUIRE_THROWS_HR(index.AddManifest(manifest, relativePath), HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS));
}

TEST_CASE("SQLiteIndex_RemoveManifestFile_NotPresent", "[sqliteindex]")
{
    SQLiteIndex index = SQLiteIndex::CreateNew(SQLITE_MEMORY_DB_CONNECTION_TARGET, Schema::Version::Latest());

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
    manifest1.Id = "test.id";
    manifest1.Name = "Test Name";
    manifest1.AppMoniker = "testmoniker";
    manifest1.Version = "1.0.0";
    manifest1.Channel = "test";
    manifest1.Tags = { "t1", "t2" };
    manifest1.Commands = { "test1", "test2" };

    std::string manifest2Path = "test/woah/test.id-1.0.0.yaml";
    Manifest manifest2;
    manifest2.Id = "test.woah";
    manifest2.Name = "Test Name WOAH";
    manifest2.AppMoniker = "testmoniker";
    manifest2.Version = "1.0.0";
    manifest2.Channel = "test";
    manifest2.Tags = {};
    manifest2.Commands = { "test1", "test2", "test3" };
    
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
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::ReadWrite);

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

TEST_CASE("SQLiteIndex_UpdateManifest", "[sqliteindex][V1_0]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string manifestPath = "test/id/test.id-1.0.0.yaml";
    Manifest manifest;
    manifest.Id = "test.id";
    manifest.Name = "Test Name";
    manifest.AppMoniker = "testmoniker";
    manifest.Version = "1.0.0";
    manifest.Channel = "test";
    manifest.Tags = { "t1", "t2" };
    manifest.Commands = { "test1", "test2" };

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
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::ReadWrite);

        // Update with no updates should return false
        REQUIRE(!index.UpdateManifest(manifest, manifestPath));

        manifest.Description = "description2";

        // Update with no indexed updates should return false
        REQUIRE(!index.UpdateManifest(manifest, manifestPath));

        // Update with indexed changes
        manifest.Name = "Test Name2";
        manifest.AppMoniker = "testmoniker2";
        manifest.Tags = { "t1", "t2", "t3" };
        manifest.Commands = {};

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
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::ReadWrite);

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

TEST_CASE("SQLiteIndex_UpdateManifestChangePath", "[sqliteindex][V1_0]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string manifestPath = "test/id/test.id-1.0.0.yaml";
    Manifest manifest;
    manifest.Id = "test.id";
    manifest.Name = "Test Name";
    manifest.AppMoniker = "testmoniker";
    manifest.Version = "1.0.0";
    manifest.Channel = "test";
    manifest.Tags = { "t1", "t2" };
    manifest.Commands = { "test1", "test2" };

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
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::ReadWrite);

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
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::ReadWrite);

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

TEST_CASE("SQLiteIndex_UpdateManifestChangeCase", "[sqliteindex][V1_0]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string manifestPath = "test/id/test.id-1.0.0.yaml";
    Manifest manifest;
    manifest.Id = "test.id";
    manifest.Name = "Test Name";
    manifest.AppMoniker = "testmoniker";
    manifest.Version = "1.0.0-test";
    manifest.Channel = "test";
    manifest.Tags = { "t1", "t2" };
    manifest.Commands = { "test1", "test2" };

    {
        SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, { 1, 0 });

        index.AddManifest(manifest, manifestPath);
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::ReadWrite);

        manifest.Id = "Test.Id";

        // Update with path update should indicate change
        REQUIRE(index.UpdateManifest(manifest, manifestPath));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::ReadWrite);

        manifest.Version = "1.0.0-Test";

        // Update with path update should indicate change
        REQUIRE(index.UpdateManifest(manifest, manifestPath));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::ReadWrite);

        manifest.Channel = "Test";

        // Update with path update should indicate change
        REQUIRE(index.UpdateManifest(manifest, manifestPath));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::ReadWrite);

        manifest.Name = "test name";

        // Update with path update should indicate change
        REQUIRE(index.UpdateManifest(manifest, manifestPath));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::ReadWrite);

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
    manifest1.Id = "test.id";
    manifest1.Name = "Test Name";
    manifest1.AppMoniker = "testmoniker";
    manifest1.Version = "1.0.0";
    manifest1.Tags = { "t1", "t2" };
    manifest1.Commands = { "test1", "test2" };

    std::string manifest2Path = "test/id/test.id-2.0.0.yaml";
    Manifest manifest2 = manifest1;
    manifest2.Id = "Test.Id";
    manifest1.Version = "2.0.0";

    {
        SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, { 1, 0 });

        index.AddManifest(manifest1, manifest1Path);

        auto results = index.Search({});
        REQUIRE(results.Matches.size() == 1);
        REQUIRE(manifest1.Id == index.GetIdStringById(results.Matches[0].first));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::ReadWrite);

        index.AddManifest(manifest2, manifest2Path);

        auto results = index.Search({});
        REQUIRE(results.Matches.size() == 1);
        REQUIRE(manifest2.Id == index.GetIdStringById(results.Matches[0].first));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::ReadWrite);

        manifest1.Id = "TEST.ID";

        REQUIRE(index.UpdateManifest(manifest1, manifest1Path));

        auto results = index.Search({});
        REQUIRE(results.Matches.size() == 1);
        REQUIRE(manifest1.Id == index.GetIdStringById(results.Matches[0].first));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::ReadWrite);

        index.RemoveManifest(manifest1, manifest1Path);

        auto results = index.Search({});
        REQUIRE(results.Matches.size() == 1);
        REQUIRE(manifest1.Id == index.GetIdStringById(results.Matches[0].first));
    }

    {
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::ReadWrite);

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
        Schema::Version versionCreated = index.GetVersion();
        REQUIRE(versionCreated == Schema::Version{ 1, 0 });
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

    SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, Schema::Version::Latest());

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
    std::string relativePath;
    SQLiteIndex index = SimpleTestSetup(tempFile, manifest, relativePath);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, manifest.Id);

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);
    REQUIRE(results.Matches[0].second.Field == ApplicationMatchField::Id);
    REQUIRE(results.Matches[0].second.Type == MatchType::Exact);
    REQUIRE(results.Matches[0].second.Value == manifest.Id);
}

TEST_CASE("SQLiteIndex_Search_MultipleMatch", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest manifest;
    std::string relativePath;
    SQLiteIndex index = SimpleTestSetup(tempFile, manifest, relativePath);

    manifest.Version = "2.0.0";
    index.AddManifest(manifest, relativePath + "2");

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, manifest.Id);

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto result = index.GetVersionsById(results.Matches[0].first);
    REQUIRE(result.size() == 2);
}

TEST_CASE("SQLiteIndex_Search_NoMatch", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest manifest;
    std::string relativePath;
    SQLiteIndex index = SimpleTestSetup(tempFile, manifest, relativePath);

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
    std::string relativePath;
    SQLiteIndex index = SimpleTestSetup(tempFile, manifest, relativePath);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, manifest.Id);

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto result = index.GetIdStringById(results.Matches[0].first);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == manifest.Id);
}

TEST_CASE("SQLiteIndex_NameString", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest manifest;
    std::string relativePath;
    SQLiteIndex index = SimpleTestSetup(tempFile, manifest, relativePath);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, manifest.Id);

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto result = index.GetNameStringById(results.Matches[0].first);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == manifest.Name);
}

TEST_CASE("SQLiteIndex_PathString", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest manifest;
    std::string relativePath;
    SQLiteIndex index = SimpleTestSetup(tempFile, manifest, relativePath);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, manifest.Id);

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto specificResult = index.GetPathStringByKey(results.Matches[0].first, manifest.Version, manifest.Channel);
    REQUIRE(specificResult.has_value());
    REQUIRE(specificResult.value() == relativePath);

    auto latestResult = index.GetPathStringByKey(results.Matches[0].first, "", manifest.Channel);
    REQUIRE(latestResult.has_value());
    REQUIRE(latestResult.value() == relativePath);
}

TEST_CASE("SQLiteIndex_Versions", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest manifest;
    std::string relativePath;
    SQLiteIndex index = SimpleTestSetup(tempFile, manifest, relativePath);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, manifest.Id);

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto result = index.GetVersionsById(results.Matches[0].first);
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].GetVersion().ToString() == manifest.Version);
    REQUIRE(result[0].GetChannel().ToString() == manifest.Channel);
}

TEST_CASE("SQLiteIndex_Search_VersionSorting", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::vector<VersionAndChannel> sortedList =
    {
        { Version("15.0.0"), Channel("") },
        { Version("14.0.0"), Channel("") },
        { Version("13.2.0-bugfix"), Channel("") },
        { Version("13.2.0"), Channel("") },
        { Version("13.0.0"), Channel("") },
        { Version("16.0.0"), Channel("alpha") },
        { Version("15.8.0"), Channel("alpha") },
        { Version("15.1.0"), Channel("beta") },
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

    SearchRequest request;
    request.Filters.emplace_back(ApplicationMatchField::Id, MatchType::Exact, "Id");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto result = index.GetVersionsById(results.Matches[0].first);
    REQUIRE(result.size() == sortedList.size());

    for (size_t i = 0; i < result.size(); ++i)
    {
        const VersionAndChannel& sortedVAC = sortedList[i];
        const VersionAndChannel& resultVAC = result[i];

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
        { Version("15.0.0"), Channel("") },
        { Version("14.0.0"), Channel("") },
        { Version("13.2.0-bugfix"), Channel("") },
        { Version("13.2.0"), Channel("") },
        { Version("13.0.0"), Channel("") },
        { Version("16.0.0"), Channel("alpha") },
        { Version("15.8.0"), Channel("alpha") },
        { Version("15.1.0"), Channel("beta") },
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

    SearchRequest request;
    request.Filters.emplace_back(ApplicationMatchField::Id, MatchType::Exact, "Id");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto result = index.GetPathStringByKey(results.Matches[0].first, "", "");
    REQUIRE(result.has_value());
    REQUIRE(result.value() == "Path3");

    result = index.GetPathStringByKey(results.Matches[0].first, "", "alpha");
    REQUIRE(result.has_value());
    REQUIRE(result.value() == "Path2");

    result = index.GetPathStringByKey(results.Matches[0].first, "", "beta");
    REQUIRE(result.has_value());
    REQUIRE(result.value() == "Path5");

    result = index.GetPathStringByKey(results.Matches[0].first, "", "gamma");
    REQUIRE(!result.has_value());
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

    SearchRequest request;
    request.Filters.emplace_back(ApplicationMatchField::Id, MatchType::Exact, "Id");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto result = index.GetPathStringByKey(results.Matches[0].first, "", "Alpha");
    REQUIRE(result.has_value());

    result = index.GetPathStringByKey(results.Matches[0].first, "13.2.0-BugFix", "");
    REQUIRE(result.has_value());

    result = index.GetPathStringByKey(results.Matches[0].first, "13.2.0-BugFix", "BETA");
    REQUIRE(!result.has_value());
}

TEST_CASE("SQLiteIndex_SearchResultsTableSearches", "[sqliteindex][V1_0]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Manifest manifest;
    std::string relativePath;
    {
        (void)SimpleTestSetup(tempFile, manifest, relativePath);
    }

    Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadOnly);
    Schema::V1_0::SearchResultsTable search(connection);

    std::string value = "test";

    // Perform every type of field and match search
    for (auto field : { ApplicationMatchField::Id, ApplicationMatchField::Name, ApplicationMatchField::Moniker, ApplicationMatchField::Tag, ApplicationMatchField::Command })
    {
        for (auto match : { MatchType::Exact, MatchType::Fuzzy, MatchType::FuzzySubstring, MatchType::Substring, MatchType::Wildcard })
        {
            search.SearchOnField(field, match, value);
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

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Substring, "Id");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 2);

    REQUIRE(index.GetIdStringById(results.Matches[0].first) == "Id");
    REQUIRE(index.GetIdStringById(results.Matches[1].first) == "Id2");
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

    SearchRequest request;
    request.Filters.emplace_back(ApplicationMatchField::Name, MatchType::Substring, "a");

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

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Substring, "Id");
    request.Filters.emplace_back(ApplicationMatchField::Name, MatchType::Substring, "Na");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto result = index.GetIdStringById(results.Matches[0].first);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == "Id2");
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

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Substring, "tag");
    request.Filters.emplace_back(ApplicationMatchField::Command, MatchType::Exact, "com3");
    request.Filters.emplace_back(ApplicationMatchField::Tag, MatchType::Substring, "foo");
    request.Filters.emplace_back(ApplicationMatchField::Moniker, MatchType::Substring, "new");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto result = index.GetIdStringById(results.Matches[0].first);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == "Id3");
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

    SearchRequest request;
    // Search for anything containing: [lower] a + umlaut
    request.Filters.emplace_back(ApplicationMatchField::Id, MatchType::Substring, u8"\xE4");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto result = index.GetNameStringById(results.Matches[0].first);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == "HasUmlaut");
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

    SearchRequest request;
    request.Query = RequestMatch(MatchType::CaseInsensitive, "id3");
    request.Inclusions.emplace_back(ApplicationMatchField::Name, MatchType::Substring, "Na");

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

    SearchRequest request;
    request.Inclusions.emplace_back(ApplicationMatchField::Name, MatchType::Substring, "Na");

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

    SearchRequest request;
    request.Inclusions.emplace_back(ApplicationMatchField::Name, MatchType::Substring, "Na");
    request.Filters.emplace_back(ApplicationMatchField::Name, MatchType::CaseInsensitive, "name");

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 1);

    auto result = index.GetIdStringById(results.Matches[0].first);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == "Nope");
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

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Substring, "id3");
    request.Inclusions.emplace_back(ApplicationMatchField::Name, MatchType::Substring, "na");
    request.Filters.emplace_back(ApplicationMatchField::Moniker, MatchType::CaseInsensitive, "MONIKER");

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

    SearchRequest request;
    request.Inclusions.push_back(ApplicationMatchFilter(ApplicationMatchField::Id, MatchType::StartsWith, "id"));

    auto results = index.Search(request);
    REQUIRE(results.Matches.size() == 2);
}
