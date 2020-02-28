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

using namespace std::string_literals;
using namespace TestCommon;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Repository::SQLite;

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

    SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, Schema::Version::Latest());

    Manifest manifest;
    manifest.Id = "test.id";
    manifest.Name = "Test Name";
    manifest.AppMoniker = "testmoniker";
    manifest.Version = "1.0.0";
    manifest.Channel = "test";
    manifest.Tags = { "t1", "t2" };
    manifest.Commands = { "test1", "test2" };

    index.AddManifest(manifest, "test/id/test.id-1.0.0.yml");
}

TEST_CASE("SQLiteIndexCreateAndAddManifestFile", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, Schema::Version::Latest());

    TestDataFile manifestFile{ "GoodManifest.yml" };
    std::filesystem::path manifestPath{ "microsoft/msixsdk/microsoft.msixsdk-1.7.32.yml" };

    index.AddManifest(manifestFile, manifestPath);

    // Attempting to add again should fail
    REQUIRE_THROWS_HR(index.AddManifest(manifestFile, manifestPath), HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS));
}

TEST_CASE("SQLiteIndex_RemoveManifestFile_NotPresent", "[sqliteindex]")
{
    SQLiteIndex index = SQLiteIndex::CreateNew(SQLITE_MEMORY_DB_CONNECTION_TARGET, Schema::Version::Latest());

    TestDataFile manifestFile{ "GoodManifest.yml" };
    std::filesystem::path manifestPath{ "microsoft/msixsdk/microsoft.msixsdk-1.7.32.yml" };

    REQUIRE_THROWS_HR(index.RemoveManifest(manifestFile, manifestPath), E_NOT_SET);
}

TEST_CASE("SQLiteIndex_RemoveManifest", "[sqliteindex][V1_0]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string manifest1Path = "test/id/test.id-1.0.0.yml";
    Manifest manifest1;
    manifest1.Id = "test.id";
    manifest1.Name = "Test Name";
    manifest1.AppMoniker = "testmoniker";
    manifest1.Version = "1.0.0";
    manifest1.Channel = "test";
    manifest1.Tags = { "t1", "t2" };
    manifest1.Commands = { "test1", "test2" };

    std::string manifest2Path = "test/woah/test.id-1.0.0.yml";
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

        TestDataFile manifestFile{ "GoodManifest.yml" };
        std::filesystem::path manifestPath{ "microsoft/msixsdk/microsoft.msixsdk-1.7.32.yml" };

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

    std::string manifestPath = "test/id/test.id-1.0.0.yml";
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

    std::string manifestPath = "test/id/test.id-1.0.0.yml";
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

        manifestPath = "test/newid/test.newid-1.0.0.yml";

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

    TestDataFile manifestFile{ "GoodManifest.yml" };
    std::filesystem::path manifestPath{ "microsoft/msixsdk/microsoft.msixsdk-1.7.32.yml" };

    index.AddManifest(manifestFile, manifestPath);

    index.PrepareForPackaging();
}
